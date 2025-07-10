/*
 * Copyright 2024 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "src/constraints/numeric_constraints.h"

#include <compare>
#include <cstdint>
#include <format>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "src/constraints/constraint_violation.h"
#include "src/internal/expressions.h"
#include "src/internal/range.h"
#include "src/types/real.h"

namespace moriarty {

namespace {

std::string ToString(const std::variant<int64_t, Expression, Real>& value) {
  if (std::holds_alternative<int64_t>(value))
    return std::to_string(std::get<int64_t>(value));
  if (std::holds_alternative<Expression>(value))
    return std::get<Expression>(value).ToString();

  return std::get<Real>(value).ToString();
}

}  // namespace

// -----------------------------------------------------------------------------
//  ExactlyIntegerExpression

ExactlyIntegerExpression::ExactlyIntegerExpression(IntegerExpression value)
    : value_(value), dependencies_(value_.GetDependencies()) {}

Range ExactlyIntegerExpression::GetRange() const {
  Range r;
  r.AtMost(value_);
  r.AtLeast(value_);
  return r;
}

std::string ExactlyIntegerExpression::ToString() const {
  return std::format("is exactly {}", value_.ToString());
}

ConstraintViolation ExactlyIntegerExpression::CheckIntegerValue(
    LookupVariableFn lookup_variable, int64_t value) const {
  int64_t expected = value_.Evaluate(lookup_variable);
  if (expected == value) return ConstraintViolation::None();

  return ConstraintViolation(
      std::format("is not exactly {}", value_.ToString()));
}

std::vector<std::string> ExactlyIntegerExpression::GetDependencies() const {
  return dependencies_;
}

// -----------------------------------------------------------------------------
//  OneOfIntegerExpression

OneOfIntegerExpression::OneOfIntegerExpression(
    std::initializer_list<IntegerExpression> options)
    : moriarty::OneOfIntegerExpression(
          std::span<const IntegerExpression>{options}) {}

OneOfIntegerExpression::OneOfIntegerExpression(
    std::span<const IntegerExpression> options) {
  for (const auto& option : options) {
    options_.push_back(Expression(option));
    auto deps = options_.back().GetDependencies();
    dependencies_.insert(dependencies_.end(), deps.begin(), deps.end());
  }
}

namespace {

std::string OptionString(const std::vector<Expression>& exprs) {
  std::string options = "{";
  for (const auto& option : exprs) {
    if (options.size() > 1) options += ", ";
    options += option.ToString();
  }
  options += "}";
  return options;
}

}  // namespace

std::vector<std::string> OneOfIntegerExpression::GetOptions() const {
  std::vector<std::string> options;
  options.reserve(options_.size());
  for (const auto& option : options_) options.push_back(option.ToString());
  return options;
}

std::string OneOfIntegerExpression::ToString() const {
  return std::format("is one of {}", OptionString(options_));
}

ConstraintViolation OneOfIntegerExpression::CheckIntegerValue(
    LookupVariableFn lookup_variable, int64_t value) const {
  for (const auto& option : options_) {
    int64_t expected = option.Evaluate(lookup_variable);
    if (expected == value) return ConstraintViolation::None();
  }
  return ConstraintViolation(
      std::format("is not one of {}", OptionString(options_)));
}

std::vector<std::string> OneOfIntegerExpression::GetDependencies() const {
  return dependencies_;
}

// -----------------------------------------------------------------------------
//  Between

Between::Between(int64_t minimum, int64_t maximum)
    : minimum_(minimum), maximum_(maximum) {
  if (minimum > maximum) {
    throw std::invalid_argument(
        "minimum must be less than or equal to maximum in Between()");
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
    throw std::invalid_argument(
        "minimum must be less than or equal to maximum in Between()");
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
  return std::format("is between {} and {}", ::moriarty::ToString(minimum_),
                     ::moriarty::ToString(maximum_));
}

ConstraintViolation Between::CheckIntegerValue(LookupVariableFn lookup_variable,
                                               int64_t value) const {
  std::optional<Range::ExtremeValues> extremes =
      GetRange().Extremes(lookup_variable);
  if (!extremes)
    return ConstraintViolation(std::format(
        "is not between {} and {} (impossible)", ::moriarty::ToString(minimum_),
        ::moriarty::ToString(maximum_)));
  if (extremes->min <= value && value <= extremes->max)
    return ConstraintViolation::None();
  return ConstraintViolation(
      std::format("is not between {} and {}", ::moriarty::ToString(minimum_),
                  ::moriarty::ToString(maximum_), value));
}

namespace {

Real GetRealValue(const std::variant<int64_t, Expression, Real>& value,
                  NumericRangeMConstraint::LookupVariableFn lookup_variable) {
  if (std::holds_alternative<Real>(value)) return std::get<Real>(value);
  if (std::holds_alternative<int64_t>(value))
    return Real(std::get<int64_t>(value));
  return Real(std::get<Expression>(value).Evaluate(lookup_variable));
}

}  // namespace

ConstraintViolation Between::CheckRealValue(LookupVariableFn lookup_variable,
                                            double value) const {
  Real mini = GetRealValue(minimum_, lookup_variable);
  Real maxi = GetRealValue(maximum_, lookup_variable);

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
  return std::format("is at most {}", ::moriarty::ToString(maximum_));
}

ConstraintViolation AtMost::CheckIntegerValue(LookupVariableFn lookup_variable,
                                              int64_t value) const {
  std::optional<Range::ExtremeValues> extremes =
      GetRange().Extremes(lookup_variable);
  if (!extremes)
    return ConstraintViolation(std::format("is not at most {} (impossible)",
                                           ::moriarty::ToString(maximum_)));
  if (extremes->min <= value && value <= extremes->max)
    return ConstraintViolation::None();
  return ConstraintViolation(
      std::format("is not at most {}", ::moriarty::ToString(maximum_)));
}

ConstraintViolation AtMost::CheckRealValue(LookupVariableFn lookup_variable,
                                           double value) const {
  Real maxi = GetRealValue(maximum_, lookup_variable);

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
  return std::format("is at least {}", ::moriarty::ToString(minimum_));
}

ConstraintViolation AtLeast::CheckIntegerValue(LookupVariableFn lookup_variable,
                                               int64_t value) const {
  std::optional<Range::ExtremeValues> extremes =
      GetRange().Extremes(lookup_variable);
  if (!extremes)
    return ConstraintViolation(std::format("is not at least {} (impossible)",
                                           ::moriarty::ToString(minimum_)));
  if (extremes->min <= value && value <= extremes->max)
    return ConstraintViolation::None();
  return ConstraintViolation(
      std::format("is not at least {}", ::moriarty::ToString(minimum_)));
}

ConstraintViolation AtLeast::CheckRealValue(LookupVariableFn lookup_variable,
                                            double value) const {
  Real mini = GetRealValue(minimum_, lookup_variable);

  if (value < mini) {
    return ConstraintViolation(
        std::format("is not at least {}", mini.ToString()));
  }

  return ConstraintViolation::None();
}

std::vector<std::string> AtLeast::GetDependencies() const {
  return dependencies_;
}

}  // namespace moriarty
