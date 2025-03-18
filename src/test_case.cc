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

#include "src/test_case.h"

#include <stdint.h>

#include <string_view>
#include <utility>

#include "src/internal/abstract_variable.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"

namespace moriarty {

TestCase& TestCase::ConstrainAnonymousVariable(
    std::string_view variable_name,
    const moriarty_internal::AbstractVariable& constraints) {
  variables_.AddOrMergeVariable(variable_name, constraints);
  return *this;
}

TestCase& TestCase::UnsafeSetAnonymousValue(std::string_view variable_name,
                                            std::any value) {
  values_.UnsafeSet(variable_name, std::move(value));
  return *this;
}

ConcreteTestCase& ConcreteTestCase::UnsafeSetAnonymousValue(
    std::string_view variable_name, std::any value) {
  values_.UnsafeSet(variable_name, std::move(value));
  return *this;
}

TCInternals UnsafeExtractTestCaseInternals(const TestCase& test_case) {
  return {test_case.variables_, test_case.values_};
}

moriarty_internal::ValueSet UnsafeExtractConcreteTestCaseInternals(
    const ConcreteTestCase& test_case) {
  return test_case.values_;
}

void UnsafeSetConcreteTestCaseInternals(ConcreteTestCase& test_case,
                                        moriarty_internal::ValueSet values) {
  test_case.values_ = std::move(values);
}

}  // namespace moriarty
