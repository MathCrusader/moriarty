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

#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/scenario.h"
#include "src/util/status_macro/status_macros.h"

namespace moriarty {

TestCase& TestCase::ConstrainAnonymousVariable(
    absl::string_view variable_name,
    const moriarty_internal::AbstractVariable& constraints) {
  ABSL_CHECK_OK(variables_.AddOrMergeVariable(variable_name, constraints));
  return *this;
}

TestCase& TestCase::UnsafeSetAnonymousValue(std::string_view variable_name,
                                            std::any value) {
  values_.UnsafeSet(variable_name, std::move(value));
  return *this;
}

TestCase& TestCase::WithScenario(Scenario scenario) {
  scenarios_.push_back(std::move(scenario));
  return *this;
}

absl::Status TestCase::DistributeScenarios() {
  for (const Scenario& scenario : scenarios_) {
    MORIARTY_RETURN_IF_ERROR(variables_.WithScenario(scenario));
  }
  return absl::OkStatus();
}

TCInternals UnsafeExtractTestCaseInternals(const TestCase& test_case) {
  return {test_case.variables_, test_case.values_, test_case.scenarios_};
}

moriarty_internal::ValueSet UnsafeExtractConcreteTestCaseInternals(
    const ConcreteTestCase& test_case) {
  return test_case.values_;
}

}  // namespace moriarty
