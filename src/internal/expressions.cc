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
#include <exception>
#include <format>
#include <limits>
#include <memory>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "absl/numeric/int128.h"
#include "absl/strings/numbers.h"

namespace moriarty {
namespace moriarty_internal {

class IntegerLiteralNode : public ExpressionNode {
 public:
  ~IntegerLiteralNode() override = default;
  explicit IntegerLiteralNode(std::string_view op_str);
  [[nodiscard]] absl::int128 Evaluate(const LookupFn& fn) const override;

 private:
  absl::int128 value_;
};

class VariableLiteralNode : public ExpressionNode {
 public:
  ~VariableLiteralNode() override = default;
  explicit VariableLiteralNode(std::string variable, std::string_view op_str);
  [[nodiscard]] absl::int128 Evaluate(const LookupFn& fn) const override;

 private:
  std::string variable_;
};

class BinaryAdditionNode : public ExpressionNode {
 public:
  ~BinaryAdditionNode() override = default;
  explicit BinaryAdditionNode(std::unique_ptr<ExpressionNode> lhs,
                              std::unique_ptr<ExpressionNode> rhs,
                              std::string_view op_str);
  [[nodiscard]] absl::int128 Evaluate(const LookupFn& fn) const override;

 private:
  std::unique_ptr<ExpressionNode> lhs_;
  std::unique_ptr<ExpressionNode> rhs_;
};

class BinarySubtractionNode : public ExpressionNode {
 public:
  ~BinarySubtractionNode() override = default;
  explicit BinarySubtractionNode(std::unique_ptr<ExpressionNode> lhs,
                                 std::unique_ptr<ExpressionNode> rhs,
                                 std::string_view op_str);
  [[nodiscard]] absl::int128 Evaluate(const LookupFn& fn) const override;

 private:
  std::unique_ptr<ExpressionNode> lhs_;
  std::unique_ptr<ExpressionNode> rhs_;
};

class BinaryMultiplicationNode : public ExpressionNode {
 public:
  ~BinaryMultiplicationNode() override = default;
  explicit BinaryMultiplicationNode(std::unique_ptr<ExpressionNode> lhs,
                                    std::unique_ptr<ExpressionNode> rhs,
                                    std::string_view op_str);
  [[nodiscard]] absl::int128 Evaluate(const LookupFn& fn) const override;

 private:
  std::unique_ptr<ExpressionNode> lhs_;
  std::unique_ptr<ExpressionNode> rhs_;
};

class BinaryDivisionNode : public ExpressionNode {
 public:
  ~BinaryDivisionNode() override = default;
  explicit BinaryDivisionNode(std::unique_ptr<ExpressionNode> lhs,
                              std::unique_ptr<ExpressionNode> rhs,
                              std::string_view op_str);
  [[nodiscard]] absl::int128 Evaluate(const LookupFn& fn) const override;

 private:
  std::unique_ptr<ExpressionNode> lhs_;
  std::unique_ptr<ExpressionNode> rhs_;
};

class BinaryModuloNode : public ExpressionNode {
 public:
  ~BinaryModuloNode() override = default;
  explicit BinaryModuloNode(std::unique_ptr<ExpressionNode> lhs,
                            std::unique_ptr<ExpressionNode> rhs,
                            std::string_view op_str);
  [[nodiscard]] absl::int128 Evaluate(const LookupFn& fn) const override;

 private:
  std::unique_ptr<ExpressionNode> lhs_;
  std::unique_ptr<ExpressionNode> rhs_;
};

class BinaryExponentiationNode : public ExpressionNode {
 public:
  ~BinaryExponentiationNode() override = default;
  explicit BinaryExponentiationNode(std::unique_ptr<ExpressionNode> lhs,
                                    std::unique_ptr<ExpressionNode> rhs,
                                    std::string_view op_str);
  [[nodiscard]] absl::int128 Evaluate(const LookupFn& fn) const override;

 private:
  std::unique_ptr<ExpressionNode> lhs_;
  std::unique_ptr<ExpressionNode> rhs_;
};

class UnaryPlusNode : public ExpressionNode {
 public:
  ~UnaryPlusNode() override = default;
  explicit UnaryPlusNode(std::unique_ptr<ExpressionNode> rhs,
                         std::string_view op_str);
  [[nodiscard]] absl::int128 Evaluate(const LookupFn& fn) const override;

