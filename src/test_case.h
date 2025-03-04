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
#include <concepts>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/mvariable.h"
#include "src/scenario.h"

namespace moriarty {

// FIXME: Remove after Context refactor
struct TCInternals {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  std::vector<Scenario> scenarios;
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
  // Sets the variable `variable_name` to a specific `value`. The MVariable of
  // `variable_name` must match the MVariable at the global context.
  //
  // Example:
  //
  //     TestCase()
  //        .SetValue<MString>("X", "hello")
  //        .SetValue<MInteger>("Y", 3);
  //
  // The following are logically equivalent:
  //  * SetValue<MCustomType>("X", value);
  //  * ConstrainVariable("X", Exactly(value));
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  TestCase& SetValue(absl::string_view variable_name, T::value_type value);

  // ConstrainVariable()
  //
  // Adds extra constraints to `variable_name`. `constraints` will be
  // merged with the variable at global context.
  //
  // Examples:
  //
  //     // X is one of {10, 11, 12, ... , 20}.
  //     TestCase().ConstrainVariable("X", MInteger().Between(10, 20));
  //
  //     // X is one of {11, 13, 15}.
  //     TestCase()
  //         .ConstrainVariable("X", MInteger().Between(10, 15).IsOdd());
  //
  //     // Equivalent to above. X is one of {11, 13, 15}.
  //     TestCase()
  //         .ConstrainVariable("X", MInteger().Between(10, 15))
  //         .ConstrainVariable("X", MInteger().IsOdd());
  //
  //     // If "X" was `Between(1, 12)` in the global context, then it is one
  //     // of {10, 11, 12} now.
  //     TestCase().ConstrainVariable("X", MInteger().Between(10, 20));
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  TestCase& ConstrainVariable(absl::string_view variable_name, T constraints);

  // ConstrainAnonymousVariable()
  //
  // May be deprecated in the future. Prefer to use `ConstrainVariable()`.
  //
  // Adds extra constraints to a variable. This version is for when you do not
  // know the exact type of the variable.
  TestCase& ConstrainAnonymousVariable(
      absl::string_view variable_name,
      const moriarty_internal::AbstractVariable& constraints);

  // This is a dangerous function that should only be used if you know what
  // you're doing. `value` must have a particular memory layout, and if you pass
  // the wrong type, this will invoke undefined behaviour. Your program may or
  // may not not crash, and will likely give weird or inconsistent results.
  TestCase& UnsafeSetAnonymousValue(std::string_view variable_name,
                                    std::any value);

  // WithScenario()
  //
  // This TestCase is covering this `scenario`.
  TestCase& WithScenario(Scenario scenario);

  // FIXME: Remove after Context refactor
  void SetVariables(moriarty_internal::VariableSet variables) {
    variables_ = std::move(variables);
  }

 private:
  moriarty_internal::VariableSet variables_;
  moriarty_internal::ValueSet values_;
  std::vector<Scenario> scenarios_;

  // Distributes all known scenarios to the variable set.
  absl::Status DistributeScenarios();

  friend TCInternals UnsafeExtractTestCaseInternals(const TestCase& test_case);
};

// ConcreteTestCase
//
// Actual values for all variables of interest.
class ConcreteTestCase {
 public:
  // SetValue()
  //
  // Sets the variable `variable_name` to a specific `value`.
  //
  // Example:
  //     ConcreteTestCase()
  //        .SetValue<MString>("X", "hello")
  //        .SetValue<MInteger>("Y", 3);
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  ConcreteTestCase& SetValue(absl::string_view variable_name,
                             T::value_type value);

  // This is a dangerous function that should only be used if you know what
  // you're doing. `value` must have a particular memory layout, and if you pass
  // the wrong type, this will invoke undefined behaviour. Your program may or
  // may not not crash, and will likely give weird or inconsistent results.
  ConcreteTestCase& UnsafeSetAnonymousValue(std::string_view variable_name,
                                            std::any value);

 private:
  moriarty_internal::ValueSet values_;

  friend moriarty_internal::ValueSet UnsafeExtractConcreteTestCaseInternals(
      const ConcreteTestCase& test_case);
};

// TestCaseMetadata
//
// Metadata about this test case. All 1-based.
//
// For example: 3 generators.
//  - A creates 4 test cases, run once (4 test cases total)
//  - B creates 3 test cases, run twice (6 test cases total)
//  - C creates 5 test cases, run three times (15 test cases total)
//
// Consider the 3rd test case that was added on the second run of C.
//
//  * GetTestCaseNumber() == 18 (4 + 2*3 + 5 + 3)
//  * GetGeneratedTestCaseMetadata().generator_name == "C"
//  * GetGeneratedTestCaseMetadata().generator_iteration == 2
//  * GetGeneratedTestCaseMetadata().case_number_in_generator = 3
class TestCaseMetadata {
 public:
  // Set the test case number for this TestCase. (1-based).
  TestCaseMetadata& SetTestCaseNumber(int test_case_number) {
    test_case_number_ = test_case_number;
    return *this;
  }

  // Which test case number is this? (1-based).
  [[nodiscard]] int GetTestCaseNumber() const { return test_case_number_; }

  // If the test case was generated, this is the important metadata that should
  // be considered. Fields may be added over time.
  struct GeneratedTestCaseMetadata {
    std::string generator_name;
    int generator_iteration;  // If this generator was called several times,
                              // which iteration?
    int case_number_in_generator;  // Which call of `AddTestCase()` was this?
                                   // (1-based)
  };
  TestCaseMetadata& SetGeneratorMetadata(
      GeneratedTestCaseMetadata generator_metadata) {
    generator_metadata_ = std::move(generator_metadata);
    return *this;
  }

  // If this TestCase was generated, return the metadata associated with it.
  const std::optional<GeneratedTestCaseMetadata>& GetGeneratorMetadata() const {
    return generator_metadata_;
  }

 private:
  int test_case_number_;
  std::optional<GeneratedTestCaseMetadata> generator_metadata_;
};

// -----------------------------------------------------------------------------
//  Template implementation below

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
TestCase& TestCase::SetValue(absl::string_view variable_name,
                             T::value_type value) {
  values_.Set<T>(variable_name, std::move(value));
  return *this;
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
TestCase& TestCase::ConstrainVariable(absl::string_view variable_name,
                                      T constraints) {
  ABSL_CHECK_OK(
      variables_.AddOrMergeVariable(variable_name, std::move(constraints)));
  return *this;
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
ConcreteTestCase& ConcreteTestCase::SetValue(absl::string_view variable_name,
                                             T::value_type value) {
  values_.Set<T>(variable_name, std::move(value));
  return *this;
}

}  // namespace moriarty

namespace moriarty {
// Convenience functions for internal use only.

TCInternals UnsafeExtractTestCaseInternals(const TestCase& test_case);
moriarty_internal::ValueSet UnsafeExtractConcreteTestCaseInternals(
    const ConcreteTestCase& test_case);

}  // namespace moriarty

#endif  // MORIARTY_SRC_TEST_CASE_H_
