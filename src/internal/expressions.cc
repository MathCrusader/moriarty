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
#include <limits>
#include <memory>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/numeric/int128.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/substitute.h"
#include "src/util/status_macro/status_macros.h"

namespace moriarty {

namespace moriarty_internal {
namespace {

using VariableMap = absl::flat_hash_map<std::string, int64_t>;

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

  absl::int128 b = base;
  absl::int128 result = 1;
  while (exponent > 0) {
    if (exponent % 2 == 1) result = Validate(result * b);
    if (exponent > 1) b = Validate(b * b);
    exponent /= 2;
  }
  return result;
}

absl::int128 GetIntegerLiteral(const Literal& literal,
                               const VariableMap& variables) {
  if (!literal.IsVariable()) return literal.Value();

  auto it = variables.find(literal.VariableName());
  if (it != variables.end()) return it->second;

  throw std::invalid_argument(std::format(
      "Variable in expression with unknown value: {}", literal.VariableName()));
}

/* -------------------------------------------------------------------------- */
/*  EVALUATION                                                                */
/* -------------------------------------------------------------------------- */

using LiteralVariant = std::variant<absl::int128, bool>;

// We need this forward declaration since these functions have circular
// dependencies.
LiteralVariant EvaluateExpression(const Expression& expression,
                                  const VariableMap& variables);

LiteralVariant EvaluateBinaryOperation(const BinaryOperation& expr,
                                       const VariableMap& variables) {
  LiteralVariant lhs = EvaluateExpression(expr.Lhs(), variables);
  LiteralVariant rhs = EvaluateExpression(expr.Rhs(), variables);

  // int/int operations
  if (std::get_if<absl::int128>(&lhs) == nullptr ||
      std::get_if<absl::int128>(&rhs) == nullptr) {
    throw std::invalid_argument(
        "Only (int, int) binary operations are defined.");
  }
  absl::int128 lhs_int = std::get<absl::int128>(lhs);
  absl::int128 rhs_int = std::get<absl::int128>(rhs);

  switch (expr.Op()) {
    case BinaryOperator::kAdd:
      return Validate(lhs_int + rhs_int);
    case BinaryOperator::kSubtract:
      return Validate(lhs_int - rhs_int);
    case BinaryOperator::kMultiply:
      return Validate(lhs_int * rhs_int);
    case BinaryOperator::kDivide:
      return Divide(lhs_int, rhs_int);
    case BinaryOperator::kModulo:
      return Mod(lhs_int, rhs_int);
    case BinaryOperator::kExponentiate:
      return Pow(lhs_int, rhs_int);
  }

  throw std::runtime_error("Unknown binary operator in expression");
}

LiteralVariant EvaluateUnaryOperation(const UnaryOperation& expr,
                                      const VariableMap& variables) {
  LiteralVariant rhs = EvaluateExpression(expr.Rhs(), variables);

  // int operations
  if (std::get_if<absl::int128>(&rhs) != nullptr) {
    absl::int128 rhs_int = std::get<absl::int128>(rhs);
    switch (expr.Op()) {
      case UnaryOperator::kPlus:
        return rhs;
      case UnaryOperator::kNegate:
        return Validate(-rhs_int);
    }
  }
  throw std::runtime_error("Unknown unary operator in expression");
}

LiteralVariant EvaluateFunction(const Function& fn,
                                const VariableMap& variables) {
  std::vector<absl::int128> arguments;
  arguments.reserve(fn.Arguments().size());
  for (const std::unique_ptr<Expression>& expr : fn.Arguments()) {
    if (expr == nullptr)
      throw std::runtime_error("Function argument must not be null");
    LiteralVariant res = EvaluateExpression(*expr, variables);
    if (!std::holds_alternative<absl::int128>(res)) {
      throw std::invalid_argument("Function arguments must be integers");
    }
    arguments.push_back(std::get<absl::int128>(res));
  }

  // TODO(b/208295758): Make this extendable.
  if (fn.Name() == "min")
    return *std::min_element(arguments.begin(), arguments.end());
  if (fn.Name() == "max")
    return *std::max_element(arguments.begin(), arguments.end());
  if (fn.Name() == "abs") {
    if (arguments.size() != 1) {
      throw std::invalid_argument("abs(x) can only take one parameter");
    }
    return abs(int64_t(arguments[0]));
  }

  throw std::invalid_argument(
      std::format("Unknown function name ({}) in expression", fn.Name()));
}

LiteralVariant EvaluateExpression(const Expression& expression,
                                  const VariableMap& variables) {
  struct Visitor {
    explicit Visitor(const VariableMap& variables) : variables(variables) {}
    const VariableMap& variables;

    LiteralVariant operator()(const Literal& lit) {
      return GetIntegerLiteral(lit, variables);
    }
    LiteralVariant operator()(const BinaryOperation& binary) {
      return EvaluateBinaryOperation(binary, variables);
    }
    LiteralVariant operator()(const UnaryOperation& unary) {
      return EvaluateUnaryOperation(unary, variables);
    }
    LiteralVariant operator()(const Function& fn) {
      return EvaluateFunction(fn, variables);
    }
  };
  return std::visit(Visitor(variables), expression.Get());
}

/* -------------------------------------------------------------------------- */
/*  REPLACE VARIABLES                                                         */
/* -------------------------------------------------------------------------- */

absl::Status GetUnknownVariables(
    const Literal& literal,
    absl::flat_hash_set<std::string>& unknown_variables) {
  if (literal.IsVariable()) unknown_variables.insert(literal.VariableName());
  return absl::OkStatus();
}

absl::Status GetUnknownVariables(
    const Expression& expression,
    absl::flat_hash_set<std::string>& unknown_variables) {
  struct Visitor {
    explicit Visitor(absl::flat_hash_set<std::string>& unknown_variables)
        : unknown_variables(unknown_variables) {}
    absl::flat_hash_set<std::string>& unknown_variables;

    absl::Status operator()(const Literal& lit) {
      return GetUnknownVariables(lit, unknown_variables);
    }
    absl::Status operator()(const BinaryOperation& binary) {
      MORIARTY_RETURN_IF_ERROR(
          GetUnknownVariables(binary.Lhs(), unknown_variables));
      return GetUnknownVariables(binary.Rhs(), unknown_variables);
    }
    absl::Status operator()(const UnaryOperation& unary) {
      return GetUnknownVariables(unary.Rhs(), unknown_variables);
    }
    absl::Status operator()(const Function& fn) {
      for (const std::unique_ptr<Expression>& arg : fn.Arguments()) {
        if (arg != nullptr) {
          MORIARTY_RETURN_IF_ERROR(
              GetUnknownVariables(*arg, unknown_variables));
        }
      }
      return absl::OkStatus();
    }
  };
  return std::visit(Visitor(unknown_variables), expression.Get());
}

}  // namespace
}  // namespace moriarty_internal

