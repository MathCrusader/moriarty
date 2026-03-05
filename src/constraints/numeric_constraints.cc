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

#include "src/constraints/numeric_constraints.h"

#include <algorithm>
#include <cstdint>
#include <format>
#include <functional>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "src/constraints/constraint_violation.h"
#include "src/context.h"
#include "src/contexts/librarian_context.h"
#include "src/internal/expressions.h"
#include "src/internal/range.h"
#include "src/librarian/errors.h"
#include "src/types/real.h"

namespace moriarty {

namespace {

std::string NumericToString(
    const std::variant<int64_t, Expression, Real>& value) {
  if (std::holds_alternative<int64_t>(value))
    return std::to_string(std::get<int64_t>(value));
  if (std::holds_alternative<Expression>(value))
    return std::get<Expression>(value).ToString();

  return std::get<Real>(value).ToString();
}

bool CloseEnough(double a, double b) {
  // FIXME: Set up appropriate global tolerance.
  return std::abs(a - b) < 1e-9;
}

}  // namespace

// -----------------------------------------------------------------------------
//  Between

Between::Between(int64_t minimum, int64_t maximum)
    : minimum_(minimum), maximum_(maximum) {
  if (minimum > maximum) {
    throw InvalidConstraint(
        "Between",
        std::format("minimum ({}) must be less than or equal to maximum ({})",
                    minimum, maximum));
  }
}

Between::Between(int64_t minimum, IntegerExpression maximum)
    : minimum_(minimum),
      maximum_(Expression(maximum)),
      dependencies_(std::get<Expression>(maximum_).GetDependencies()) {}

Between::Between(int64_t minimum, Real maximum)
    : minimum_(minimum), maximum_(maximum) {}

Between::Between(IntegerExpression minimum, int64_t maximum)
    : minimum_(Expression(minimum)),
      maximum_(maximum),
      dependencies_(std::get<Expression>(minimum_).GetDependencies()) {}

Between::Between(IntegerExpression minimum, IntegerExpression maximum)
    : minimum_(Expression(minimum)), maximum_(Expression(maximum)) {
  dependencies_ = std::get<Expression>(minimum_).GetDependencies();
  auto max_deps = std::get<Expression>(maximum_).GetDependencies();
  dependencies_.insert(dependencies_.end(), max_deps.begin(), max_deps.end());
}

Between::Between(IntegerExpression minimum, Real maximum)
    : minimum_(Expression(minimum)),
      maximum_(maximum),
      dependencies_(std::get<Expression>(minimum_).GetDependencies()) {}

Between::Between(Real minimum, int64_t maximum)
    : minimum_(minimum), maximum_(maximum) {
  if (minimum > maximum) {
    throw InvalidConstraint(
        "Between",
        std::format("minimum ({}) must be less than or equal to maximum ({})",
                    minimum.ToString(), maximum));
  }
}

Between::Between(Real minimum, IntegerExpression maximum)
    : minimum_(minimum),
      maximum_(Expression(maximum)),
      dependencies_(std::get<Expression>(maximum_).GetDependencies()) {}

Between::Between(Real minimum, Real maximum)
    : minimum_(minimum), maximum_(maximum) {}

Range Between::GetRange() const {
  Range r;
  std::visit([&](const auto& min) { r.AtLeast(min); }, minimum_);
  std::visit([&](const auto& max) { r.AtMost(max); }, maximum_);
  return r;
}

std::string Between::ToString() const {
  return std::format("is between {} and {}", NumericToString(minimum_),
                     NumericToString(maximum_));
}

namespace {

Real GetRealValue(ConstraintContext ctx,
                  const std::variant<int64_t, Expression, Real>& value) {
  if (std::holds_alternative<Real>(value)) return std::get<Real>(value);
  if (std::holds_alternative<int64_t>(value))
    return Real(std::get<int64_t>(value));
  return Real(ctx.EvaluateExpression(std::get<Expression>(value)));
}

}  // namespace

ValidationResult Between::Validate(ConstraintContext ctx, int64_t value) const {
  Real mini = GetRealValue(ctx, minimum_);
  Real maxi = GetRealValue(ctx, maximum_);

  if (mini <= value && value <= maxi) return ValidationResult::Ok();

  std::optional<librarian::Details> details;
  if (maxi < mini) {
    details = librarian::Details(
        "minimum {} is greater than maximum {} (which is impossible to "
        "satisfy)",
        mini.ToString(), maxi.ToString());
  } else if (value < mini) {
    details = librarian::Details("too small");
  } else if (value > maxi) {
    details = librarian::Details("too large");
  }
  return ValidationResult::Violation(
      ctx.GetLocalVariableName(), value,
      librarian::Expected("{} ≤ {} ≤ {}", NumericToString(minimum_),
                          ctx.GetLocalVariableName(),
                          NumericToString(maximum_)),
      librarian::Evaluated("{} ≤ {} ≤ {}", mini.ToString(),
                           ctx.GetLocalVariableName(), maxi.ToString()),
      details);
}

ValidationResult Between::Validate(ConstraintContext ctx, int value) const {
  return Validate(ctx, static_cast<int64_t>(value));
}

ValidationResult Between::Validate(ConstraintContext ctx, double value) const {
  Real mini = GetRealValue(ctx, minimum_);
  Real maxi = GetRealValue(ctx, maximum_);

  if (mini <= value && value <= maxi) return ValidationResult::Ok();

  std::optional<librarian::Details> details;
  if (maxi < mini) {
    details = librarian::Details(
        "minimum {} is greater than maximum {} (which is impossible to "
        "satisfy)",
        mini.ToString(), maxi.ToString());
  } else if (value < mini) {
    details = librarian::Details("too small");
  } else if (value > maxi) {
    details = librarian::Details("too large");
  }
  return ValidationResult::Violation(
      ctx.GetLocalVariableName(), value,
      librarian::Expected("{} ≤ {} ≤ {}", NumericToString(minimum_),
                          ctx.GetLocalVariableName(),
                          NumericToString(maximum_)),
      librarian::Evaluated("{} ≤ {} ≤ {}", mini.ToString(),
                           ctx.GetLocalVariableName(), maxi.ToString()),
      details);
}

std::vector<std::string> Between::GetDependencies() const {
  return dependencies_;
}

// -----------------------------------------------------------------------------
//  AtMost

AtMost::AtMost(int64_t maximum) : maximum_(maximum) {}

AtMost::AtMost(IntegerExpression maximum)
    : maximum_(Expression(maximum)),
      dependencies_(std::get<Expression>(maximum_).GetDependencies()) {}

AtMost::AtMost(Real maximum) : maximum_(maximum) {}

Range AtMost::GetRange() const {
  Range r;
  std::visit([&](const auto& max) { r.AtMost(max); }, maximum_);
  return r;
}

std::string AtMost::ToString() const {
  return std::format("is at most {}", NumericToString(maximum_));
}

ValidationResult AtMost::Validate(ConstraintContext ctx, int64_t value) const {
  Real maxi = GetRealValue(ctx, maximum_);

  if (value <= maxi) return ValidationResult::Ok();

  return ValidationResult::Violation(
      ctx.GetLocalVariableName(), value,
      librarian::Expected("{} ≤ {}", ctx.GetLocalVariableName(),
                          NumericToString(maximum_)),
      librarian::Evaluated("{} ≤ {}", ctx.GetLocalVariableName(),
                           maxi.ToString()),
      librarian::Details("too large"));
}

ValidationResult AtMost::Validate(ConstraintContext ctx, int value) const {
  return Validate(ctx, static_cast<int64_t>(value));
}

ValidationResult AtMost::Validate(ConstraintContext ctx, double value) const {
  Real maxi = GetRealValue(ctx, maximum_);

  if (value <= maxi) return ValidationResult::Ok();

  return ValidationResult::Violation(
      ctx.GetLocalVariableName(), value,
      librarian::Expected("{} ≤ {}", ctx.GetLocalVariableName(),
                          NumericToString(maximum_)),
      librarian::Evaluated("{} ≤ {}", ctx.GetLocalVariableName(),
                           maxi.ToString()),
      librarian::Details("too large"));
}

std::vector<std::string> AtMost::GetDependencies() const {
  return dependencies_;
}

// -----------------------------------------------------------------------------
//  AtLeast

AtLeast::AtLeast(int64_t minimum) : minimum_(minimum) {}

AtLeast::AtLeast(IntegerExpression minimum)
    : minimum_(Expression(minimum)),
      dependencies_(std::get<Expression>(minimum_).GetDependencies()) {}

AtLeast::AtLeast(Real minimum) : minimum_(minimum) {}

Range AtLeast::GetRange() const {
  Range r;
  std::visit([&](const auto& min) { r.AtLeast(min); }, minimum_);
  return r;
}

std::string AtLeast::ToString() const {
  return std::format("is at least {}", NumericToString(minimum_));
}

ValidationResult AtLeast::Validate(ConstraintContext ctx, int64_t value) const {
  Real mini = GetRealValue(ctx, minimum_);

  if (value >= mini) return ValidationResult::Ok();

  return ValidationResult::Violation(
      ctx.GetLocalVariableName(), value,
      librarian::Expected("{} ≥ {}", ctx.GetLocalVariableName(),
                          NumericToString(minimum_)),
      librarian::Evaluated("{} ≥ {}", ctx.GetLocalVariableName(),
                           mini.ToString()),
      librarian::Details("too small"));
}

ValidationResult AtLeast::Validate(ConstraintContext ctx, int value) const {
  return Validate(ctx, static_cast<int64_t>(value));
}

ValidationResult AtLeast::Validate(ConstraintContext ctx, double value) const {
  Real mini = GetRealValue(ctx, minimum_);

  if (value >= mini) return ValidationResult::Ok();

  return ValidationResult::Violation(
      ctx.GetLocalVariableName(), value,
      librarian::Expected("{} ≥ {}", ctx.GetLocalVariableName(),
                          NumericToString(minimum_)),
      librarian::Evaluated("{} ≥ {}", ctx.GetLocalVariableName(),
                           mini.ToString()),
      librarian::Details("too small"));
}

std::vector<std::string> AtLeast::GetDependencies() const {
  return dependencies_;
}

namespace librarian {

// -----------------------------------------------------------------------------
//  ExactlyNumeric

ExactlyNumeric::ExactlyNumeric(int64_t value) : value_(value) {}

ExactlyNumeric::ExactlyNumeric(Real value) : value_(value) {}

ExactlyNumeric::ExactlyNumeric(IntegerExpression value)
    : value_(Expression(value)),
      dependencies_(std::get<Expression>(value_).GetDependencies()) {}

Range ExactlyNumeric::GetRange() const {
  Range r;
  std::visit([&](const auto& min) { r.AtLeast(min); }, value_);
  std::visit([&](const auto& max) { r.AtMost(max); }, value_);
  return r;
}

std::variant<int64_t, Expression, Real> ExactlyNumeric::GetValue() const {
  return value_;
}

std::string ExactlyNumeric::ToString() const {
  return std::format("is exactly {}", NumericToString(value_));
}

ValidationResult ExactlyNumeric::Validate(ConstraintContext ctx,
                                          int64_t value) const {
  if (std::holds_alternative<Expression>(value_)) {
    const Expression& expr = std::get<Expression>(value_);
    int64_t expected = ctx.EvaluateExpression(expr);
    if (expected == value) return ValidationResult::Ok();
    return ValidationResult::Violation(ctx.GetLocalVariableName(), value,
                                       librarian::Expected(expr.ToString()),
                                       librarian::Evaluated("{}", expected));
  }

  if (std::holds_alternative<int64_t>(value_)) {
    int64_t expected = std::get<int64_t>(value_);
    if (expected == value) return ValidationResult::Ok();
    return ValidationResult::Violation(ctx.GetLocalVariableName(), value,
                                       librarian::Expected("{}", expected));
  }

  if (std::holds_alternative<Real>(value_)) {
    const Real& expected = std::get<Real>(value_);
    if (expected == value) return ValidationResult::Ok();
    return ValidationResult::Violation(
        ctx.GetLocalVariableName(), value,
        librarian::Expected(expected.ToString()));
  }

  throw std::logic_error(
      "ExactlyNumeric::CheckIntegerValue: unexpected value type");
}

ValidationResult ExactlyNumeric::Validate(ConstraintContext ctx,
                                          int value) const {
  return Validate(ctx, static_cast<int64_t>(value));
}

ValidationResult ExactlyNumeric::Validate(ConstraintContext ctx,
                                          double value) const {
  if (std::holds_alternative<Expression>(value_)) {
    const Expression& expr = std::get<Expression>(value_);
    int64_t expected = ctx.EvaluateExpression(expr);
    if (CloseEnough(expected, value)) return ValidationResult::Ok();
    return ValidationResult::Violation(ctx.GetLocalVariableName(), value,
                                       librarian::Expected(expr.ToString()),
                                       librarian::Evaluated("{}", expected));
  }

  if (std::holds_alternative<int64_t>(value_)) {
    int64_t expected = std::get<int64_t>(value_);
    if (CloseEnough(expected, value)) return ValidationResult::Ok();
    return ValidationResult::Violation(ctx.GetLocalVariableName(), value,
                                       librarian::Expected("{}", expected));
  }

  if (std::holds_alternative<Real>(value_)) {
    const Real& expected = std::get<Real>(value_);
    if (CloseEnough(expected.GetApproxValue(), value))
      return ValidationResult::Ok();
    return ValidationResult::Violation(
        ctx.GetLocalVariableName(), value,
        librarian::Expected(expected.ToString()));
  }

  throw std::logic_error(
      "ExactlyNumeric::CheckIntegerValue: unexpected value type");
}

std::vector<std::string> ExactlyNumeric::GetDependencies() const {
  return dependencies_;
}

// -----------------------------------------------------------------------------
//  OneOfNumeric

OneOfNumeric::OneOfNumeric(std::span<const std::string> options)
    : expr_options_(std::vector<std::vector<Expression>>(
          1, std::vector<Expression>(options.begin(), options.end()))) {
  for (const auto& expr : expr_options_.front()) {
    auto deps_for_option = expr.GetDependencies();
    dependencies_.insert(dependencies_.end(), deps_for_option.begin(),
                         deps_for_option.end());
  }
}

OneOfNumeric::OneOfNumeric(std::span<const IntegerExpression> options)
    : expr_options_(std::vector<std::vector<Expression>>(
          1, std::vector<Expression>(options.begin(), options.end()))) {
  for (const auto& expr : expr_options_.front()) {
    auto deps_for_option = expr.GetDependencies();
    dependencies_.insert(dependencies_.end(), deps_for_option.begin(),
                         deps_for_option.end());
  }
}

OneOfNumeric::OneOfNumeric(std::span<const Real> options) {
  std::ignore = numeric_options_.ConstrainOptions(options);
}

OneOfNumeric::OneOfNumeric(std::span<const int64_t> options) {
  std::ignore = numeric_options_.ConstrainOptions(
      std::vector<Real>(options.begin(), options.end()));
}

bool OneOfNumeric::ConstrainOptions(IntegerExpression other) {
  return ConstrainOptions(
      OneOfNumeric(std::span<const IntegerExpression>{{other}}));
}

bool OneOfNumeric::ConstrainOptions(int64_t other) {
  return ConstrainOptions(OneOfNumeric(std::span<const int64_t>{{other}}));
}

bool OneOfNumeric::ConstrainOptions(Real other) {
  return ConstrainOptions(OneOfNumeric(std::span<const Real>{{other}}));
}

bool OneOfNumeric::ConstrainOptions(const OneOfNumeric& other) {
  expr_options_.insert(expr_options_.end(), other.expr_options_.begin(),
                       other.expr_options_.end());
  dependencies_.insert(dependencies_.end(), other.dependencies_.begin(),
                       other.dependencies_.end());
  std::ranges::sort(dependencies_);
  dependencies_.erase(std::unique(dependencies_.begin(), dependencies_.end()),
                      dependencies_.end());

  if (other.numeric_options_.HasBeenConstrained()) {
    if (!numeric_options_.ConstrainOptions(
            other.numeric_options_.GetOptions())) {
      // No numeric options left. It doesn't matter what the expressions are,
      // there's nothing valid left.
      return false;
    }
  }

  // It's possible that there are no valid options left in the expressions, but
  // it's too hard to figure that out in general.
  // Example: OneOf({x, x + 1}) and OneOf({x + 3, x + 4}) is never valid.
  return true;
}

bool OneOfNumeric::ConstrainOptions(
    std::span<const IntegerExpression> options) {
  return ConstrainOptions(OneOfNumeric(options));
}

bool OneOfNumeric::ConstrainOptions(std::span<const Real> options) {
  return ConstrainOptions(OneOfNumeric(options));
}

bool OneOfNumeric::ConstrainOptions(std::span<const int64_t> options) {
  return ConstrainOptions(OneOfNumeric(options));
}

bool OneOfNumeric::ConstrainOptions(const ExactlyNumeric& other) {
  auto val = other.GetValue();
  if (std ::holds_alternative<int64_t>(val)) {
    return ConstrainOptions(std::get<int64_t>(val));
  } else if (std::holds_alternative<Real>(val)) {
    return ConstrainOptions(std::get<Real>(val));
  } else if (std::holds_alternative<Expression>(val)) {
    return ConstrainOptions(std::get<Expression>(val).ToString());
  }

  throw std::logic_error(
      "OneOfNumeric::ConstrainOptions: unexpected value type");
}

namespace {

template <typename T>
void AppendList(const std::vector<T>& list, std::string& append_to) {
  append_to += "[";
  bool first = true;
  for (const T& option : list) {
    if (!first) append_to += ", ";
    first = false;
    append_to += option.ToString();
  }
  append_to += "]";
}

std::string OptionString(const std::vector<std::vector<Expression>>& exprs,
                         const OneOfHandler<Real>& reals) {
  std::string options;
  if (exprs.size() + reals.HasBeenConstrained() != 1) {
    options += "one of the elements from each list: {";
  } else {
    options += "one of: ";
  }
  bool first = true;
  for (const std::vector<Expression>& option_list : exprs) {
    if (!first) options += ", ";
    first = false;
    AppendList(option_list, options);
  }
  if (reals.HasBeenConstrained()) {
    if (!first) options += ", ";
    first = false;
    AppendList(reals.GetOptions(), options);
  }
  if (exprs.size() + reals.HasBeenConstrained() != 1) options += "}";
  return options;
}

}  // namespace

std::vector<Real> OneOfNumeric::GetOptions(
    librarian::AnalyzeVariableContext ctx) const {
  std::optional<std::vector<Real>> valid_options;
  if (numeric_options_.HasBeenConstrained()) {
    valid_options = numeric_options_.GetOptions();
    std::sort(valid_options->begin(), valid_options->end());
    valid_options->erase(
        std::unique(valid_options->begin(), valid_options->end()),
        valid_options->end());
  }

  for (const std::vector<Expression>& oneof_list : expr_options_) {
    std::vector<Real> expr_options;
    for (const Expression& expr : oneof_list)
      expr_options.push_back(Real(ctx.EvaluateExpression(expr)));
    std::ranges::sort(expr_options);
    expr_options.erase(std::unique(expr_options.begin(), expr_options.end()),
                       expr_options.end());
    if (!valid_options) {
      valid_options = std::move(expr_options);
    } else {
      valid_options->erase(
          std::set_intersection(valid_options->begin(), valid_options->end(),
                                expr_options.begin(), expr_options.end(),
                                valid_options->begin()),
          valid_options->end());
    }
  }

  return valid_options.value_or(std::vector<Real>());
}

std::vector<Real> OneOfNumeric::GetOptionsLookup(
    std::function<int64_t(std::string_view)> lookup_variable) const {
  std::optional<std::vector<Real>> valid_options;
  if (numeric_options_.HasBeenConstrained()) {
    valid_options = numeric_options_.GetOptions();
    std::sort(valid_options->begin(), valid_options->end());
    valid_options->erase(
        std::unique(valid_options->begin(), valid_options->end()),
        valid_options->end());
  }

  for (const std::vector<Expression>& oneof_list : expr_options_) {
    std::vector<Real> expr_options;
    for (const Expression& expr : oneof_list)
      expr_options.push_back(Real(expr.Evaluate(lookup_variable)));
    std::ranges::sort(expr_options);
    expr_options.erase(std::unique(expr_options.begin(), expr_options.end()),
                       expr_options.end());
    if (!valid_options) {
      valid_options = std::move(expr_options);
    } else {
      valid_options->erase(
          std::set_intersection(valid_options->begin(), valid_options->end(),
                                expr_options.begin(), expr_options.end(),
                                valid_options->begin()),
          valid_options->end());
    }
  }

  return valid_options.value_or(std::vector<Real>());
}

std::string OneOfNumeric::ToString() const {
  return std::format("is {}", OptionString(expr_options_, numeric_options_));
}

ValidationResult OneOfNumeric::Validate(ConstraintContext ctx,
                                        int64_t value) const {
  if (numeric_options_.HasBeenConstrained()) {
    if (!numeric_options_.HasOption(Real(value))) {
      const std::vector<Real>& options = numeric_options_.GetOptions();
      return ValidationResult::Violation(
          ctx.GetLocalVariableName(), value,
          librarian::Expected("one of {}",
                              moriarty_internal::ValuePrinter(options)));
    }
  }

  for (const std::vector<Expression>& option_list : expr_options_) {
    std::vector<int64_t> evaluated_options;
    evaluated_options.reserve(option_list.size());
    for (const Expression& expr : option_list)
      evaluated_options.push_back(ctx.EvaluateExpression(expr));

    if (std::ranges::find(evaluated_options, value) ==
        evaluated_options.end()) {
      std::string expected_options;
      AppendList(option_list, expected_options);
      return ValidationResult::Violation(
          ctx.GetLocalVariableName(), value,
          librarian::Expected("one of {}", expected_options),
          librarian::Evaluated(
              "one of {}", moriarty_internal::ValuePrinter(evaluated_options)));
    }
  }
  return ValidationResult::Ok();
}

ValidationResult OneOfNumeric::Validate(ConstraintContext ctx,
                                        int value) const {
  return Validate(ctx, static_cast<int64_t>(value));
}

ValidationResult OneOfNumeric::Validate(ConstraintContext ctx,
                                        double value) const {
  if (numeric_options_.HasBeenConstrained() &&
      std::ranges::find_if(numeric_options_.GetOptions(),
                           [&](const Real& option) {
                             return CloseEnough(option.GetApproxValue(), value);
                           }) == numeric_options_.GetOptions().end()) {
    const std::vector<Real>& options = numeric_options_.GetOptions();
    return ValidationResult::Violation(
        ctx.GetLocalVariableName(), value,
        librarian::Expected("one of {}",
                            moriarty_internal::ValuePrinter(options)));
  }

  for (const std::vector<Expression>& option_list : expr_options_) {
    std::vector<int64_t> evaluated_options;
    evaluated_options.reserve(option_list.size());
    for (const Expression& expr : option_list)
      evaluated_options.push_back(ctx.EvaluateExpression(expr));

    auto it = std::ranges::find_if(evaluated_options, [&](int64_t option) {
      return CloseEnough(option, value);
    });
    if (it == evaluated_options.end()) {
      std::string expected_options;
      AppendList(option_list, expected_options);
      return ValidationResult::Violation(
          ctx.GetLocalVariableName(), value,
          librarian::Expected("one of {}", expected_options),
          librarian::Evaluated(
              "one of {}", moriarty_internal::ValuePrinter(evaluated_options)));
    }
  }
  return ValidationResult::Ok();
}

std::vector<std::string> OneOfNumeric::GetDependencies() const {
  return dependencies_;
}

bool OneOfNumeric::HasBeenConstrained() const {
  return numeric_options_.HasBeenConstrained() || !expr_options_.empty();
}

std::optional<Real> OneOfNumeric::GetUniqueValue(
    librarian::AnalyzeVariableContext ctx) const {
  // TODO: We can optimize this by checking if we have a single option and early
  // exit.
  auto options = GetOptions(ctx);
  if (options.size() != 1) return std::nullopt;
  return options.front();
}

}  // namespace librarian
}  // namespace moriarty
