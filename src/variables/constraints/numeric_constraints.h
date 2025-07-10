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

#ifndef MORIARTY_SRC_VARIABLES_NUMERIC_CONSTRAINTS_H_
#define MORIARTY_SRC_VARIABLES_NUMERIC_CONSTRAINTS_H_

#include <concepts>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "src/internal/expressions.h"
#include "src/internal/range.h"
#include "src/librarian/errors.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/constraints/constraint_violation.h"
#include "src/variables/real.h"

namespace moriarty {

class NumericRangeMConstraint : public MConstraint {
 public:
  using LookupVariableFn = std::function<int64_t(std::string_view)>;
  using IntegerExpression = std::string_view;

  virtual ~NumericRangeMConstraint() = default;
  virtual std::string ToString() const = 0;
  virtual ConstraintViolation CheckIntegerValue(
      LookupVariableFn lookup_variable, int64_t value) const = 0;
  virtual ConstraintViolation CheckRealValue(LookupVariableFn lookup_variable,
                                             double value) const {
    throw InvalidConstraint(
        "CheckRealValue() is not implemented for this constraint");
  };
  virtual std::vector<std::string> GetDependencies() const = 0;
};

// Constraint stating that the variable must be exactly the value of this
// expression. E.g., `ExactlyIntegerExpression("3 * N + 1")`.
class ExactlyIntegerExpression : public NumericRangeMConstraint {
 public:
  // The numeric value must be exactly this value.
  explicit ExactlyIntegerExpression(IntegerExpression value);

  // Returns the range of values that this constraint represents.
  [[nodiscard]] Range GetRange() const;

  // Returns a human-readable representation of this constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns true if the given value satisfies this constraint.
  [[nodiscard]] ConstraintViolation CheckIntegerValue(
      LookupVariableFn lookup_variable, int64_t value) const;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const;

 private:
  Expression value_;
  std::vector<std::string> dependencies_;
};

// Constraint stating that the variable must be exactly the value of this
// expression. E.g., `OneOfIntegerExpression({"3 * N + 1", "14"})`.
class OneOfIntegerExpression : public NumericRangeMConstraint {
 public:
  // The numeric value must be exactly these values.
  template <typename Container>
  explicit OneOfIntegerExpression(const Container& options);
  explicit OneOfIntegerExpression(std::span<const IntegerExpression> options);
  explicit OneOfIntegerExpression(
      std::initializer_list<IntegerExpression> options);

  ~OneOfIntegerExpression() override = default;

  // Returns the options that this constraint represents.
  [[nodiscard]] std::vector<std::string> GetOptions() const;

  // Returns a human-readable representation of this constraint.
  [[nodiscard]] std::string ToString() const override;

  // Returns true if the given value satisfies this constraint.
  [[nodiscard]] ConstraintViolation CheckIntegerValue(
      LookupVariableFn lookup_variable, int64_t value) const override;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const override;

 private:
  std::vector<Expression> options_;
  std::vector<std::string> dependencies_;
};

// Constraint stating that the numeric value must be in the inclusive range
// [minimum, maximum]. Note that if `Real` is used for either `minimum` or
// `maximum`, then this constraint is not valid for MInteger.
//
// Examples:
//  Between(1, 10)
//  Between("3 * N + 1", "10^9")
//  Between(Real("0.5"), "10^12")
//  Between(Real("-1e6"), Real("1e6"))
class Between : public NumericRangeMConstraint {
 public:
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

  ~Between() override = default;

  // Returns the range of values that this constraint represents.
  [[nodiscard]] Range GetRange() const;

  // Returns a human-readable representation of this constraint.
  [[nodiscard]] std::string ToString() const override;

  // Returns true if `[minimum] <= value <= [maximum]`.
  [[nodiscard]] ConstraintViolation CheckIntegerValue(
      LookupVariableFn lookup_variable, int64_t value) const override;

  // Returns true if `[minimum] <= value <= [maximum]`.
  [[nodiscard]] ConstraintViolation CheckRealValue(
      LookupVariableFn lookup_variable, double value) const override;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const override;

 private:
  std::variant<int64_t, Expression, Real> minimum_;
  std::variant<int64_t, Expression, Real> maximum_;
  std::vector<std::string> dependencies_;
};

// Constraint stating that the numeric value must be this value or smaller.
class AtMost : public NumericRangeMConstraint {
 public:
  using LookupVariableFn = std::function<int64_t(std::string_view)>;

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

  ~AtMost() override = default;

  // Returns the range of values that this constraint represents.
  [[nodiscard]] Range GetRange() const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const override;

  // Returns true if `value <= [maximum]`.
  [[nodiscard]] ConstraintViolation CheckIntegerValue(
      LookupVariableFn lookup_variable, int64_t value) const override;

  // Returns true if `value <= [maximum]`.
  [[nodiscard]] ConstraintViolation CheckRealValue(
      LookupVariableFn lookup_variable, double value) const override;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const override;

 private:
  std::variant<int64_t, Expression, Real> maximum_;
  std::vector<std::string> dependencies_;
};

// Constraint stating that the numeric value must be this value or larger.
class AtLeast : public NumericRangeMConstraint {
 public:
  using LookupVariableFn = std::function<int64_t(std::string_view)>;

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

  ~AtLeast() override = default;

  // Returns the range of values that this constraint represents.
  [[nodiscard]] Range GetRange() const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const override;

  // Returns true if the given value satisfies this constraint.
  [[nodiscard]] ConstraintViolation CheckIntegerValue(
      LookupVariableFn lookup_variable, int64_t value) const override;

  // Returns true if `[minimum] <= value`.
  [[nodiscard]] ConstraintViolation CheckRealValue(
      LookupVariableFn lookup_variable, double value) const override;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const override;

 private:
  std::variant<int64_t, Expression, Real> minimum_;
  std::vector<std::string> dependencies_;
};

// -----------------------------------------------------------------------------
//  Template implementation below

template <typename Container>
OneOfIntegerExpression::OneOfIntegerExpression(const Container& options) {
  for (const std::string& option : options)
    options_.push_back(Expression(option));
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_NUMERIC_CONSTRAINTS_H_
