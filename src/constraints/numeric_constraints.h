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

#ifndef MORIARTY_VARIABLES_NUMERIC_CONSTRAINTS_H_
#define MORIARTY_VARIABLES_NUMERIC_CONSTRAINTS_H_

#include <concepts>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "src/constraints/base_constraints.h"
#include "src/constraints/constraint_violation.h"
#include "src/contexts/librarian_context.h"
#include "src/internal/expressions.h"
#include "src/internal/range.h"
#include "src/librarian/one_of_handler.h"
#include "src/types/real.h"

namespace moriarty {

// Constraint stating that the numeric value must be in the inclusive range
// [minimum, maximum]. Note that if `Real` is used for either `minimum` or
// `maximum`, then this constraint is not valid for MInteger.
//
// Examples:
//  Between(1, 10)
//  Between("3 * N + 1", "10^9")
//  Between(Real("0.5"), "10^12")
//  Between(Real("-1e6"), Real("1e6"))
class Between : public MConstraint {
 public:
  using IntegerExpression = std::string_view;

  // The numeric value must be in the inclusive range [minimum, maximum].
  explicit Between(int64_t minimum, int64_t maximum);
  // The numeric value must be in the inclusive range [minimum, maximum].
  explicit Between(int64_t minimum, IntegerExpression maximum);
  // The numeric value must be in the inclusive range [minimum, maximum].
  explicit Between(int64_t minimum, Real maximum);
  // The numeric value must be in the inclusive range [minimum, maximum].
  explicit Between(IntegerExpression minimum, int64_t maximum);
  // The numeric value must be in the inclusive range [minimum, maximum].
  explicit Between(IntegerExpression minimum, IntegerExpression maximum);
  // The numeric value must be in the inclusive range [minimum, maximum].
  explicit Between(IntegerExpression minimum, Real maximum);
  // The numeric value must be in the inclusive range [minimum, maximum].
  explicit Between(Real minimum, int64_t maximum);
  // The numeric value must be in the inclusive range [minimum, maximum].
  explicit Between(Real minimum, IntegerExpression maximum);
  // The numeric value must be in the inclusive range [minimum, maximum].
  explicit Between(Real minimum, Real maximum);

  // No floating point values are allowed. Use `Real()`.
  Between(std::floating_point auto, auto) = delete;
  // No floating point values are allowed. Use `Real()`.
  Between(auto, std::floating_point auto) = delete;

  // Returns the range of values that this constraint represents.
  [[nodiscard]] Range GetRange() const;

  // Returns a human-readable representation of this constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns true if `[minimum] <= value <= [maximum]`.
  ConstraintViolation CheckValue(librarian::AnalysisContext ctx,
                                 int64_t value) const;
  ConstraintViolation CheckValue(librarian::AnalysisContext ctx,
                                 int value) const;

  // Returns true if `[minimum] <= value <= [maximum]`.
  ConstraintViolation CheckValue(librarian::AnalysisContext ctx,
                                 double value) const;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const;

 private:
  std::variant<int64_t, Expression, Real> minimum_;
  std::variant<int64_t, Expression, Real> maximum_;
  std::vector<std::string> dependencies_;
};

// Constraint stating that the numeric value must be this value or smaller.
class AtMost : public MConstraint {
 public:
  using IntegerExpression = std::string_view;

  // The numeric value must be this value or smaller. E.g., AtMost(123)
  explicit AtMost(int64_t maximum);

  // The numeric value must be this value or smaller.
  // E.g., AtMost("10^9") or AtMost("3 * N + 1")
  explicit AtMost(IntegerExpression maximum);

  // The numeric value must be this value or smaller.
  // E.g., AtMost(Real("0.5")) or AtMost(Real("-1e6"))
  explicit AtMost(Real maximum);

  // No floating point values are allowed. Use `Real()`.
  AtMost(std::floating_point auto) = delete;

  // Returns the range of values that this constraint represents.
  [[nodiscard]] Range GetRange() const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns true if `value <= [maximum]`.
  ConstraintViolation CheckValue(librarian::AnalysisContext ctx,
                                 int64_t value) const;
  ConstraintViolation CheckValue(librarian::AnalysisContext ctx,
                                 int value) const;

  // Returns true if `value <= [maximum]`.
  ConstraintViolation CheckValue(librarian::AnalysisContext ctx,
                                 double value) const;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const;

 private:
  std::variant<int64_t, Expression, Real> maximum_;
  std::vector<std::string> dependencies_;
};

// Constraint stating that the numeric value must be this value or larger.
class AtLeast : public MConstraint {
 public:
  using IntegerExpression = std::string_view;

  // The numeric value must be this value or larger. E.g., AtLeast(123)
  explicit AtLeast(int64_t minimum);

