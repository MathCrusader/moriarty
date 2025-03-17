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

#include "src/variables/constraints/numeric_constraints.h"

#include <cstdint>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "src/internal/expressions.h"
#include "src/internal/range.h"

namespace moriarty {

// -----------------------------------------------------------------------------
//  ExactlyIntegerExpression

// TODO: These hide absl::StatusOr<>. We should consider alternatives.
ExactlyIntegerExpression::ExactlyIntegerExpression(IntegerExpression value)
    : value_(*ParseExpression(value)) {}

Range ExactlyIntegerExpression::GetRange() const {
  Range r;
  r.AtMost(value_.ToString()).IgnoreError();
  r.AtLeast(value_.ToString()).IgnoreError();
  return r;
}

std::string ExactlyIntegerExpression::ToString() const {
  return std::format("is exactly {}", value_.ToString());
}

bool ExactlyIntegerExpression::IsSatisfiedWith(LookupVariableFn lookup_variable,
                                               int64_t value) const {
  int64_t expected = EvaluateIntegerExpression(value_, lookup_variable);
  return expected == value;
}

std::string ExactlyIntegerExpression::Explanation(
    LookupVariableFn lookup_variable, int64_t value) const {
  return std::format("is not exactly {}", value_.ToString());
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
    auto expr = ParseExpression(option);
    if (!expr.ok()) {
      throw std::invalid_argument(
          std::format("OneOfIntegerExpression: {}", expr.status().ToString()));
    }
    options_.push_back(*std::move(expr));
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

bool OneOfIntegerExpression::IsSatisfiedWith(LookupVariableFn lookup_variable,
                                             int64_t value) const {
  for (const auto& option : options_) {
    int64_t expected = EvaluateIntegerExpression(option, lookup_variable);
    if (expected == value) return true;
  }
  return false;
}

std::string OneOfIntegerExpression::Explanation(
    LookupVariableFn lookup_variable, int64_t value) const {
  return std::format("is not one of {}", OptionString(options_));
}

// -----------------------------------------------------------------------------
//  Between

// TODO: These hide absl::StatusOr<>. We should consider alternatives.
Between::Between(int64_t minimum, int64_t maximum)
    : minimum_(*ParseExpression(std::to_string(minimum))),
      maximum_(*ParseExpression(std::to_string(maximum))) {
  if (minimum > maximum) {
    throw std::invalid_argument(
        "minimum must be less than or equal to maximum in Between()");
  }
}

Between::Between(int64_t minimum, IntegerExpression maximum)
    : minimum_(*ParseExpression(std::to_string(minimum))),
      maximum_(*ParseExpression(maximum)) {}

Between::Between(IntegerExpression minimum, int64_t maximum)
    : minimum_(*ParseExpression(minimum)),
      maximum_(*ParseExpression(std::to_string(maximum))) {}

Between::Between(IntegerExpression minimum, IntegerExpression maximum)
    : minimum_(*ParseExpression(minimum)),
      maximum_(*ParseExpression(maximum)) {}

// FIXME: This is a silly way of doing this... We shouldn't reconstruct.
Range Between::GetRange() const {
  Range r;
  r.AtLeast(minimum_.ToString()).IgnoreError();
  r.AtMost(maximum_.ToString()).IgnoreError();
  return r;
}

std::string Between::ToString() const {
  return std::format("is between {} and {}", minimum_.ToString(),
                     maximum_.ToString());
}

bool Between::IsSatisfiedWith(LookupVariableFn lookup_variable,
                              int64_t value) const {
  absl::StatusOr<std::optional<Range::ExtremeValues>> extremes =
      GetRange().Extremes(lookup_variable);
  if (!extremes.ok()) return false;
  if (!extremes->has_value()) return false;
  return (*extremes)->min <= value && value <= (*extremes)->max;
}

std::string Between::Explanation(LookupVariableFn lookup_variable,
                                 int64_t value) const {
  return std::format("is not between {} and {}", minimum_.ToString(),
                     maximum_.ToString());
}

// -----------------------------------------------------------------------------
//  AtMost

AtMost::AtMost(int64_t maximum)
    : maximum_(*ParseExpression(std::to_string(maximum))) {}

AtMost::AtMost(IntegerExpression maximum)
    : maximum_(*ParseExpression(maximum)) {}

Range AtMost::GetRange() const {
  Range r;
  r.AtMost(maximum_.ToString()).IgnoreError();
  return r;
}

std::string AtMost::ToString() const {
  return std::format("is at most {}", maximum_.ToString());
}

bool AtMost::IsSatisfiedWith(LookupVariableFn lookup_variable,
                             int64_t value) const {
  absl::StatusOr<std::optional<Range::ExtremeValues>> extremes =
      GetRange().Extremes(lookup_variable);
  if (!extremes.ok()) return false;
  if (!extremes->has_value()) return false;
  return value >= (*extremes)->min && value <= (*extremes)->max;
}

std::string AtMost::Explanation(LookupVariableFn lookup_variable,
                                int64_t value) const {
  return std::format("is not at most {}", maximum_.ToString());
}

// -----------------------------------------------------------------------------
//  AtLeast

AtLeast::AtLeast(int64_t minimum)
    : minimum_(*ParseExpression(std::to_string(minimum))) {}

AtLeast::AtLeast(IntegerExpression minimum)
    : minimum_(*ParseExpression(minimum)) {}

Range AtLeast::GetRange() const {
  Range r;
  r.AtLeast(minimum_.ToString()).IgnoreError();
  return r;
}

std::string AtLeast::ToString() const {
  return std::format("is at least {}", minimum_.ToString());
}

bool AtLeast::IsSatisfiedWith(LookupVariableFn lookup_variable,
                              int64_t value) const {
  absl::StatusOr<std::optional<Range::ExtremeValues>> extremes =
      GetRange().Extremes(lookup_variable);
  if (!extremes.ok()) return false;
  if (!extremes->has_value()) return false;
  return value >= (*extremes)->min && value <= (*extremes)->max;
}

std::string AtLeast::Explanation(LookupVariableFn lookup_variable,
                                 int64_t value) const {
  return std::format("is not at least {}", minimum_.ToString());
}

}  // namespace moriarty