int64_t EvaluateIntegerExpression(const Expression& expression) {
  return EvaluateIntegerExpression(expression,
                                   absl::flat_hash_map<std::string, int64_t>{});
}

int64_t EvaluateIntegerExpression(
    const Expression& expression,
    const absl::flat_hash_map<std::string, int64_t>& variables) {
  moriarty_internal::LiteralVariant value =
      moriarty_internal::EvaluateExpression(expression, variables);

  if (std::get_if<absl::int128>(&value) == nullptr) {
    throw std::invalid_argument(
        "Expression does not evaluate to an integer value.");
  }
  return int64_t(std::get<absl::int128>(value));
}

int64_t EvaluateIntegerExpression(
    const Expression& expression,
    std::function<int64_t(std::string_view)> lookup_variable) {
  absl::flat_hash_map<std::string, int64_t> values;
  auto needed_variables = NeededVariables(expression);
  if (!needed_variables.ok()) {
    throw std::invalid_argument("Failed to get needed variables");
  }
  for (const std::string& variable : *needed_variables) {
    values[variable] = lookup_variable(variable);
  }
  return EvaluateIntegerExpression(expression, values);
}

absl::StatusOr<absl::flat_hash_set<std::string>> NeededVariables(
    const Expression& expression) {
  absl::flat_hash_set<std::string> unknown_variables;
  MORIARTY_RETURN_IF_ERROR(
      moriarty_internal::GetUnknownVariables(expression, unknown_variables));
  return unknown_variables;
}

