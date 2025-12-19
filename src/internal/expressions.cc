// Copyright 2025 Darcy Best
// Copyright 2024 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/internal/expressions.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <limits>
#include <memory>
#include <optional>
#include <stack>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/numeric/int128.h"
#include "absl/strings/numbers.h"
#include "src/librarian/errors.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

enum class OperatorT {
  kAdd,
  kSubtract,
  kMultiply,
  kDivide,
  kModulo,
  kExponentiate,
  kUnaryPlus,
  kUnaryNegate,

  // Internal logic assumes these are the only scope operators.
  kCommaScope,
  kOpenParenScope,
  kFunctionStartScope,
  kStartExpressionScope,
};

enum class TokenT {
  kInteger,
  kVariable,

  kFunctionNameWithParen,
  kOpenParen,
  kCloseParen,
  kComma,
  kStartOfExpression,
  kEndOfExpression,

  kAdd,
  kSubtract,
  kMultiply,
  kDivide,
  kModulo,
  kExponentiate,
  kUnaryPlus,
  kUnaryNegate,
};

/* -------------------------------------------------------------------------- */
/*  OPERATORS                                                                 */
/* -------------------------------------------------------------------------- */

absl::int128 Validate(absl::int128 value) {
  if (value < std::numeric_limits<int64_t>::min() ||
      value > std::numeric_limits<int64_t>::max()) {
    throw ExpressionEvaluationError("Expression overflows int64_t");
  }
  return value;
}

absl::int128 Divide(absl::int128 lhs, absl::int128 rhs) {
  if (rhs == 0)
    throw ExpressionEvaluationError("Division by zero in expression");
  return Validate(absl::int128(lhs) / rhs);
}

absl::int128 Mod(absl::int128 lhs, absl::int128 rhs) {
  if (rhs == 0) throw ExpressionEvaluationError("Mod by zero in expression");
  return Validate(absl::int128(lhs) % rhs);
}

absl::int128 Pow(absl::int128 base, absl::int128 exponent) {
  if (exponent < 0)
    throw ExpressionEvaluationError("exponent must be non-negative in pow()");
  if (base == 0 && exponent == 0)
    throw ExpressionEvaluationError("0 to the power of 0 is undefined.");

  absl::int128 result = 1;
  while (exponent > 0) {
    if (exponent % 2 == 1) result = Validate(result * base);
    if (exponent > 1) base = Validate(base * base);
    exponent /= 2;
  }
  return result;
}

void TrimLeadingWhitespace(std::string_view& expression) {
  while (!expression.empty() && std::isspace(expression.front()))
    expression.remove_prefix(1);
}

std::string_view Trim(std::string_view x) {
  while (!x.empty() && std::isspace(x.front())) x.remove_prefix(1);
  while (!x.empty() && std::isspace(x.back())) x.remove_suffix(1);
  return x;
}

std::string_view Concat(std::string_view a, std::string_view b) {
  // There's really no generic message we can give here, since it really
  // depends on the exact expression.
  if (a.data() + a.size() != b.data())
    throw ExpressionParseError("Cannot parse expression (near `{}` or `{}`)", a,
                               b);
  return std::string_view(a.data(), b.data() - a.data() + b.size());
}

std::string_view Concat(std::string_view a, std::string_view b,
                        std::string_view c) {
  return Concat(a, Concat(b, c));
}

std::optional<ExpressionProgram::SingleArgFunction> ParseSingleArgFunction(
    std::string_view name) {
  if (name == "abs") return ExpressionProgram::SingleArgFunction::kAbs;
  return std::nullopt;
}

std::optional<ExpressionProgram::MultiArgFunction> ParseMultiArgFunction(
    std::string_view name) {
  if (name == "min") return ExpressionProgram::MultiArgFunction::kMin;
  if (name == "max") return ExpressionProgram::MultiArgFunction::kMax;
  return std::nullopt;
}

}  // namespace

ExpressionProgram::ExpressionProgram(std::string_view expression)
    : expression_(expression) {}

absl::int128 ExpressionProgram::Evaluate(const LookupFn& fn) const {
  return EvaluateNode(root_index_, fn);
}

std::string_view ExpressionProgram::ExpressionString() const {
  return expression_;
}

const std::vector<std::string>& ExpressionProgram::Dependencies() const {
  return dependencies_;
}