 private:
  std::unique_ptr<ExpressionNode> rhs_;
};

class UnaryNegateNode : public ExpressionNode {
 public:
  ~UnaryNegateNode() override = default;
  explicit UnaryNegateNode(std::unique_ptr<ExpressionNode> rhs,
                           std::string_view op_str);
  [[nodiscard]] absl::int128 Evaluate(const LookupFn& fn) const override;

 private:
  std::unique_ptr<ExpressionNode> rhs_;
};

class FunctionNode : public ExpressionNode {
 public:
  ~FunctionNode() override = default;
  explicit FunctionNode(std::string_view fn_name_str,
                        std::vector<std::unique_ptr<ExpressionNode>> arguments,
                        std::string_view arg_str);
  [[nodiscard]] absl::int128 Evaluate(const LookupFn& fn) const override;

 private:
  std::string name_;  // FIXME: Make this a function pointer.
  std::vector<std::unique_ptr<ExpressionNode>> arguments_;
};

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
    throw std::overflow_error("Expression overflows int64_t");
  }
  return value;
}

absl::int128 Divide(absl::int128 lhs, absl::int128 rhs) {
  if (rhs == 0) throw std::invalid_argument("Division by zero in expression");
  return Validate(absl::int128(lhs) / rhs);
}

absl::int128 Mod(absl::int128 lhs, absl::int128 rhs) {
  if (rhs == 0) throw std::invalid_argument("Mod by zero in expression");
  return Validate(absl::int128(lhs) % rhs);
}

absl::int128 Pow(absl::int128 base, absl::int128 exponent) {
  if (exponent < 0)
    throw std::invalid_argument("exponent must be non-negative in pow()");
  if (base == 0 && exponent == 0)
    throw std::invalid_argument("0 to the power of 0 is undefined.");

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
    throw std::invalid_argument(
        std::format("Cannot parse expression (near `{}` or `{}`)", a, b));
  return std::string_view(a.data(), b.data() - a.data() + b.size());
}

std::string_view Concat(std::string_view a, std::string_view b,
                        std::string_view c) {
  return Concat(a, Concat(b, c));
}

void ExpressionNode::SetString(std::string_view new_value) { str_ = new_value; }

void ExpressionNode::AddDependencies(std::vector<std::string>&& dependencies) {
  dependencies_.insert(dependencies_.end(), dependencies.begin(),
                       dependencies.end());
  std::ranges::sort(dependencies_);
  auto it = std::unique(dependencies_.begin(), dependencies_.end());
  dependencies_.erase(it, dependencies_.end());
}

std::vector<std::string> ExpressionNode::ReleaseDependencies() {
  std::vector<std::string> deps = std::move(dependencies_);
  dependencies_.clear();
  return deps;
}

std::string_view ExpressionNode::ToString() const { return str_; }

ExpressionNode::ExpressionNode(std::string_view str) : str_(str) {}

IntegerLiteralNode::IntegerLiteralNode(std::string_view str)
    : ExpressionNode(str) {
  if (!absl::SimpleAtoi(str, &value_)) {
    throw std::invalid_argument(
        std::format("Failed to parse integer: {}", str));
  }
  value_ = Validate(value_);
}

absl::int128 IntegerLiteralNode::Evaluate(const LookupFn& fn) const {
  return value_;
}

VariableLiteralNode::VariableLiteralNode(std::string variable,
                                         std::string_view str)
    : ExpressionNode(str), variable_(std::move(variable)) {
  AddDependencies({variable_});
}

absl::int128 VariableLiteralNode::Evaluate(const LookupFn& fn) const {
  return fn(variable_);
}

BinaryAdditionNode::BinaryAdditionNode(std::unique_ptr<ExpressionNode> lhs,
                                       std::unique_ptr<ExpressionNode> rhs,
                                       std::string_view op_str)
    : ExpressionNode(Concat(lhs->ToString(), op_str, rhs->ToString())),
      lhs_(std::move(lhs)),
      rhs_(std::move(rhs)) {
  AddDependencies(lhs_->ReleaseDependencies());
  AddDependencies(rhs_->ReleaseDependencies());
}

absl::int128 BinaryAdditionNode::Evaluate(const LookupFn& fn) const {
  return Validate(lhs_->Evaluate(fn) + rhs_->Evaluate(fn));
}