  // The numeric value must be this value or larger.
  // E.g., AtLeast("10^9") or AtLeast("3 * N + 1")
  explicit AtLeast(IntegerExpression minimum);

  // The numeric value must be this value or larger.
  // E.g., AtLeast(Real("0.5")) or AtLeast(Real("-1e6"))
  explicit AtLeast(Real minimum);

  // No floating point values are allowed. Use `Real()`.
  AtLeast(std::floating_point auto) = delete;

  // Returns the range of values that this constraint represents.
  [[nodiscard]] Range GetRange() const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns true if the given value satisfies this constraint.
  ConstraintViolation CheckValue(librarian::AnalysisContext ctx,
                                 int64_t value) const;
  ConstraintViolation CheckValue(librarian::AnalysisContext ctx,
                                 int value) const;

  // Returns true if `[minimum] <= value`.
  ConstraintViolation CheckValue(librarian::AnalysisContext ctx,
                                 double value) const;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const;

 private:
  std::variant<int64_t, Expression, Real> minimum_;
  std::vector<std::string> dependencies_;
};

namespace librarian {

// Constraint stating that the variable must be exactly this value. This allows
// any numeric type (int, Real, IntegerExpression).
class ExactlyNumeric : public MConstraint {
 public:
  using IntegerExpression = std::string_view;

  explicit ExactlyNumeric(int64_t value);
  explicit ExactlyNumeric(Real value);
  explicit ExactlyNumeric(IntegerExpression value);

  [[nodiscard]] Range GetRange() const;
  [[nodiscard]] std::string ToString() const;
  ConstraintViolation CheckValue(librarian::AnalysisContext ctx,
                                 int64_t value) const;
  ConstraintViolation CheckValue(librarian::AnalysisContext ctx,
                                 int value) const;
  ConstraintViolation CheckValue(librarian::AnalysisContext ctx,
                                 double value) const;
  [[nodiscard]] std::vector<std::string> GetDependencies() const;

  std::variant<int64_t, Expression, Real> GetValue() const;

 private:
  std::variant<int64_t, Expression, Real> value_;
  std::vector<std::string> dependencies_;
};

// Constraint stating that the variable must be one of these values. This allows
// any numeric type (int, Real, IntegerExpression).
//
// This class is a specialization of `OneOfHandler`, but for numeric values.
// The API is slightly different.
class OneOfNumeric : public MConstraint {
 public:
  using IntegerExpression = std::string_view;

  OneOfNumeric() = default;  // No options, empty set.
  explicit OneOfNumeric(std::span<const std::string> options);
  explicit OneOfNumeric(std::span<const IntegerExpression> options);
  explicit OneOfNumeric(std::span<const Real> options);
  explicit OneOfNumeric(std::span<const int64_t> options);

  [[nodiscard]] bool HasBeenConstrained() const;
  [[nodiscard]] std::optional<Real> GetUniqueValue(
      librarian::AnalysisContext fn) const;

  [[nodiscard]] bool ConstrainOptions(const OneOfNumeric& other);
  [[nodiscard]] bool ConstrainOptions(
      std::span<const IntegerExpression> options);
  [[nodiscard]] bool ConstrainOptions(std::span<const Real> options);
  [[nodiscard]] bool ConstrainOptions(std::span<const int64_t> options);
  [[nodiscard]] bool ConstrainOptions(IntegerExpression other);
  [[nodiscard]] bool ConstrainOptions(Real other);
  [[nodiscard]] bool ConstrainOptions(int64_t other);
  [[nodiscard]] bool ConstrainOptions(const ExactlyNumeric& other);

  // Similar to OneOfHandler::HasOption()
  ConstraintViolation CheckValue(librarian::AnalysisContext ctx,
                                 int64_t value) const;
  ConstraintViolation CheckValue(librarian::AnalysisContext ctx,
                                 int value) const;
  ConstraintViolation CheckValue(librarian::AnalysisContext ctx,
                                 double value) const;

  [[nodiscard]] std::vector<Real> GetOptionsLookup(
      std::function<int64_t(std::string_view)> lookup_fn) const;
  [[nodiscard]] std::vector<Real> GetOptions(
      librarian::AnalysisContext ctx) const;
  [[nodiscard]] std::string ToString() const;
  [[nodiscard]] std::vector<std::string> GetDependencies() const;

 private:
  OneOfHandler<Real> numeric_options_;

  // Each element represents a list of expressions that the value must come
  // from. We must handle it like this since we do not know the value of
  // variables.
  std::vector<std::vector<Expression>> expr_options_;
  std::vector<std::string> dependencies_;
};

}  // namespace librarian
}  // namespace moriarty

#endif  // MORIARTY_VARIABLES_NUMERIC_CONSTRAINTS_H_