size_t ExpressionProgram::AddIntegerNode(std::string_view span) {
  absl::int128 value;
  if (!absl::SimpleAtoi(span, &value)) {
    throw ExpressionParseError("Failed to parse integer: {}", span);
  };
  nodes_.push_back({.kind = NodeKind::kInteger,
                    .span = span,
                    .payload = Node::IntegerData{.value = Validate(value)}});
  return nodes_.size() - 1;
}

size_t ExpressionProgram::AddVariableNode(std::string_view span) {
  nodes_.push_back(
      {.kind = NodeKind::kVariable,
       .span = span,
       .payload = Node::VariableData{.name = std::string(Trim(span))}});
  return nodes_.size() - 1;
}

size_t ExpressionProgram::AddUnaryNode(NodeKind kind, size_t child,
                                       std::string_view op_span) {
  nodes_.push_back({.kind = kind,
                    .span = Concat(op_span, NodeSpan(child)),
                    .payload = Node::UnaryData{child}});
  return nodes_.size() - 1;
}

size_t ExpressionProgram::AddBinaryNode(NodeKind kind, size_t lhs, size_t rhs,
                                        std::string_view op_span) {
  nodes_.push_back({.kind = kind,
                    .span = Concat(NodeSpan(lhs), op_span, NodeSpan(rhs)),
                    .payload = Node::BinaryData{lhs, rhs}});
  return nodes_.size() - 1;
}

size_t ExpressionProgram::AddFunctionNode(std::string_view fn_name_span,
                                          std::vector<size_t> args,
                                          std::string_view total_span) {
  if (fn_name_span.data() != total_span.data())
    throw ExpressionParseError(
        "[Internal error]: function name and arguments mismatch");
  std::string_view x = Trim(fn_name_span);
  if (!x.empty() && x.back() == '(') x.remove_suffix(1);
  std::string_view fn_name = Trim(x);

  Node::Payload payload;
  if (auto single = ParseSingleArgFunction(fn_name)) {
    if (args.size() != 1) {
      throw ExpressionParseError(
          "{}() expects exactly one argument, received {}", fn_name,
          args.size());
    }
    nodes_.push_back({
        .kind = NodeKind::kFunction,
        .span = total_span,
        .payload =
            Node::SingleArgFunctionData{
                .function = *single,
                .argument = args.front(),
            },
    });
  } else if (auto multi = ParseMultiArgFunction(fn_name)) {
    if (args.empty()) {
      throw ExpressionParseError("{}() expects at least one argument", fn_name);
    }
    nodes_.push_back({
        .kind = NodeKind::kFunction,
        .span = total_span,
        .payload =
            Node::MultiArgFunctionData{
                .function = *multi,
                .arguments = std::move(args),
            },
    });
  } else {
    throw ExpressionParseError("Unknown function: {}", fn_name);
  }

  return nodes_.size() - 1;
}

void ExpressionProgram::Finalize(size_t root_index) {
  root_index_ = root_index;
  dependencies_.clear();
  for (const Node& node : nodes_) {
    if (node.kind == NodeKind::kVariable) {
      dependencies_.push_back(std::get<Node::VariableData>(node.payload).name);
    }
  }
  std::ranges::sort(dependencies_);
  auto it = std::ranges::unique(dependencies_);
  dependencies_.erase(std::ranges::begin(it), std::ranges::end(dependencies_));
}

std::string_view ExpressionProgram::NodeSpan(size_t index) const {
  return nodes_[index].span;
}

void ExpressionProgram::UpdateSpan(size_t index, std::string_view span) {
  nodes_[index].span = span;
}