BinarySubtractionNode::BinarySubtractionNode(
    std::unique_ptr<ExpressionNode> lhs, std::unique_ptr<ExpressionNode> rhs,
    std::string_view op_str)
    : ExpressionNode(Concat(lhs->ToString(), op_str, rhs->ToString())),
      lhs_(std::move(lhs)),
      rhs_(std::move(rhs)) {
  AddDependencies(lhs_->ReleaseDependencies());
  AddDependencies(rhs_->ReleaseDependencies());
}

absl::int128 BinarySubtractionNode::Evaluate(const LookupFn& fn) const {
  return Validate(lhs_->Evaluate(fn) - rhs_->Evaluate(fn));
}

BinaryMultiplicationNode::BinaryMultiplicationNode(
    std::unique_ptr<ExpressionNode> lhs, std::unique_ptr<ExpressionNode> rhs,
    std::string_view op_str)
    : ExpressionNode(Concat(lhs->ToString(), op_str, rhs->ToString())),
      lhs_(std::move(lhs)),
      rhs_(std::move(rhs)) {
  AddDependencies(lhs_->ReleaseDependencies());
  AddDependencies(rhs_->ReleaseDependencies());
}

absl::int128 BinaryMultiplicationNode::Evaluate(const LookupFn& fn) const {
  return Validate(lhs_->Evaluate(fn) * rhs_->Evaluate(fn));
}

BinaryDivisionNode::BinaryDivisionNode(std::unique_ptr<ExpressionNode> lhs,
                                       std::unique_ptr<ExpressionNode> rhs,
                                       std::string_view op_str)
    : ExpressionNode(Concat(lhs->ToString(), op_str, rhs->ToString())),
      lhs_(std::move(lhs)),
      rhs_(std::move(rhs)) {
  AddDependencies(lhs_->ReleaseDependencies());
  AddDependencies(rhs_->ReleaseDependencies());
}

absl::int128 BinaryDivisionNode::Evaluate(const LookupFn& fn) const {
  return Divide(lhs_->Evaluate(fn), rhs_->Evaluate(fn));
}

BinaryModuloNode::BinaryModuloNode(std::unique_ptr<ExpressionNode> lhs,
                                   std::unique_ptr<ExpressionNode> rhs,
                                   std::string_view op_str)
    : ExpressionNode(Concat(lhs->ToString(), op_str, rhs->ToString())),
      lhs_(std::move(lhs)),
      rhs_(std::move(rhs)) {
  AddDependencies(lhs_->ReleaseDependencies());
  AddDependencies(rhs_->ReleaseDependencies());
}

absl::int128 BinaryModuloNode::Evaluate(const LookupFn& fn) const {
  return Mod(lhs_->Evaluate(fn), rhs_->Evaluate(fn));
}

BinaryExponentiationNode::BinaryExponentiationNode(
    std::unique_ptr<ExpressionNode> lhs, std::unique_ptr<ExpressionNode> rhs,
    std::string_view op_str)
    : ExpressionNode(Concat(lhs->ToString(), op_str, rhs->ToString())),
      lhs_(std::move(lhs)),
      rhs_(std::move(rhs)) {
  AddDependencies(lhs_->ReleaseDependencies());
  AddDependencies(rhs_->ReleaseDependencies());
}

absl::int128 BinaryExponentiationNode::Evaluate(const LookupFn& fn) const {
  return Pow(lhs_->Evaluate(fn), rhs_->Evaluate(fn));
}

UnaryPlusNode::UnaryPlusNode(std::unique_ptr<ExpressionNode> rhs,
                             std::string_view op_str)
    : ExpressionNode(Concat(op_str, rhs->ToString())), rhs_(std::move(rhs)) {
  AddDependencies(rhs_->ReleaseDependencies());
}

absl::int128 UnaryPlusNode::Evaluate(const LookupFn& fn) const {
  return rhs_->Evaluate(fn);
}

UnaryNegateNode::UnaryNegateNode(std::unique_ptr<ExpressionNode> rhs,
                                 std::string_view op_str)
    : ExpressionNode(Concat(op_str, rhs->ToString())), rhs_(std::move(rhs)) {
  AddDependencies(rhs_->ReleaseDependencies());
}

absl::int128 UnaryNegateNode::Evaluate(const LookupFn& fn) const {
  return Validate(-rhs_->Evaluate(fn));
}

