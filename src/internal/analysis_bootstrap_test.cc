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

#include "src/internal/analysis_bootstrap.h"

#include <optional>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/internal/value_set.h"
#include "src/testing/mtest_type.h"
#include "src/testing/status_test_util.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/constraints/base_constraints.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

using ::moriarty_testing::MTestType;
using ::moriarty_testing::TestType;
using ::moriarty_testing::ThrowsValueNotFound;
using ::testing::HasSubstr;
using ::testing::Optional;

TEST(AnalysisBootstrapTest,
     AllVariablesSatisfyConstraintsSucceedsWithNoVariables) {
  VariableSet variables;
  ValueSet values;
  EXPECT_EQ(AllVariablesSatisfyConstraints(variables, values), std::nullopt);
}

TEST(AnalysisBootstrapTest,
     AllVariablesSatisfyConstraintsSucceedsInNormalCase) {
  VariableSet variables;
  ValueSet values;

  std::vector<TestType> options;
  for (int i = 0; i < 100; i++) options.push_back(TestType(i));
  MORIARTY_ASSERT_OK(
      variables.AddVariable("A", MTestType().AddConstraint(OneOf(options))));
  MORIARTY_ASSERT_OK(
      variables.AddVariable("B", MTestType().AddConstraint(OneOf(options))));

  values.Set<MTestType>("A", options[4]);
  values.Set<MTestType>("B", options[53]);

  EXPECT_EQ(AllVariablesSatisfyConstraints(variables, values), std::nullopt);
}

TEST(AnalysisBootstrapTest,
     AllVariablesSatisfyConstraintsFailIfAtLeastOneValueFails) {
  VariableSet variables;
  ValueSet values;

  std::vector<TestType> options;
  for (int i = 0; i < 100; i++) options.push_back(TestType(i));
  MORIARTY_ASSERT_OK(
      variables.AddVariable("A", MTestType().AddConstraint(OneOf(options))));
  MORIARTY_ASSERT_OK(
      variables.AddVariable("B", MTestType().AddConstraint(OneOf(options))));

  values.Set<MTestType>("A", options[4]);
  values.Set<MTestType>("B", TestType(100000));  // Not in the list!

  EXPECT_THAT(AllVariablesSatisfyConstraints(variables, values),
              Optional(HasSubstr("B")));
}

TEST(AnalysisBootstrapTest,
     AllVariablesSatisfyConstraintsFailIfAnyValueIsMissing) {
  VariableSet variables;
  ValueSet values;

  std::vector<TestType> options;
  for (int i = 0; i < 100; i++) options.push_back(TestType(i));
  MORIARTY_ASSERT_OK(
      variables.AddVariable("A", MTestType().AddConstraint(OneOf(options))));
  MORIARTY_ASSERT_OK(
      variables.AddVariable("B", MTestType().AddConstraint(OneOf(options))));

  values.Set<MTestType>("A", options[4]);

  // FIXME: Determine semantics of satisfies constraints when a value is
  // missing.
  EXPECT_THAT([&] { (void)AllVariablesSatisfyConstraints(variables, values); },
              ThrowsValueNotFound("B"));
}

TEST(AnalysisBootstrapTest,
     AllVariablesSatisfyConstraintsSucceedsIfThereAreExtraValues) {
  VariableSet variables;
  ValueSet values;

  std::vector<TestType> options;
  for (int i = 0; i < 100; i++) options.push_back(TestType(i));
  MORIARTY_ASSERT_OK(
      variables.AddVariable("A", MTestType().AddConstraint(OneOf(options))));
  MORIARTY_ASSERT_OK(
      variables.AddVariable("B", MTestType().AddConstraint(OneOf(options))));

  values.Set<MTestType>("A", options[30]);
  values.Set<MTestType>("B", options[40]);
  values.Set<MTestType>("C", options[50]);

  EXPECT_EQ(AllVariablesSatisfyConstraints(variables, values), std::nullopt);
}

TEST(AnalysisBootstrapTest,
     AllVariablesSatisfyConstraintsWorksForDependentVariables) {
  VariableSet variables;
  ValueSet values;

  MORIARTY_ASSERT_OK(variables.AddVariable("A", MTestType()));
  MORIARTY_ASSERT_OK(variables.AddVariable("B", MTestType().SetAdder("A")));

  TestType valA = MTestType::kGeneratedValue;
  TestType valB = 2 * MTestType::kGeneratedValue;  // One by default, one for A

  values.Set<MTestType>("A", valA);
  values.Set<MTestType>("B", valB);

  EXPECT_EQ(AllVariablesSatisfyConstraints(variables, values), std::nullopt);
}

TEST(AnalysisBootstrapTest,
     AllVariablesSatisfyConstraintsFailsIfMissingValues) {
  VariableSet variables;
  ValueSet values;

  MORIARTY_ASSERT_OK(variables.AddVariable("A", MTestType()));

  EXPECT_THAT([&] { (void)AllVariablesSatisfyConstraints(variables, values); },
              ThrowsValueNotFound("A"));
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
