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

#ifndef MORIARTY_SRC_TEST_CASE_H_
#define MORIARTY_SRC_TEST_CASE_H_

#include <any>
#include <string_view>
#include <utility>

#include "src/internal/abstract_variable.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"

namespace moriarty {

// FIXME: Remove after Context refactor
struct TCInternals {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
};

// TestCase
//
// A collection of variables representing a single test case. If you want to
// test your system with 5 inputs, there should be 5 `Case`s. See Moriarty and
// Generator documentation for more information.
class TestCase {
 public:
  // SetValue()
  //
  // Sets the variable `variable_name` to be exactly `value`. Example:
  //
  // TestCase()
  //    .SetValue<MString>("X", "hello")
  //    .SetValue<MInteger>("Y", 3);
  //
  // The following are logically equivalent:
  //  * SetValue<MType>("X", value);
  //  * ConstrainVariable("X", Exactly(value));
  //
  // See `ConstrainVariable()`.
  template <MoriartyVariable T>
  TestCase& SetValue(std::string_view variable_name, T::value_type value);

  // ConstrainVariable()
  //
  // Adds extra constraints to `variable_name`. `constraints` will be
  // merged with the variable at global context.
  //
  // Examples (2nd and 3rd are equivalent):
  //
  // TC1.ConstrainVariable("X", MInteger(Between(10, 20)));
  // TC2.ConstrainVariable("X", MInteger(Between(10, 15), Odd()));
  // TC3.ConstrainVariable("X", MInteger(Between(10, 15)))
  //    .ConstrainVariable("X", MInteger(Odd()));
  //
  // If "X" was `Between(1, 12)` in the global context, then it is one
  // of {10, 11, 12} now:
  //
  // TC.ConstrainVariable("X", MInteger(Between(10, 20)));
  //
  // See `SetValue()`.
  template <MoriartyVariable T>
  TestCase& ConstrainVariable(std::string_view variable_name, T constraints);

  // Adds extra constraints to a variable. This version is for when you do not
  // know the exact type of the variable. Prefer to use `ConstrainVariable()`.
  // This function may be removed in the future.
  TestCase& ConstrainAnonymousVariable(
      std::string_view variable_name,
      const moriarty_internal::AbstractVariable& constraints);

  // This is a dangerous function that should only be used if you know what
  // you're doing. `value` must have a particular memory layout, and if you pass
  // the wrong type, this will invoke undefined behaviour. Your program may or
  // may not not crash, and will likely give weird or inconsistent results.
  // Moreover, this function may be removed at any time.
  TestCase& UnsafeSetAnonymousValue(std::string_view variable_name,
                                    std::any value);

 private:
  moriarty_internal::VariableSet variables_;
  moriarty_internal::ValueSet values_;

  friend TCInternals UnsafeExtractTestCaseInternals(const TestCase& test_case);
};

// ConcreteTestCase
//
// Actual values for all variables of interest.
class ConcreteTestCase {
 public:
  // SetValue()
  //
  // Sets the variable `variable_name` to be exactly `value`. Example:
  //
  // ConcreteTestCase()
  //    .SetValue<MString>("X", "hello")
  //    .SetValue<MInteger>("Y", 3);
  template <MoriartyVariable T>
  ConcreteTestCase& SetValue(std::string_view variable_name,
                             T::value_type value);

  // GetValue()
  //
  // Returns the value of the variable `variable_name`.
  template <MoriartyVariable T>
  [[nodiscard]] T::value_type GetValue(std::string_view variable_name) const;

  // This is a dangerous function that should only be used if you know what
  // you're doing. `value` must have a particular memory layout, and if you pass
  // the wrong type, this will invoke undefined behaviour. Your program may or
  // may not not crash, and will likely give weird or inconsistent results.
  // Moreover, this function may be removed at any time.
  ConcreteTestCase& UnsafeSetAnonymousValue(std::string_view variable_name,
                                            std::any value);

  // This is a dangerous function that should only be used if you know what
  // you're doing. This is sometimes needed to pass a reference to a particular
  // context. Do not depend on this function as it may be removed at any point.
  moriarty_internal::ValueSet& UnsafeGetValues() &;

 private:
  moriarty_internal::ValueSet values_;

  friend moriarty_internal::ValueSet UnsafeExtractConcreteTestCaseInternals(
      const ConcreteTestCase& test_case);
  friend void UnsafeSetConcreteTestCaseInternals(
      ConcreteTestCase& test_case, moriarty_internal::ValueSet values);
};

// -----------------------------------------------------------------------------
//  Template implementation below

template <MoriartyVariable T>
TestCase& TestCase::SetValue(std::string_view variable_name,
                             T::value_type value) {
  values_.Set<T>(variable_name, std::move(value));
  return *this;
}

template <MoriartyVariable T>
TestCase& TestCase::ConstrainVariable(std::string_view variable_name,
                                      T constraints) {
  variables_.AddOrMergeVariable(variable_name, std::move(constraints));
  return *this;
}

template <MoriartyVariable T>
ConcreteTestCase& ConcreteTestCase::SetValue(std::string_view variable_name,
                                             T::value_type value) {
  values_.Set<T>(variable_name, std::move(value));
  return *this;
}

template <MoriartyVariable T>
T::value_type ConcreteTestCase::GetValue(std::string_view variable_name) const {
  return values_.Get<T>(variable_name);
}

}  // namespace moriarty

namespace moriarty {
// Convenience functions for internal use only.

TCInternals UnsafeExtractTestCaseInternals(const TestCase& test_case);
moriarty_internal::ValueSet UnsafeExtractConcreteTestCaseInternals(
    const ConcreteTestCase& test_case);
void UnsafeSetConcreteTestCaseInternals(ConcreteTestCase& test_case,
                                        moriarty_internal::ValueSet values);

}  // namespace moriarty

#endif  // MORIARTY_SRC_TEST_CASE_H_