FunctionNode::FunctionNode(
    std::string_view fn_name_str,
    std::vector<std::unique_ptr<ExpressionNode>> arguments,
    std::string_view args_str)
    : ExpressionNode(args_str), arguments_(std::move(arguments)) {
  if (fn_name_str.data() != args_str.data())
    throw std::invalid_argument(
        "[Internal error]: function name and arguments mismatch");

  for (const auto& arg : arguments_)
    AddDependencies(arg->ReleaseDependencies());

  std::string_view x = Trim(fn_name_str);
  if (!x.empty() && x.back() == '(') x.remove_suffix(1);
  name_ = std::string(Trim(x));

  if (name_ == "max") return;  // Nothing to validate
  if (name_ == "min") return;  // Nothing to validate
  if (name_ == "abs") {
    if (arguments_.size() != 1) {
      throw std::invalid_argument(
          std::format("abs() expects exactly one argument, receieved {}",
                      arguments_.size()));
    }
    return;
  }

  throw std::invalid_argument(std::format("Unknown function: {}", name_));
}

absl::int128 FunctionNode::Evaluate(const LookupFn& fn) const {
  std::vector<absl::int128> evaluated_args;
  for (const auto& arg : arguments_) {
    evaluated_args.push_back(arg->Evaluate(fn));
  }

  // Example function implementations
  if (name_ == "min")
    return *std::min_element(evaluated_args.begin(), evaluated_args.end());

  if (name_ == "max")
    return *std::max_element(evaluated_args.begin(), evaluated_args.end());

  if (name_ == "abs") return std::abs(int64_t(evaluated_args[0]));

  throw std::invalid_argument(
      "[Internal error] Unknown function; the constructor should have caught "
      "this: " +
      name_);
}

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
    // inside the scope is evaluated first. Within them, commas < brackets < EOS
    case OperatorT::kCommaScope:
      return 10010;

    case OperatorT::kOpenParenScope:
    case OperatorT::kFunctionStartScope:
      return 10020;

    case OperatorT::kStartExpressionScope:
      return 10030;
  }
  throw std::runtime_error("[Internal Error] Unknown operator");
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

void NewApplyOperation(Operator op,
                       std::stack<std::unique_ptr<ExpressionNode>>& operands) {
  auto binary_op = [&]() -> std::pair<std::unique_ptr<ExpressionNode>,
                                      std::unique_ptr<ExpressionNode>> {
    if (operands.size() < 2) {
      throw std::runtime_error(
          "Attempting to do a binary operation, but I don't have 2 operands.");
    }
    auto rhs = std::move(operands.top());
    operands.pop();
    auto lhs = std::move(operands.top());
    operands.pop();
    return {std::move(lhs), std::move(rhs)};
  };
  auto unary_op = [&]() -> std::unique_ptr<ExpressionNode> {
    if (operands.size() < 1) {
      throw std::runtime_error(
          "Attempting to do a unary operation, but I don't have 2 operands.");
    }
    auto rhs = std::move(operands.top());
    operands.pop();
    return rhs;
  };

  switch (op.op) {
    case OperatorT::kAdd: {
      auto [lhs, rhs] = binary_op();
      operands.push(std::make_unique<BinaryAdditionNode>(
          std::move(lhs), std::move(rhs), op.str));
      break;
    }
    case OperatorT::kSubtract: {
      auto [lhs, rhs] = binary_op();
      operands.push(std::make_unique<BinarySubtractionNode>(
          std::move(lhs), std::move(rhs), op.str));
      break;
    }
    case OperatorT::kMultiply: {
      auto [lhs, rhs] = binary_op();
      operands.push(std::make_unique<BinaryMultiplicationNode>(
          std::move(lhs), std::move(rhs), op.str));
      break;
    }
    case OperatorT::kDivide: {
      auto [lhs, rhs] = binary_op();
      operands.push(std::make_unique<BinaryDivisionNode>(
          std::move(lhs), std::move(rhs), op.str));
      break;
    }
    case OperatorT::kModulo: {
      auto [lhs, rhs] = binary_op();
      operands.push(std::make_unique<BinaryModuloNode>(std::move(lhs),
                                                       std::move(rhs), op.str));
      break;
    }
    case OperatorT::kExponentiate: {
      auto [lhs, rhs] = binary_op();
      operands.push(std::make_unique<BinaryExponentiationNode>(
          std::move(lhs), std::move(rhs), op.str));
      break;
    }
    case OperatorT::kUnaryPlus: {
      auto rhs = unary_op();
      operands.push(std::make_unique<UnaryPlusNode>(std::move(rhs), op.str));
      break;
    }
    case OperatorT::kUnaryNegate: {
      auto rhs = unary_op();
      operands.push(std::make_unique<UnaryNegateNode>(std::move(rhs), op.str));
      break;
    }
    default:
      throw std::runtime_error(std::format(
          "[Internal Error] Attempting to apply invalid operator. Minimal "
          "context available: {}::{}",
          op.str, int(op.op)));
  }
}