absl::int128 ExpressionProgram::EvaluateNode(size_t index,
                                             const LookupFn& fn) const {
  const Node& node = nodes_[index];
  switch (node.kind) {
    case NodeKind::kInteger: {
      const auto& data = std::get<Node::IntegerData>(node.payload);
      return data.value;
    }
    case NodeKind::kVariable: {
      const auto& data = std::get<Node::VariableData>(node.payload);
      return fn(data.name);
    }
    case NodeKind::kBinaryAdd: {
      const auto& data = std::get<Node::BinaryData>(node.payload);
      return Validate(EvaluateNode(data.lhs, fn) + EvaluateNode(data.rhs, fn));
    }
    case NodeKind::kBinarySubtract: {
      const auto& data = std::get<Node::BinaryData>(node.payload);
      return Validate(EvaluateNode(data.lhs, fn) - EvaluateNode(data.rhs, fn));
    }
    case NodeKind::kBinaryMultiply: {
      const auto& data = std::get<Node::BinaryData>(node.payload);
      return Validate(EvaluateNode(data.lhs, fn) * EvaluateNode(data.rhs, fn));
    }
    case NodeKind::kBinaryDivide: {
      const auto& data = std::get<Node::BinaryData>(node.payload);
      return Divide(EvaluateNode(data.lhs, fn), EvaluateNode(data.rhs, fn));
    }
    case NodeKind::kBinaryModulo: {
      const auto& data = std::get<Node::BinaryData>(node.payload);
      return Mod(EvaluateNode(data.lhs, fn), EvaluateNode(data.rhs, fn));
    }
    case NodeKind::kBinaryExponentiate: {
      const auto& data = std::get<Node::BinaryData>(node.payload);
      return Pow(EvaluateNode(data.lhs, fn), EvaluateNode(data.rhs, fn));
    }
    case NodeKind::kUnaryPlus: {
      const auto& data = std::get<Node::UnaryData>(node.payload);
      return EvaluateNode(data.child, fn);
    }
    case NodeKind::kUnaryNegate: {
      const auto& data = std::get<Node::UnaryData>(node.payload);
      return Validate(-EvaluateNode(data.child, fn));
    }
    case NodeKind::kFunction: {
      if (std::holds_alternative<Node::SingleArgFunctionData>(node.payload)) {
        const auto& data = std::get<Node::SingleArgFunctionData>(node.payload);
        absl::int128 value = EvaluateNode(data.argument, fn);
        switch (data.function) {
          case SingleArgFunction::kAbs:
            return Validate(value < 0 ? -value : value);
        }
      } else if (std::holds_alternative<Node::MultiArgFunctionData>(
                     node.payload)) {
        const auto& data = std::get<Node::MultiArgFunctionData>(node.payload);
        std::vector<absl::int128> evaluated_args;
        evaluated_args.reserve(data.arguments.size());
        for (size_t child : data.arguments) {
          evaluated_args.push_back(EvaluateNode(child, fn));
        }
        switch (data.function) {
          case MultiArgFunction::kMin:
            return *std::min_element(evaluated_args.begin(),
                                     evaluated_args.end());
          case MultiArgFunction::kMax:
            return *std::max_element(evaluated_args.begin(),
                                     evaluated_args.end());
        }
      }
      throw ExpressionParseError(
          "[Internal error] Unknown function; constructor should verify.");
    }
  }
  throw ExpressionParseError("[Internal Error] Unknown node kind");
}

