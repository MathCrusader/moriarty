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

#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "src/internal/expressions.h"
#include "src/internal/range.h"
#include "src/variables/constraints/base_constraints.h"

namespace moriarty {

class IntegerRangeMConstraint : public MConstraint {
 public:
  using LookupVariableFn = std::function<int64_t(std::string_view)>;
  using IntegerExpression = std::string_view;

  virtual ~IntegerRangeMConstraint() = default;
  virtual std::string ToString() const = 0;
  virtual bool IsSatisfiedWith(LookupVariableFn lookup_variable,
                               int64_t value) const = 0;
  virtual std::string UnsatisfiedReason(LookupVariableFn lookup_variable,
                                        int64_t value) const = 0;
  virtual std::vector<std::string> GetDependencies() const = 0;
};

// Constraint stating that the variable must be exactly the value of this
// expression. E.g., `ExactlyIntegerExpression("3 * N + 1")`.
class ExactlyIntegerExpression : public IntegerRangeMConstraint {
 public:
  // The numeric value must be exactly this value.
  explicit ExactlyIntegerExpression(IntegerExpression value);

  // Returns the range of values that this constraint represents.
  [[nodiscard]] Range GetRange() const;

  // Returns a human-readable representation of this constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns true if the given value satisfies this constraint.
  [[nodiscard]] bool IsSatisfiedWith(LookupVariableFn lookup_variable,
                                     int64_t value) const;

  // Gives a human-readable explanation of why value does not satisfy the
  // constraints. Precondition: IsSatisfiedWith() == false;
  [[nodiscard]] std::string UnsatisfiedReason(LookupVariableFn lookup_variable,
                                              int64_t value) const;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const;

 private:
  Expression value_;
  std::vector<std::string> dependencies_;
};

// Constraint stating that the variable must be exactly the value of this
// expression. E.g., `OneOfIntegerExpression({"3 * N + 1", "14"})`.
class OneOfIntegerExpression : public IntegerRangeMConstraint {
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
  [[nodiscard]] bool IsSatisfiedWith(LookupVariableFn lookup_variable,
                                     int64_t value) const override;

  // Gives a human-readable explanation of why value does not satisfy the
  // constraints. Precondition: IsSatisfiedWith() == false;
  [[nodiscard]] std::string UnsatisfiedReason(LookupVariableFn lookup_variable,
                                              int64_t value) const override;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const override;

 private:
  std::vector<Expression> options_;
  std::vector<std::string> dependencies_;
};

// Constraint stating that the numeric value must be in the inclusive range
// [minimum, maximum].
class Between : public IntegerRangeMConstraint {
 public:
  // The numeric value must be in the inclusive range [minimum, maximum].
  explicit Between(int64_t minimum, int64_t maximum);

  // The numeric value must be in the inclusive range [minimum, maximum].
  explicit Between(IntegerExpression minimum, IntegerExpression maximum);

  // The numeric value must be in the inclusive range [minimum, maximum].
  explicit Between(int64_t minimum, IntegerExpression maximum);

  // The numeric value must be in the inclusive range [minimum, maximum].
  explicit Between(IntegerExpression minimum, int64_t maximum);

  ~Between() override = default;

  // Returns the range of values that this constraint represents.
  [[nodiscard]] Range GetRange() const;

  // Returns a human-readable representation of this constraint.
  [[nodiscard]] std::string ToString() const override;

  // Returns true if the given value satisfies this constraint.
  [[nodiscard]] bool IsSatisfiedWith(LookupVariableFn lookup_variable,
                                     int64_t value) const override;

  // Gives a human-readable explanation of why value does not satisfy the
  // constraints. Precondition: IsSatisfiedWith() == false;
  [[nodiscard]] std::string UnsatisfiedReason(LookupVariableFn lookup_variable,
                                              int64_t value) const override;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const override;

 private:
  Expression minimum_;
  Expression maximum_;
  std::vector<std::string> dependencies_;
};

// Constraint stating that the numeric value must be this value or smaller.
class AtMost : public IntegerRangeMConstraint {
 public:
  using LookupVariableFn = std::function<int64_t(std::string_view)>;

  // The numeric value must be this value or smaller. E.g., AtMost(123)
  explicit AtMost(int64_t maximum);

  // The numeric value must be this value or smaller.
  // E.g., AtMost("10^9") or AtMost("3 * N + 1")
  explicit AtMost(IntegerExpression maximum);

  ~AtMost() override = default;

  // Returns the range of values that this constraint represents.
  [[nodiscard]] Range GetRange() const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const override;

  // Returns true if the given value satisfies this constraint.
  [[nodiscard]] bool IsSatisfiedWith(LookupVariableFn lookup_variable,
                                     int64_t value) const override;

  // Gives a human-readable explanation of why value does not satisfy the
  // constraints. Precondition: IsSatisfiedWith() == false;
  [[nodiscard]] std::string UnsatisfiedReason(LookupVariableFn lookup_variable,
                                              int64_t value) const override;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const override;

 private:
  Expression maximum_;
  std::vector<std::string> dependencies_;
};

// Constraint stating that the numeric value must be this value or larger.
class AtLeast : public IntegerRangeMConstraint {
 public:
  using LookupVariableFn = std::function<int64_t(std::string_view)>;

  // The numeric value must be this value or larger. E.g., AtLeast(123)
  explicit AtLeast(int64_t minimum);

  // The numeric value must be this value or larger.
  // E.g., AtLeast("10^9") or AtLeast("3 * N + 1")
  explicit AtLeast(IntegerExpression minimum);

  ~AtLeast() override = default;

  // Returns the range of values that this constraint represents.
  [[nodiscard]] Range GetRange() const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const override;

  // Returns true if the given value satisfies this constraint.
  [[nodiscard]] bool IsSatisfiedWith(LookupVariableFn lookup_variable,
                                     int64_t value) const override;

  // Gives a human-readable explanation of why value does not satisfy the
  // constraints. Precondition: IsSatisfiedWith() == false;
  [[nodiscard]] std::string UnsatisfiedReason(LookupVariableFn lookup_variable,
                                              int64_t value) const override;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const override;

 private:
  Expression minimum_;
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
