/*
 * Copyright 2025 Darcy Best
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

#ifndef MORIARTY_SRC_TESTING_MTEST_TYPE_H_
#define MORIARTY_SRC_TESTING_MTEST_TYPE_H_

#include <stdint.h>

#include <optional>
#include <string>
#include <vector>

#include "src/constraints/base_constraints.h"
#include "src/constraints/constraint_violation.h"
#include "src/contexts/librarian_context.h"
#include "src/librarian/mvariable.h"
#include "src/variables/minteger.h"

namespace moriarty_testing {

// This file contains a fake data type and a fake Moriarty variable. They aren't
// meant to have interesting behaviours, just enough functionality that tests
// can use them.

// TestType [for internal tests only]
//
// Simple data type that behaves (almost) exactly like an `int`.
// The variable `MTestType` will generate this.
//   `int64_t` is to `MInteger` as `TestType` is to `MTestType`
struct TestType {
  // NOLINTNEXTLINE: FakeDataType *is* an integer (and is only used in testing)
  TestType(int val = 0) : value(val) {}

  // NOLINTNEXTLINE: FakeDataType *is* an integer (and is only used in testing)
  operator int() const { return value; }

  int value;
};

class LastDigit : public moriarty::MConstraint {
 public:
  explicit LastDigit(moriarty::MInteger digit) : digit_(std::move(digit)) {}
  moriarty::MInteger GetDigit() const { return digit_; }
  std::string ToString() const {
    return std::format("the last digit {}", digit_.ToString());
  }

  moriarty::ConstraintViolation CheckValue(
      moriarty::librarian::AnalysisContext ctx, const TestType& value) const {
    auto check = digit_.CheckValue(ctx, value.value % 10);
    if (check.IsOk()) return moriarty::ConstraintViolation::None();
    return moriarty::ConstraintViolation(
        std::format("the last digit of {} {}", value.value, check.Reason()));
  }
  std::vector<std::string> GetDependencies() const {
    return digit_.GetDependencies();
  }

 private:
  moriarty::MInteger digit_;
};

class NumberOfDigits : public moriarty::MConstraint {
 public:
  explicit NumberOfDigits(moriarty::MInteger num_digits)
      : num_digits_(num_digits) {}
  moriarty::MInteger GetDigit() const { return num_digits_; }
  std::string ToString() const {
    return std::format("the number of digits {}", num_digits_.ToString());
  }

  moriarty::ConstraintViolation CheckValue(
      moriarty::librarian::AnalysisContext ctx, const TestType& value) const {
    auto check =
        num_digits_.CheckValue(ctx, std::to_string(value.value).size());
    if (check.IsOk()) return moriarty::ConstraintViolation::None();
    return moriarty::ConstraintViolation(std::format(
        "the number of digits in {} {}", value.value, check.Reason()));
  }
  std::vector<std::string> GetDependencies() const {
    return num_digits_.GetDependencies();
  }

 private:
  moriarty::MInteger num_digits_;
};

// MTestType [for internal tests only]
//
// A bare bones Moriarty variable.
class MTestType : public moriarty::librarian::MVariable<MTestType, TestType> {
 public:
  // This value is the default value returned from Generate.
  static constexpr int64_t kGeneratedValue = 123456789;
  // This value is returned when Property size = "small"
  static constexpr int64_t kGeneratedValueSmallSize = 123;
  // This value is returned when Property size = "large"
  static constexpr int64_t kGeneratedValueLargeSize = 123456;
  // These two values are returned as corner cases.
  static constexpr int64_t kCorner1 = 99991;
  static constexpr int64_t kCorner2 = 99992;

  ~MTestType() override = default;

  template <typename... Constraints>
    requires(moriarty::ConstraintFor<MTestType, Constraints> && ...)
  MTestType(Constraints&&... constraints) {
    (AddConstraint(std::forward<Constraints>(constraints)), ...);
  }

  using MVariable<MTestType, TestType>::AddConstraint;  // Custom constraints
  MTestType& AddConstraint(moriarty::Exactly<TestType> constraint);
  MTestType& AddConstraint(moriarty::OneOf<TestType> constraint);
  MTestType& AddConstraint(LastDigit constraint);
  MTestType& AddConstraint(NumberOfDigits constraint);

  std::string Typename() const override { return "MTestType"; };

 private:
  std::optional<moriarty::MInteger> last_digit_;
  std::optional<moriarty::MInteger> num_digits_;

  // Always returns 123456789, but maybe slightly modified.
  TestType GenerateImpl(
      moriarty::librarian::ResolverContext ctx) const override;
  std::optional<TestType> GetUniqueValueImpl(
      moriarty::librarian::AnalysisContext ctx) const override;
  TestType ReadImpl(moriarty::librarian::ReaderContext ctx) const override;
  void PrintImpl(moriarty::librarian::PrinterContext ctx,
                 const TestType& value) const override;
  std::vector<MTestType> ListEdgeCasesImpl(
      moriarty::librarian::AnalysisContext ctx) const override;
};

}  // namespace moriarty_testing

#endif  // MORIARTY_SRC_TESTING_MTEST_TYPE_H_