namespace {

// ------------------------------------------------------------------------
struct Operator {
  OperatorT op;
  std::string_view str;
};

struct Token {
  TokenT kind;
  std::string_view str;
};

bool IsLeftAssociative(OperatorT op) { return op != OperatorT::kExponentiate; }

int Precedence(OperatorT type) {
  switch (type) {
    case OperatorT::kExponentiate:
      return 30;

    // Lower than exponentiation so -10^6 == -1'000'000
    case OperatorT::kUnaryPlus:
    case OperatorT::kUnaryNegate:
      return 40;

    case OperatorT::kMultiply:
    case OperatorT::kDivide:
    case OperatorT::kModulo:
      return 50;

    case OperatorT::kAdd:
    case OperatorT::kSubtract:
      return 60;

    // Scope operators should have the highest precedence so that everything
    // inside the scope is evaluated first. Within them, commas < brackets <
    // EOS
    case OperatorT::kCommaScope:
      return 10010;

    case OperatorT::kOpenParenScope:
    case OperatorT::kFunctionStartScope:
      return 10020;

    case OperatorT::kStartExpressionScope:
      return 10030;
  }
  throw ExpressionParseError("[Internal Error] Unknown operator");
}

bool IsScopeOperator(OperatorT op) {
  return op == OperatorT::kStartExpressionScope ||
         op == OperatorT::kOpenParenScope ||
         op == OperatorT::kFunctionStartScope || op == OperatorT::kCommaScope;
}

std::optional<int> CloseScopePrecedence(TokenT token) {
  switch (token) {
    case TokenT::kComma:
      return Precedence(OperatorT::kCommaScope);
    case TokenT::kCloseParen:
      return Precedence(OperatorT::kOpenParenScope);
    case TokenT::kEndOfExpression:
      return Precedence(OperatorT::kStartExpressionScope);
    default:
      return std::nullopt;
  }
}

void NewApplyOperation(Operator op, ExpressionProgram& program,
                       std::stack<size_t>& operands) {
  auto binary_op = [&](ExpressionProgram::NodeKind kind) {
    if (operands.size() < 2) {
      throw ExpressionParseError(
          "Attempting to do a binary operation, but I don't have 2 "
          "operands.");
    }
    size_t rhs = operands.top();
    operands.pop();
    size_t lhs = operands.top();
    operands.pop();

    operands.push(program.AddBinaryNode(kind, lhs, rhs, op.str));
  };
  auto unary_op = [&](ExpressionProgram::NodeKind kind) {
    if (operands.empty()) {
      throw ExpressionParseError(
          "Attempting to do a unary operation, but I don't have an "
          "operand.");
    }
    size_t rhs = operands.top();
    operands.pop();

    operands.push(program.AddUnaryNode(kind, rhs, op.str));
  };

  switch (op.op) {
    case OperatorT::kAdd:
      binary_op(ExpressionProgram::NodeKind::kBinaryAdd);
      break;
    case OperatorT::kSubtract:
      binary_op(ExpressionProgram::NodeKind::kBinarySubtract);
      break;
    case OperatorT::kMultiply:
      binary_op(ExpressionProgram::NodeKind::kBinaryMultiply);
      break;
    case OperatorT::kDivide:
      binary_op(ExpressionProgram::NodeKind::kBinaryDivide);
      break;
    case OperatorT::kModulo:
      binary_op(ExpressionProgram::NodeKind::kBinaryModulo);
      break;
    case OperatorT::kExponentiate:
      binary_op(ExpressionProgram::NodeKind::kBinaryExponentiate);
      break;
    case OperatorT::kUnaryPlus:
      unary_op(ExpressionProgram::NodeKind::kUnaryPlus);
      break;
    case OperatorT::kUnaryNegate:
      unary_op(ExpressionProgram::NodeKind::kUnaryNegate);
      break;
    default:
      throw ExpressionParseError(
          "[Internal Error] Attempting to apply invalid operator. Minimal "
          "context available: {}::{}",
          op.str, int(op.op));
  }
}

void CollapseScope(std::stack<Operator>& operators,
                   std::stack<size_t>& operands, ExpressionProgram& program) {
  while (!operators.empty() && !IsScopeOperator(operators.top().op)) {
    NewApplyOperation(operators.top(), program, operands);
    operators.pop();
  }
}

std::string ParsingErrorMessage(std::string_view full_expression,
                                std::string_view current_expression,
                                std::string_view error) {
  return std::format("Near index {}, Error:\n{}",
                     current_expression.data() - full_expression.data(), error);
}

void PushScopeToken(Token token, std::stack<Operator>& operators,
                    std::stack<size_t>& operands, ExpressionProgram& program) {
  auto close_scope_precedence = CloseScopePrecedence(token.kind);

  if (!close_scope_precedence) {
    throw ExpressionParseError(
        "[Internal Error] PushScopeToken called without a scope token");
  }

  CollapseScope(operators, operands, program);

  // This means we have an empty string in some substring scope.
  // E.g., "", "()", "max(4,,5)", etc
  if (operands.empty())
    throw ExpressionParseError("No tokens to parse inside (sub)expression");
  if (token.kind == TokenT::kComma) {
    operators.push({OperatorT::kCommaScope, token.str});
    return;
  }

  std::vector<size_t> args;
  std::string_view arg_str = token.str;

  while (!operators.empty() && !operands.empty() &&
         operators.top().op == OperatorT::kCommaScope) {
    arg_str =
        Concat(operators.top().str, program.NodeSpan(operands.top()), arg_str);
    args.push_back(operands.top());
    operands.pop();
    operators.pop();
  }

  if (operators.empty() ||
      *close_scope_precedence != Precedence(operators.top().op)) {
    if (token.kind == TokenT::kEndOfExpression) {
      throw ExpressionParseError(
          "Unexpected end-of-expression. Probably an extra '(' or ','");
    } else {
      throw ExpressionParseError("')' is missing a corresponding '('");
    }
  }
  if (operands.empty())
    throw ExpressionParseError("No tokens to parse inside (sub)expression");

  Operator op = operators.top();
  arg_str = Concat(op.str, program.NodeSpan(operands.top()), arg_str);
  args.push_back(operands.top());
  operands.pop();
  operators.pop();

  if (op.op == OperatorT::kFunctionStartScope) {
    std::reverse(args.begin(), args.end());
    operands.push(program.AddFunctionNode(op.str, std::move(args), arg_str));
    return;
  }

  if (args.size() != 1)
    throw ExpressionParseError("Invalid parentheses: {}", arg_str);
  program.UpdateSpan(args[0], arg_str);
  operands.push(args[0]);
}

void PushToken(Token token, std::stack<Operator>& operators,
               std::stack<size_t>& operands, ExpressionProgram& program) {
  if (auto close = CloseScopePrecedence(token.kind); close.has_value()) {
    PushScopeToken(token, operators, operands, program);
    return;
  }

  if (token.kind == TokenT::kInteger) {
    operands.push(program.AddIntegerNode(token.str));
    return;
  }

  if (token.kind == TokenT::kVariable) {
    operands.push(program.AddVariableNode(token.str));
    return;
  }

  // For tokens and operators that have a 1-to-1 mapping
  auto token_to_operator = [](TokenT kind) {
    switch (kind) {
      case TokenT::kAdd:
        return OperatorT::kAdd;
      case TokenT::kSubtract:
        return OperatorT::kSubtract;
      case TokenT::kMultiply:
        return OperatorT::kMultiply;
      case TokenT::kDivide:
        return OperatorT::kDivide;
      case TokenT::kModulo:
        return OperatorT::kModulo;
      case TokenT::kExponentiate:
        return OperatorT::kExponentiate;
      case TokenT::kUnaryPlus:
        return OperatorT::kUnaryPlus;
      case TokenT::kUnaryNegate:
        return OperatorT::kUnaryNegate;

      case TokenT::kOpenParen:
        return OperatorT::kOpenParenScope;
      case TokenT::kFunctionNameWithParen:
        return OperatorT::kFunctionStartScope;

      default:
        throw ExpressionParseError(
            "[Internal error] Unknown operator in token_to_operator");
    }
  };

  OperatorT op = token_to_operator(token.kind);

  if (!IsScopeOperator(op)) {
    bool is_left = IsLeftAssociative(op);
    int p = Precedence(op);
    while (!operators.empty()) {
      int q = Precedence(operators.top().op);
      if (p < q || (!is_left && p == q)) break;
      NewApplyOperation(operators.top(), program, operands);
      operators.pop();
    }
  }

  operators.push({token_to_operator(token.kind), token.str});
}

bool IsUnaryFollowing(TokenT previous_token) {
  switch (previous_token) {
    case TokenT::kStartOfExpression:
    case TokenT::kOpenParen:
    case TokenT::kFunctionNameWithParen:
    case TokenT::kComma:
    case TokenT::kAdd:
    case TokenT::kSubtract:
    case TokenT::kMultiply:
    case TokenT::kDivide:
    case TokenT::kModulo:
    case TokenT::kExponentiate:
      return true;
    case TokenT::kInteger:
    case TokenT::kVariable:
    case TokenT::kCloseParen:
      return false;
    case TokenT::kUnaryNegate:
    case TokenT::kUnaryPlus:
      throw ExpressionParseError(
          "Error in expression. Found a unary operator after another unary "
          "operator. --3 is not interpreted as -(-3). Note that `x--3` will "
          "work [x - (-3)], but `(--3)` will not.");
    case TokenT::kEndOfExpression:
      throw ExpressionParseError(
          "[Internal Error] IsUnaryFollowing called with kEndOfExpression");
  }
  throw ExpressionParseError("[Internal Error] Unknown token type");
}

// Reads the first token in expression. That token is returned, and the new
// unprocessed prefix.
std::pair<TokenT, std::string_view> NewConsumeFirstToken(
    std::string_view expression, TokenT previous_token) {
  TrimLeadingWhitespace(expression);
  if (expression.empty()) return {TokenT::kEndOfExpression, expression};

  char current = expression.front();
  expression.remove_prefix(1);

  // FIXME: Check if this handles () properly.
  if (current == '(') return {TokenT::kOpenParen, expression};
  if (current == ')') return {TokenT::kCloseParen, expression};
  if (current == ',') return {TokenT::kComma, expression};

  if (current == '*') return {TokenT::kMultiply, expression};
  if (current == '/') return {TokenT::kDivide, expression};
  if (current == '%') return {TokenT::kModulo, expression};
  if (current == '^') return {TokenT::kExponentiate, expression};

  if (current == '+' || current == '-') {
    bool unary = IsUnaryFollowing(previous_token);
    if (current == '+' && unary) return {TokenT::kUnaryPlus, expression};
    if (current == '+' && !unary) return {TokenT::kAdd, expression};
    if (current == '-' && unary) return {TokenT::kUnaryNegate, expression};
    if (current == '-' && !unary) return {TokenT::kSubtract, expression};
  }

  // Check for integer
  if (std::isdigit(current)) {
    while (!expression.empty() && std::isdigit(expression.front())) {
      expression.remove_prefix(1);
    }
    return {TokenT::kInteger, expression};
  }

  // Check for variable ([A-Za-z][A-Za-z0-9_]*)
  if (std::isalpha(current)) {
    auto valid_char = [](char c) {
      return std::isalpha(c) || std::isdigit(c) || c == '_';
    };
    while (!expression.empty() && valid_char(expression.front())) {
      expression.remove_prefix(1);
    }
    // Check if the next character is a open parenthesis.
    TrimLeadingWhitespace(expression);
    if (!expression.empty() && expression.front() == '(') {
      expression.remove_prefix(1);
      return {TokenT::kFunctionNameWithParen, expression};
    }

    return {TokenT::kVariable, expression};
  }

  throw ExpressionParseError("Unknown character in expression: '{}'", current);
}

}  // namespace