/* -------------------------------------------------------------------------- */
/*  STRING PARSING                                                            */
/* -------------------------------------------------------------------------- */

namespace moriarty_internal {
namespace {

// Special characters that may appear in your expression.
enum class SpecialCharacter {
  kOpenParen,
  kCloseParen,
  kStartOfString,
  kEndOfString,
  kComma
};

// Special operators which define beginning of a new scope.
enum class ScopeOperator {
  kStartOfString,
  kOpenParen,
  kStartOfFunction,
  kComma
};

// All different types of operators that be in your expression. Note that
// Parentheses are needed as a "scope changing" operator.
using OperatorType = std::variant<ScopeOperator, BinaryOperator, UnaryOperator>;

// All different types of tokens that need to be processed in an expression.
using TokenType = std::variant<SpecialCharacter, BinaryOperator, UnaryOperator,
                               Literal, Function>;

// Checks if the expression is just an argument list for a function (no name
// allowed).
bool IsArgumentListForPartialFunction(const Expression& expr) {
  if (!std::holds_alternative<Function>(expr.Get())) return false;
  return std::get<Function>(expr.Get()).Name().empty();
}

// Checks if the expression is just the name for a function (no arguments
// allowed).
bool IsNameForPartialFunction(const Expression& expr) {
  if (!std::holds_alternative<Function>(expr.Get())) return false;
  const Function& fn = std::get<Function>(expr.Get());
  return !fn.Name().empty() && fn.Arguments().empty();
}

void ConvertToArgumentList(Expression& expr) {
  std::vector<std::unique_ptr<Expression>> arg_list;
  arg_list.push_back(std::make_unique<Expression>(std::move(expr)));
  Function fn(/* name = */ "", std::move(arg_list));
  expr = Expression(std::move(fn));
}

absl::StatusOr<Expression> ValidateAndPopCompletedExpression(
    std::stack<Expression>& st) {
  if (st.empty()) {
    return absl::InvalidArgumentError("No elements on the stack");
  }

  if (IsArgumentListForPartialFunction(st.top())) {
    return absl::InvalidArgumentError("Incomplete function. Missing name?");
  }

  if (IsNameForPartialFunction(st.top())) {
    return absl::InvalidArgumentError(
        "Incomplete function. Missing arguments?");
  }

  Expression expr = std::move(st.top());
  st.pop();
  return expr;
}

// ApplyOperation
//
// Helper class that takes a reference to a stack for `ShuntingYard` and applies
// a specific operation. Each operation should take as many arguments from the
// top of the stack as needed, with the top being the rightmost parameter if
// multiple parameters are needed.
struct ApplyOperation {
  explicit ApplyOperation(std::stack<Expression>* st) : st(*st) {}
  std::stack<Expression>& st;

