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
#include <compare>
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
#include "src/contexts/librarian_context.h"
#include "src/internal/expressions.h"
#include "src/internal/range.h"
#include "src/librarian/errors.h"
#include "src/librarian/util/debug_string.h"
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

ConstraintViolation Between::CheckValue(librarian::AnalyzeVariableContext ctx,
                                        int64_t value) const {
  std::optional<Range::ExtremeValues<int64_t>> extremes =
      ctx.GetRangeEndpoints(GetRange());
  if (!extremes)
    return ConstraintViolation(
        std::format("is not between {} and {} (impossible)",
                    NumericToString(minimum_), NumericToString(maximum_)));
  if (extremes->min <= value && value <= extremes->max)
    return ConstraintViolation::None();
  return ConstraintViolation(std::format("is not between {} and {}",
                                         NumericToString(minimum_),
                                         NumericToString(maximum_), value));
}

ConstraintViolation Between::CheckValue(librarian::AnalyzeVariableContext ctx,
                                        int value) const {
  return CheckValue(ctx, static_cast<int64_t>(value));
}

namespace {

Real GetRealValue(librarian::AnalyzeVariableContext ctx,
                  const std::variant<int64_t, Expression, Real>& value) {
  if (std::holds_alternative<Real>(value)) return std::get<Real>(value);
  if (std::holds_alternative<int64_t>(value))
    return Real(std::get<int64_t>(value));
  return Real(ctx.EvaluateExpression(std::get<Expression>(value)));
}

}  // namespace