// TODO: Add support for several features:
//  * functions (such as clamp(n, lo, hi), sum(A))
//  * comparison operators (<, >, <=, >=)
//  * equality operators (==, !=)
//  * boolean operators (and, or, xor, not)
//  * postfix unary operators (such as n!)
std::shared_ptr<const ExpressionProgram> ExpressionProgram::Parse(
    std::string_view expression) {
  std::shared_ptr<ExpressionProgram> program(new ExpressionProgram(expression));

  std::string_view remaining = program->ExpressionString();
  std::string_view original_expression = remaining;

  std::stack<Operator> operators;
  operators.push({OperatorT::kStartExpressionScope, remaining.substr(0, 0)});
  std::stack<size_t> operands;

  auto get_token_str = [&](std::string_view prev_suff,
                           std::string_view curr_suff) -> std::string_view {
    return prev_suff.substr(0, prev_suff.size() - curr_suff.size());
  };

  TokenT prev = TokenT::kStartOfExpression;
  while (prev != TokenT::kEndOfExpression) {
    try {
      auto [token_kind, new_suffix] = NewConsumeFirstToken(remaining, prev);
      Token token = {token_kind, get_token_str(remaining, new_suffix)};
      PushToken(token, operators, operands, *program);
      remaining = new_suffix;
      prev = token_kind;
    } catch (const ExpressionParseError& e) {
      throw ExpressionParseError(
          ExpressionParseError::ExpressionTag{}, original_expression,
          ParsingErrorMessage(original_expression, remaining, e.what()));
    } catch (const ExpressionEvaluationError& e) {
      throw ExpressionParseError(
          ExpressionParseError::ExpressionTag{}, original_expression,
          ParsingErrorMessage(original_expression, remaining, e.what()));
    }
  }

  if (operands.size() != 1 || !operators.empty()) {
    throw ExpressionParseError(
        "[Internal Error] Expression does not parse properly, but should "
        "have been caught by another exception.");
  }
  if (program->NodeSpan(operands.top()) != original_expression) {
    throw ExpressionParseError(
        "[Internal Error] Expression span does not match the original "
        "string.");
  }

  program->Finalize(operands.top());
  return program;
}

}  // namespace moriarty_internal

std::vector<std::string> Expression::GetDependencies() const {
  return program_->Dependencies();
}

std::string Expression::ToString() const {
  return std::string(program_->ExpressionString());
}

int64_t Expression::Evaluate(
    std::function<int64_t(std::string_view)> lookup_variable) const {
  return static_cast<int64_t>(program_->Evaluate(lookup_variable));
}

int64_t Expression::Evaluate() const {
  auto missing = [](std::string_view) -> int64_t {
    throw ExpressionEvaluationError("Variable not found");
  };
  return static_cast<int64_t>(program_->Evaluate(missing));
}

Expression::Expression(std::string_view str) {
  program_ = moriarty_internal::ExpressionProgram::Parse(str);
}

bool Expression::operator==(const Expression& other) const {
  return program_->ExpressionString() == other.program_->ExpressionString();
}
}  // namespace moriarty