  absl::Status operator()(const ScopeOperator& op) {
    switch (op) {
      case ScopeOperator::kOpenParen:
        return absl::InvalidArgumentError(
            "Attempting to process an invalid open braket. Normally means "
            "there's an open parenthesis without corresponding closing "
            "parenthesis found.");

      case ScopeOperator::kStartOfString:
        return absl::InvalidArgumentError(
            "Attempting to process the start of string prematurely. This "
            "normally means there's an open parenthesis without corresponding "
            "closing parenthesis found.");

      case ScopeOperator::kComma: {
        if (st.size() < 2)
          return absl::InvalidArgumentError(
              "Attempting to apply a comma operation with fewer than two "
              "operands.");

        // The function's name will be empty iff it is on the same scope as this
        // comma operator. If it is not a partial function, then it is the first
        // argument of the function.
        if (!IsArgumentListForPartialFunction(st.top())) {
          ConvertToArgumentList(st.top());
        }

        Function fn = std::move(std::get<Function>(st.top().Get()));
        st.pop();
        fn.AppendArgument(std::move(st.top()));
        st.top() = Expression(std::move(fn));
        return absl::OkStatus();
      }

      case ScopeOperator::kStartOfFunction: {
        if (st.empty()) {
          return absl::InvalidArgumentError(
              "Attempting to parse a function that should have at least one "
              "parameter, but no parameters available.");
        }

        // My argument is the next value. Comma operator has already converted
        // several arguments into a partial function (with no-name). So if this
        // isn't in this partial form, convert it to that now.
        if (!IsArgumentListForPartialFunction(st.top())) {
          ConvertToArgumentList(st.top());
        }

        // The top item contains all arguments in reverse order.
        Function fn_args = std::move(std::get<Function>(st.top().Get()));
        st.pop();
        if (st.empty() || !IsNameForPartialFunction(st.top())) {
          return absl::InvalidArgumentError(
              "Attempting to parse a function, but failed to find name. (2)");
        }
        std::string name = std::get<Function>(st.top().Get()).Name();
        st.pop();

        fn_args.ReverseArguments();
        fn_args.SetName(name);
        st.push(Expression(std::move(fn_args)));

        return absl::OkStatus();
      }
    }
    throw std::runtime_error("Unknown scope operator");
  }

  absl::Status operator()(const BinaryOperator& op) {
    if (st.size() < 2)
      return absl::InvalidArgumentError(
          "Attempting to apply a binary operation with fewer than two "
          "operands.");

    MORIARTY_ASSIGN_OR_RETURN(Expression rhs,
                              ValidateAndPopCompletedExpression(st));
    MORIARTY_ASSIGN_OR_RETURN(Expression lhs,
                              ValidateAndPopCompletedExpression(st));

    st.push(Expression(BinaryOperation(std::move(lhs), op, std::move(rhs))));
    return absl::OkStatus();
  }

  absl::Status operator()(const UnaryOperator& op) {
    if (st.empty())
      return absl::InvalidArgumentError(
          "Attempting to apply a unary operation with no operands.");

    MORIARTY_ASSIGN_OR_RETURN(Expression rhs,
                              ValidateAndPopCompletedExpression(st));

    st.push(Expression(UnaryOperation(op, rhs)));
    return absl::OkStatus();
  }
};

bool IsLeftAssociative(BinaryOperator op) {
  // Only exponentiation is right associative.
  return op != BinaryOperator::kExponentiate;
}

// Precedence()
//
// Gives the precedence of an object in the parsing process. The lower the
// precedence, the earlier it will happen. For example, * should have lower
// precedence than +. All values here are multiples of 10 so we can add
// intermediate values in the future.
int Precedence(OperatorType type) {
  struct PrecedenceValues {
    int operator()(const BinaryOperator& op) {
      switch (op) {
        case BinaryOperator::kAdd:
        case BinaryOperator::kSubtract:
          return 60;
        case BinaryOperator::kMultiply:
        case BinaryOperator::kDivide:
        case BinaryOperator::kModulo:
          return 50;
        case BinaryOperator::kExponentiate:
          return 30;
      }
      throw std::runtime_error("Unknown binary operator");
    }
    int operator()(const UnaryOperator& op) {
      switch (op) {
        case UnaryOperator::kPlus:
        case UnaryOperator::kNegate:
          // This should be larger than exponentiation so -1 ^ 2 == -1 to mimic
          // Python
          return 40;
      }
      throw std::runtime_error("Unknown unary operator");
    }
    // Scope operators should have the highest priority so that everything
    // inside the scope is evaluated first. Within them, commas < brackets < EOS
    int operator()(const ScopeOperator& op) {
      switch (op) {
        case ScopeOperator::kComma:
          return 10010;
        case ScopeOperator::kOpenParen:
        case ScopeOperator::kStartOfFunction:
          return 10020;
        case ScopeOperator::kStartOfString:
          return 10030;
      }
      throw std::runtime_error("Unknown scope operator");
    }
  };

  return std::visit(PrecedenceValues(), type);
}

// IsMinusUnary()
//
// A '-' sign can either be unary (-5) or binary (5 - 3) depending on context.
// In particular, based on the previous token.
bool IsMinusUnary(const TokenType& previous_token) {
  struct IsUnaryMinusGivenPreviousToken {
    bool operator()(const BinaryOperator& op) { return true; }
    bool operator()(const UnaryOperator& op) {
      // For prefix unary, we are making the decision that you cannot have
      // another one following it. --3 is not -(-3).
      return false;
    }
    bool operator()(const SpecialCharacter& op) {
      return op == SpecialCharacter::kOpenParen ||
             op == SpecialCharacter::kStartOfString ||
             op == SpecialCharacter::kComma;
    }
    bool operator()(const Literal& lit) { return false; }
    bool operator()(const Function& fn) { return true; }
  };

  return std::visit(IsUnaryMinusGivenPreviousToken(), previous_token);
}

// ShuntingYard
//
// Processes an infix expression, and evaluates it as we go. Tokens should be
// inserted from left-to-right using the appropriate `operator()`. For example,
// for "3 + -4 * 5", you should call:
//
//  ShuntingYard SY;
//  SY(Literal(3));
//  SY(BinaryOperator(+));
//  SY(UnaryOperator(-));
//  SY(Literal(4));
//  SY(BinaryOperator(*));
//  SY(Literal(5));
//  ans = SY.FinishExpression();
class ShuntingYard {
 public:
  ShuntingYard() { operators_.push(ScopeOperator::kStartOfString); }

