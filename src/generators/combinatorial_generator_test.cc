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

#include "src/generators/combinatorial_generator.h"

#include <vector>

#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/context.h"
#include "src/internal/combinatorial_coverage.h"
#include "src/internal/combinatorial_coverage_test_util.h"
#include "src/internal/generation_bootstrap.h"
#include "src/internal/random_engine.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/testing/mtest_type.h"
#include "src/util/test_status_macro/status_testutil.h"

namespace moriarty {

using ::moriarty::CombinatorialCoverage;
using ::moriarty::moriarty_internal::GenerateTestCase;
using ::moriarty::moriarty_internal::ValueSet;

namespace {

int MapValueToTestCaseNumber(int val) {
  // These numbers come from the difficult cases implementation in MTestType.
  // This only works since they share the same cases.
  switch (val) {
    case 2:
      return 0;
    case 3:
      return 1;
    default:
      return -1;
  }
}

std::vector<CoveringArrayTestCase> CasesToCoveringArray(
    std::vector<moriarty_internal::ValueSet> cases) {
  std::vector<CoveringArrayTestCase> cov_array;
  for (moriarty_internal::ValueSet& valset : cases) {
    CoveringArrayTestCase catc;
    std::vector<int> dims;

    dims.push_back(MapValueToTestCaseNumber(
        valset.Get<moriarty_testing::MTestType>("X").value().value));
    dims.push_back(MapValueToTestCaseNumber(
        valset.Get<moriarty_testing::MTestType>("Y").value().value));
    catc.test_case = dims;
    cov_array.push_back(catc);
  }
  return cov_array;
}

TEST(CombinatorialCoverage, GenerateShouldCreateCasesFromCoveringArray) {
  moriarty_internal::ValueSet values;
  moriarty_internal::VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("X", moriarty_testing::MTestType()));
  MORIARTY_ASSERT_OK(variables.AddVariable("Y", moriarty_testing::MTestType()));

  moriarty_internal::RandomEngine rng({1, 2, 3, 4}, "v0.1");
  GenerateContext ctx(variables, values, rng);

  std::vector<TestCase> test_cases = CombinatorialCoverage(ctx);
  std::vector<ValueSet> generated_cases;
  for (const TestCase& test_case : test_cases) {
    generated_cases.push_back(
        GenerateTestCase(test_case, variables, {rng, std::nullopt}));
  }

  EXPECT_THAT(CasesToCoveringArray(generated_cases),
              IsStrength2CoveringArray(std::vector<int>({2, 2})));
}

}  // namespace
}  // namespace moriarty