void CollapseScope(std::stack<Operator>& operators,
                   std::stack<std::unique_ptr<ExpressionNode>>& operands) {
  while (!operators.empty() && !IsScopeOperator(operators.top().op)) {
    NewApplyOperation(operators.top(), operands);
    operators.pop();
  }
}

std::string ParsingErrorMessage(std::string_view full_expression,
                                std::string_view current_expression,
                                std::string_view error) {
  return std::format("Error while parsing expression near index {}: {}\n{}",
                     current_expression.data() - full_expression.data(),
                     full_expression, error);
}

void PushScopeToken(Token token, std::stack<Operator>& operators,
                    std::stack<std::unique_ptr<ExpressionNode>>& operands) {
  auto close_scope_precedence = CloseScopePrecedence(token.kind);

  if (!close_scope_precedence) {
    throw std::runtime_error(
        "[Internal Error] PushScopeToken called without a scope token");
  }

  CollapseScope(operators, operands);

  // This means we have an empty string in some substring scope.
  // E.g., "", "()", "max(4,,5)", etc
  if (operands.empty())
    throw std::runtime_error("No tokens to parse inside (sub)expression");

  if (token.kind == TokenT::kComma) {
    operators.push({OperatorT::kCommaScope, token.str});
    return;
  }

  std::vector<std::unique_ptr<ExpressionNode>> args;
  std::string_view arg_str = token.str;

  while (!operators.empty() && !operands.empty() &&
         operators.top().op == OperatorT::kCommaScope) {
    arg_str = Concat(operators.top().str, operands.top()->ToString(), arg_str);
    args.push_back(std::move(operands.top()));
    operands.pop();
    operators.pop();
  }

  if (operators.empty() ||
      *close_scope_precedence != Precedence(operators.top().op)) {
    if (token.kind == TokenT::kEndOfExpression) {
      throw std::runtime_error(
          "Unexpected end-of-expression. Probably an extra '(' or ','");
    } else {
      throw std::runtime_error("')' is missing a corresponding '('");
    }
  }
  if (operands.empty())
    throw std::runtime_error("No tokens to parse inside (sub)expression");

  Operator op = operators.top();
  arg_str = Concat(op.str, operands.top()->ToString(), arg_str);
  args.push_back(std::move(operands.top()));
  operands.pop();
  operators.pop();

  if (op.op == OperatorT::kFunctionStartScope) {
    std::reverse(args.begin(), args.end());
    operands.push(
        std::make_unique<FunctionNode>(op.str, std::move(args), arg_str));
    return;
  }

  if (args.size() != 1)
    throw std::runtime_error(std::format("Invalid parentheses: {}", arg_str));
  args[0]->SetString(arg_str);
  operands.push(std::move(args[0]));
}