  // These operator() receive the next token in the stream.
  absl::Status operator()(const BinaryOperator& op) {
    bool left_assoc = IsLeftAssociative(op);
    int precedence_op = Precedence(op);
    while (!operators_.empty()) {
      int precedence_top = TopOpPrecedence();
      if (!(precedence_top < precedence_op ||
            (left_assoc && precedence_top == precedence_op)))
        break;
      MORIARTY_RETURN_IF_ERROR(
          std::visit(ApplyOperation(&postfix_), operators_.top()));
      operators_.pop();
    }
    operators_.push(op);
    return absl::OkStatus();
  }

  absl::Status operator()(const UnaryOperator& op) {
    // All unary operators right now are prefix-unary operators. Different logic
    // will be needed for postfix-unary operators.
    operators_.push(op);
    return absl::OkStatus();
  }

  absl::Status operator()(SpecialCharacter ch) {
    ABSL_CHECK(ch != SpecialCharacter::kStartOfString);

    if (ch == SpecialCharacter::kOpenParen) {
      operators_.push(ScopeOperator::kOpenParen);
      return absl::OkStatus();
    }

    if (ch == SpecialCharacter::kEndOfString) {
      ABSL_CHECK(!seen_end_of_string_);
      seen_end_of_string_ = true;
    }

    auto corresponding_open_scope = [](SpecialCharacter ch) {
      switch (ch) {
        case SpecialCharacter::kCloseParen:
          return ScopeOperator::kOpenParen;
        case SpecialCharacter::kEndOfString:
          return ScopeOperator::kStartOfString;
        case SpecialCharacter::kComma:
          return ScopeOperator::kComma;
        default:
          ABSL_CHECK(false) << "Should not get here.";
          return ScopeOperator::kOpenParen;
      }
    };

    int precedence_op = Precedence(corresponding_open_scope(ch));
    while (!operators_.empty()) {
      int precedence_top = TopOpPrecedence();
      if (precedence_top >= precedence_op) break;
      MORIARTY_RETURN_IF_ERROR(
          std::visit(ApplyOperation(&postfix_), operators_.top()));
      operators_.pop();
    }

    int precedence_top = TopOpPrecedence();
    if (operators_.empty() || precedence_top != precedence_op)
      if (ch == SpecialCharacter::kCloseParen)
        return absl::InvalidArgumentError("Unmatched closed parenthesis.");

    if (ch == SpecialCharacter::kCloseParen && IsFunction(operators_.top())) {
      MORIARTY_RETURN_IF_ERROR(
          std::visit(ApplyOperation(&postfix_), operators_.top()));
    }

    // Pop the corresponding closing if it matches us.
    if (ch != SpecialCharacter::kComma)
      if (precedence_top == precedence_op) operators_.pop();

    if (ch == SpecialCharacter::kComma) operators_.push(ScopeOperator::kComma);

    return absl::OkStatus();
  }

