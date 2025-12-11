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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/context.h"
#include "src/internal/combinatorial_coverage.h"
#include "src/internal/combinatorial_coverage_test_util.h"
#include "src/internal/generation_bootstrap.h"
#include "src/internal/random_engine.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/testing/gtest_helpers.h"
#include "src/librarian/testing/mtest_type.h"

namespace moriarty {

using ::moriarty::CombinatorialCoverage;
using ::moriarty::moriarty_internal::ValueSet;
using ::moriarty_testing::Context;
using ::moriarty_testing::MTestType;

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
  for (moriarty_internal::ValueSet& values : cases) {
    CoveringArrayTestCase catc;

    catc.test_case = {
        MapValueToTestCaseNumber(values.Get<MTestType>("X").value),
        MapValueToTestCaseNumber(values.Get<MTestType>("Y").value)};
    cov_array.push_back(catc);
  }
  return cov_array;
}

TEST(CombinatorialCoverage, GenerateShouldCreateCasesFromCoveringArray) {
  Context context =
      Context().WithVariable("X", MTestType()).WithVariable("Y", MTestType());

  moriarty_internal::RandomEngine rng({1, 2, 3, 4}, "v0.1");
  GenerateContext ctx(context.Variables(), context.Values(), rng);

  std::vector<MTestCase> test_cases = CombinatorialCoverage(ctx);
  std::vector<ValueSet> generated_cases;
  for (const MTestCase& test_case : test_cases) {
    auto [extra_constraints, values] =
        UnsafeExtractTestCaseInternals(test_case);
    generated_cases.push_back(moriarty_internal::GenerateAllValues(
        context.Variables(), extra_constraints, values, {rng}));
  }

  EXPECT_THAT(CasesToCoveringArray(generated_cases),
              IsStrength2CoveringArray(std::vector<int>({2, 2})));
}

}  // namespace
}  // namespace moriarty