ConstraintViolation Between::CheckValue(librarian::AnalyzeVariableContext ctx,
                                        double value) const {
  Real mini = GetRealValue(ctx, minimum_);
  Real maxi = GetRealValue(ctx, maximum_);

  if (!(mini <= value && value <= maxi)) {
    return ConstraintViolation(std::format("is not between {} and {}",
                                           mini.ToString(), maxi.ToString()));
  }
  return ConstraintViolation::None();
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

ConstraintViolation AtMost::CheckValue(librarian::AnalyzeVariableContext ctx,
                                       int64_t value) const {
  std::optional<Range::ExtremeValues<int64_t>> extremes =
      ctx.GetRangeEndpoints(GetRange());
  if (!extremes)
    return ConstraintViolation(std::format("is not at most {} (impossible)",
                                           NumericToString(maximum_)));
  if (extremes->min <= value && value <= extremes->max)
    return ConstraintViolation::None();
  return ConstraintViolation(
      std::format("is not at most {}", NumericToString(maximum_)));
}

ConstraintViolation AtMost::CheckValue(librarian::AnalyzeVariableContext ctx,
                                       int value) const {
  return CheckValue(ctx, static_cast<int64_t>(value));
}

ConstraintViolation AtMost::CheckValue(librarian::AnalyzeVariableContext ctx,
                                       double value) const {
  Real maxi = GetRealValue(ctx, maximum_);

  if (value > maxi) {
    return ConstraintViolation(
        std::format("is not at most {}", maxi.ToString()));
  }
  std::partial_ordering maxi_cmp = maxi <=> value;
  if (maxi_cmp != std::partial_ordering::greater &&
      maxi_cmp != std::partial_ordering::equivalent) {
    return ConstraintViolation(
        std::format("is not at most {}", maxi.ToString()));
  }
  return ConstraintViolation::None();
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

ConstraintViolation AtLeast::CheckValue(librarian::AnalyzeVariableContext ctx,
                                        int64_t value) const {
  std::optional<Range::ExtremeValues<int64_t>> extremes =
      ctx.GetRangeEndpoints(GetRange());
  if (!extremes)
    return ConstraintViolation(std::format("is not at least {} (impossible)",
                                           NumericToString(minimum_)));
  if (extremes->min <= value && value <= extremes->max)
    return ConstraintViolation::None();
  return ConstraintViolation(
      std::format("is not at least {}", NumericToString(minimum_)));
}

ConstraintViolation AtLeast::CheckValue(librarian::AnalyzeVariableContext ctx,
                                        int value) const {
  return CheckValue(ctx, static_cast<int64_t>(value));
}

ConstraintViolation AtLeast::CheckValue(librarian::AnalyzeVariableContext ctx,
                                        double value) const {
  Real mini = GetRealValue(ctx, minimum_);

  if (value < mini) {
    return ConstraintViolation(
        std::format("is not at least {}", mini.ToString()));
  }

  return ConstraintViolation::None();
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

ConstraintViolation ExactlyNumeric::CheckValue(
    librarian::AnalyzeVariableContext ctx, int64_t value) const {
  if (std::holds_alternative<Expression>(value_)) {
    int64_t expected = ctx.EvaluateExpression(std::get<Expression>(value_));
    if (expected == value) return ConstraintViolation::None();
    return ConstraintViolation(
        std::format("is not exactly {} (got {})",
                    std::get<Expression>(value_).ToString(), value));
  }

  if (std::holds_alternative<int64_t>(value_)) {
    if (std::get<int64_t>(value_) == value) return ConstraintViolation::None();
    return ConstraintViolation(std::format("is not exactly {} (got {})",
                                           std::get<int64_t>(value_), value));
  }

  if (std::holds_alternative<Real>(value_)) {
    if (std::get<Real>(value_) == value) return ConstraintViolation::None();
    return ConstraintViolation(std::format("is not exactly {} (got {})",
                                           std::get<Real>(value_).ToString(),
                                           value));
  }

  throw std::logic_error(
      "ExactlyNumeric::CheckIntegerValue: unexpected value type");
}

ConstraintViolation ExactlyNumeric::CheckValue(
    librarian::AnalyzeVariableContext ctx, int value) const {
  return CheckValue(ctx, static_cast<int64_t>(value));
}

ConstraintViolation ExactlyNumeric::CheckValue(
    librarian::AnalyzeVariableContext ctx, double value) const {
  if (std::holds_alternative<Expression>(value_)) {
    int64_t expected = ctx.EvaluateExpression(std::get<Expression>(value_));
    if (CloseEnough(expected, value)) return ConstraintViolation::None();
    return ConstraintViolation(
        std::format("is not exactly {} (got {})",
                    std::get<Expression>(value_).ToString(), value));
  }

  if (std::holds_alternative<int64_t>(value_)) {
    if (CloseEnough(std::get<int64_t>(value_), value))
      return ConstraintViolation::None();
    return ConstraintViolation(std::format("is not exactly {} (got {})",
                                           std::get<int64_t>(value_), value));
  }

  if (std::holds_alternative<Real>(value_)) {
    if (CloseEnough(std::get<Real>(value_).GetApproxValue(), value))
      return ConstraintViolation::None();
    return ConstraintViolation(std::format("is not exactly {} (got {})",
                                           std::get<Real>(value_).ToString(),
                                           value));
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
  std::sort(dependencies_.begin(), dependencies_.end());
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
    std::sort(expr_options.begin(), expr_options.end());
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
    std::sort(expr_options.begin(), expr_options.end());
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

ConstraintViolation OneOfNumeric::CheckValue(
    librarian::AnalyzeVariableContext ctx, int64_t value) const {
  if (numeric_options_.HasBeenConstrained()) {
    if (!numeric_options_.HasOption(Real(value))) {
      return ConstraintViolation(
          std::format("{} is not {}", value,
                      OptionString(expr_options_, numeric_options_)));
    }
  }

  for (const std::vector<Expression>& option_list : expr_options_) {
    auto it = std::find_if(option_list.begin(), option_list.end(),
                           [&](const Expression& expr) {
                             return ctx.EvaluateExpression(expr) == value;
                           });
    if (it == option_list.end()) {
      return ConstraintViolation(
          std::format("{} is not {}", DebugString(value),
                      OptionString(expr_options_, numeric_options_)));
    }
  }
  return ConstraintViolation::None();
}

ConstraintViolation OneOfNumeric::CheckValue(
    librarian::AnalyzeVariableContext ctx, int value) const {
  return CheckValue(ctx, static_cast<int64_t>(value));
}

ConstraintViolation OneOfNumeric::CheckValue(
    librarian::AnalyzeVariableContext ctx, double value) const {
  if (numeric_options_.HasBeenConstrained() &&
      std::find_if(numeric_options_.GetOptions().begin(),
                   numeric_options_.GetOptions().end(),
                   [&](const Real& option) {
                     return CloseEnough(option.GetApproxValue(), value);
                   }) == numeric_options_.GetOptions().end()) {
    return ConstraintViolation(std::format(
        "{} is not {}", value, OptionString(expr_options_, numeric_options_)));
  }

  for (const std::vector<Expression>& option_list : expr_options_) {
    auto it = std::find_if(
        option_list.begin(), option_list.end(), [&](const Expression& expr) {
          return CloseEnough(ctx.EvaluateExpression(expr), value);
        });
    if (it == option_list.end()) {
      return ConstraintViolation(
          std::format("{} is not {}", DebugString(value),
                      OptionString(expr_options_, numeric_options_)));
    }
  }
  return ConstraintViolation::None();
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