  absl::Status operator()(const Literal& lit) {
    postfix_.push(Expression(lit));
    return absl::OkStatus();
  }

  absl::Status operator()(const Function& f) {
    postfix_.push(Expression(f));
    operators_.push(ScopeOperator::kStartOfFunction);

    return absl::OkStatus();
  }

  // After all tokens have been added (including kEndOfString), GetResult()
  // returns the Expression
  absl::StatusOr<Expression> GetResult() {
    ABSL_CHECK(seen_end_of_string_);
    ABSL_CHECK(operators_.empty());

    if (postfix_.size() != 1) {
      return absl::InvalidArgumentError(
          absl::Substitute("Expression does not parse properly. $0 tokens in "
                           "stack. Inbalanced brackets?",
                           postfix_.size()));
    }
    return ValidateAndPopCompletedExpression(postfix_);
  }

 private:
  bool seen_end_of_string_ = false;
  std::stack<OperatorType> operators_;
  std::stack<Expression> postfix_;

  // Returns the precedence of the top token, or 0 for empty stack.
  int TopOpPrecedence() const {
    if (operators_.empty()) return 0;
    return Precedence(operators_.top());
  }

  bool IsFunction(OperatorType x) {
    return std::holds_alternative<ScopeOperator>(x) &&
           std::get<ScopeOperator>(x) == ScopeOperator::kStartOfFunction;
  }
};

void TrimLeadingWhitespace(std::string_view& expression) {
  while (!expression.empty() && std::isspace(expression.front()))
    expression.remove_prefix(1);
}

absl::StatusOr<Literal> ConsumeLeadingInteger(char c,
                                              std::string_view& expression) {
  std::string expr = std::string(1, c) + std::string(expression);
  int idx = 0;
  while (idx < expr.size() && std::isdigit(expr[idx])) idx++;
  std::string_view potential_int = std::string_view{expr}.substr(0, idx);

  int64_t value;
  if (!absl::SimpleAtoi(potential_int, &value)) {
    return absl::InvalidArgumentError(
        absl::Substitute("Failed to parse integer: $0", potential_int));
  }
  expression.remove_prefix(idx - 1);  // c was already removed
  return Literal(value);
}

// Reads the first token in expression. That token is returned and `expression`
// is updated to remove that token. Returns `std::nullopt` on an empty
// expression or unknown token.
absl::StatusOr<TokenType> ConsumeFirstToken(std::string_view& expression,
                                            const TokenType& previous_token) {
  TrimLeadingWhitespace(expression);
  if (expression.empty()) return SpecialCharacter::kEndOfString;

  char current = expression.front();
  expression.remove_prefix(1);

  if (current == '(') return SpecialCharacter::kOpenParen;
  if (current == ')') {
    if ((std::holds_alternative<SpecialCharacter>(previous_token) &&
         std::get<SpecialCharacter>(previous_token) ==
             SpecialCharacter::kOpenParen) ||
        std::holds_alternative<Function>(previous_token)) {
      return absl::InvalidArgumentError("Cannot have () in an expression.");
    }

    return SpecialCharacter::kCloseParen;
  }
  if (current == ',') return SpecialCharacter::kComma;

  if (current == '*') return BinaryOperator::kMultiply;
  if (current == '/') return BinaryOperator::kDivide;
  if (current == '%') return BinaryOperator::kModulo;
  if (current == '^') return BinaryOperator::kExponentiate;

  if (current == '+' || current == '-') {
    if (IsMinusUnary(previous_token))
      return current == '+' ? UnaryOperator::kPlus : UnaryOperator::kNegate;
    return current == '+' ? BinaryOperator::kAdd : BinaryOperator::kSubtract;
  }

  // Check for integer
  if (std::isdigit(current)) return ConsumeLeadingInteger(current, expression);

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
      return Function(variable_name, {});
    }

    return Literal(variable_name);
  }