void PushToken(Token token, std::stack<Operator>& operators,
               std::stack<std::unique_ptr<ExpressionNode>>& operands) {
  if (auto close = CloseScopePrecedence(token.kind); close.has_value()) {
    PushScopeToken(token, operators, operands);
    return;
  }

  if (token.kind == TokenT::kInteger) {
    operands.push(std::make_unique<IntegerLiteralNode>(token.str));
    return;
  }

  if (token.kind == TokenT::kVariable) {
    operands.push(std::make_unique<VariableLiteralNode>(
        std::string(Trim(token.str)), token.str));
    return;
  }

  // For tokens and operators that have a 1-to-1 mapping
  auto token_to_operator = [](TokenT kind) {
    if (kind == TokenT::kAdd) return OperatorT::kAdd;
    if (kind == TokenT::kSubtract) return OperatorT::kSubtract;
    if (kind == TokenT::kMultiply) return OperatorT::kMultiply;
    if (kind == TokenT::kDivide) return OperatorT::kDivide;
    if (kind == TokenT::kModulo) return OperatorT::kModulo;
    if (kind == TokenT::kExponentiate) return OperatorT::kExponentiate;
    if (kind == TokenT::kUnaryPlus) return OperatorT::kUnaryPlus;
    if (kind == TokenT::kUnaryNegate) return OperatorT::kUnaryNegate;

    if (kind == TokenT::kOpenParen) return OperatorT::kOpenParenScope;
    if (kind == TokenT::kFunctionNameWithParen)
      return OperatorT::kFunctionStartScope;

    throw std::runtime_error(
        "[Internal error] Unknown operator in token_to_operator");
  };

  OperatorT op = token_to_operator(token.kind);

  if (!IsScopeOperator(op)) {
    bool is_left = IsLeftAssociative(op);
    int p = Precedence(op);
    while (!operators.empty()) {
      int q = Precedence(operators.top().op);
      if (p < q || (!is_left && p == q)) break;
      NewApplyOperation(operators.top(), operands);
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
      throw std::runtime_error(
          "Error in expression. Found a unary operator after another unary "
          "operator. --3 is not interpreted as -(-3). Note that `x--3` will "
          "work [x - (-3)], but `(--3)` will not.");
    case TokenT::kEndOfExpression:
      throw std::runtime_error(
          "[Internal Error] IsUnaryFollowing called with kEndOfExpression");
  }
  throw std::runtime_error("[Internal Error] Unknown token type");
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
    std::string variable_name(1, current);
    auto valid_char = [](char c) {
      return std::isalpha(c) || std::isdigit(c) || c == '_';
    };
    while (!expression.empty() && valid_char(expression.front())) {
      variable_name += expression.front();
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

  throw std::runtime_error("[Parse Error] Unknown character in expression");
}

std::unique_ptr<ExpressionNode> ParseExpression(std::string_view expression) {
  // Implements the shunting-yard algorithm
  // TODO(b/208295758): Add support for several features:
  //  * functions (such as min(x, y), abs(x), clamp(n, lo, hi))
  //  * comparison operators (<, >, <=, >=)
  //  * equality operators (==, !=)
  //  * boolean operators (and, or, xor, not)
  //  * postfix unary operators (such as n!)

  std::stack<Operator> operators;
  operators.push({OperatorT::kStartExpressionScope, expression.substr(0, 0)});
  std::stack<std::unique_ptr<ExpressionNode>> operands;

  std::string_view original_expression = expression;

  auto get_token_str = [&](std::string_view prev_suff,
                           std::string_view curr_suff) -> std::string_view {
    return prev_suff.substr(0, prev_suff.size() - curr_suff.size());
  };

  TokenT prev = TokenT::kStartOfExpression;
  while (prev != TokenT::kEndOfExpression) {
    try {
      auto [token_kind, new_suffix] = NewConsumeFirstToken(expression, prev);
      Token token = {token_kind, get_token_str(expression, new_suffix)};
      PushToken(token, operators, operands);
      expression = new_suffix;
      prev = token_kind;
    } catch (std::exception& e) {
      throw std::runtime_error(
          ParsingErrorMessage(original_expression, expression, e.what()));
    }
  }

  if (operands.size() != 1 || !operators.empty()) {
    throw std::runtime_error(
        "[Internal Error] Expression does not parse "
        "properly, but should have been caught by another exception.");
  }
  if (operands.top()->ToString() != original_expression) {
    throw std::runtime_error(
        "[Internal Error] Expression ToString() does not return the original "
        "string.");
  }

  return std::move(operands.top());
}

}  // namespace moriarty_internal

std::vector<std::string> Expression::GetDependencies() const {
  return dependencies_;
}

std::string_view Expression::ToString() const { return str_; }

int64_t Expression::Evaluate(
    std::function<int64_t(std::string_view)> lookup_variable) const {
  return static_cast<int64_t>(expr_->Evaluate(lookup_variable));
}

int64_t Expression::Evaluate() const {
  auto tmp = [](std::string_view s) -> int64_t {
    throw std::invalid_argument("Variable not found");
  };
  return static_cast<int64_t>(expr_->Evaluate(tmp));
}

Expression::Expression(std::string_view str) : str_(str) {
  expr_ = moriarty_internal::ParseExpression(str_);
  dependencies_ = expr_->ReleaseDependencies();
}

}  // namespace moriarty