  return absl::InvalidArgumentError(
      absl::Substitute("Unknown character: $0", current));
}

}  // namespace
}  // namespace moriarty_internal

// Implements the shunting-yard algorithm
// TODO(b/208295758): Add support for several features:
//  * functions (such as min(x, y), abs(x), clamp(n, lo, hi))
//  * comparison operators (<, >, <=, >=)
//  * equality operators (==, !=)
//  * boolean operators (and, or, xor, not)
//  * postfix unary operators (such as n!)
absl::StatusOr<Expression> ParseExpression(std::string_view expression) {
  moriarty_internal::ShuntingYard shunting_yard;

  std::string_view original_expression = expression;

  moriarty_internal::TokenType previous_token =
      moriarty_internal::SpecialCharacter::kStartOfString;
  while (!(std::holds_alternative<moriarty_internal::SpecialCharacter>(
               previous_token) &&
           std::get<moriarty_internal::SpecialCharacter>(previous_token) ==
               moriarty_internal::SpecialCharacter::kEndOfString)) {
    MORIARTY_ASSIGN_OR_RETURN(moriarty_internal::TokenType token,
                              ConsumeFirstToken(expression, previous_token));

    MORIARTY_RETURN_IF_ERROR(std::visit(shunting_yard, token));
    previous_token = std::move(token);
  }

  MORIARTY_ASSIGN_OR_RETURN(Expression result, shunting_yard.GetResult());
  result.SetString(original_expression);
  return result;
}

/* -------------------------------------------------------------------------- */
/*  EXPRESSION CLASSES                                                        */
/* -------------------------------------------------------------------------- */

namespace moriarty_internal {

BinaryOperation::BinaryOperation(Expression lhs, BinaryOperator op,
                                 Expression rhs)
    : op_(op),
      lhs_(std::make_unique<Expression>(std::move(lhs))),
      rhs_(std::make_unique<Expression>(std::move(rhs))) {}

BinaryOperation::BinaryOperation(const BinaryOperation& other)
    : op_(other.op_),
      lhs_(std::make_unique<Expression>(*other.lhs_)),
      rhs_(std::make_unique<Expression>(*other.rhs_)) {}

BinaryOperation& BinaryOperation::operator=(BinaryOperation other) {
  std::swap(op_, other.op_);
  std::swap(lhs_, other.lhs_);
  std::swap(rhs_, other.rhs_);
  return *this;
}

UnaryOperation::UnaryOperation(UnaryOperator op, Expression rhs)
    : op_(op), rhs_(std::make_unique<Expression>(std::move(rhs))) {}

UnaryOperation::UnaryOperation(const UnaryOperation& other)
    : op_(other.op_), rhs_(std::make_unique<Expression>(*other.rhs_)) {}

UnaryOperation& UnaryOperation::operator=(UnaryOperation other) {
  std::swap(op_, other.op_);
  std::swap(rhs_, other.rhs_);
  return *this;
}

Function::Function(std::string name,
                   std::vector<std::unique_ptr<Expression>> arguments)
    : name_(std::move(name)), arguments_(std::move(arguments)) {}

Function::Function(const Function& other) : name_(other.name_) {
  for (const auto& arg_ptr : other.arguments_) {
    arguments_.push_back(std::make_unique<Expression>(*arg_ptr));
  }
}

Function& Function::operator=(Function other) {
  std::swap(name_, other.name_);
  std::swap(arguments_, other.arguments_);
  return *this;
}

void Function::AppendArgument(Expression expr) {
  arguments_.push_back(std::make_unique<Expression>(std::move(expr)));
}

}  // namespace moriarty_internal

Expression::Expression(moriarty_internal::Literal lit)
    : expression_(std::move(lit)) {}

Expression::Expression(moriarty_internal::BinaryOperation binary_op)
    : expression_(std::move(binary_op)) {}

Expression::Expression(moriarty_internal::UnaryOperation unary_op)
    : expression_(std::move(unary_op)) {}

Expression::Expression(moriarty_internal::Function function)
    : expression_(std::move(function)) {}

}  // namespace moriarty
