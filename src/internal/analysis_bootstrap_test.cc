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
#include "src/librarian/test_utils.h"
#include "src/testing/mtest_type.h"
#include "src/testing/status_test_util.h"
#include "src/variables/constraints/base_constraints.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

using ::moriarty_testing::Context;
using ::moriarty_testing::MTestType;
using ::moriarty_testing::TestType;
using ::moriarty_testing::ThrowsValueNotFound;
using ::testing::HasSubstr;
using ::testing::Optional;

TEST(AnalysisBootstrapTest,
     AllVariablesSatisfyConstraintsSucceedsWithNoVariables) {
  Context context;
  EXPECT_EQ(
      AllVariablesSatisfyConstraints(context.Variables(), context.Values()),
      std::nullopt);
}

TEST(AnalysisBootstrapTest,
     AllVariablesSatisfyConstraintsSucceedsInNormalCase) {
  std::vector<TestType> options;
  for (int i = 0; i < 100; i++) options.push_back(TestType(i));
  Context context =
      Context()
          .WithVariable("A", MTestType().AddConstraint(OneOf(options)))
          .WithVariable("B", MTestType().AddConstraint(OneOf(options)))
          .WithValue<MTestType>("A", options[4])
          .WithValue<MTestType>("B", options[53]);

  EXPECT_EQ(
      AllVariablesSatisfyConstraints(context.Variables(), context.Values()),
      std::nullopt);
}

TEST(AnalysisBootstrapTest,
     AllVariablesSatisfyConstraintsFailIfAtLeastOneValueFails) {
  std::vector<TestType> options;
  for (int i = 0; i < 100; i++) options.push_back(TestType(i));

  Context context =
      Context()
          .WithVariable("A", MTestType().AddConstraint(OneOf(options)))
          .WithVariable("B", MTestType().AddConstraint(OneOf(options)))
          .WithValue<MTestType>("A", options[4])
          .WithValue<MTestType>("B", TestType(100000));  // Not in the list!

  EXPECT_THAT(
      AllVariablesSatisfyConstraints(context.Variables(), context.Values()),
      Optional(HasSubstr("B")));
}

TEST(AnalysisBootstrapTest,
     AllVariablesSatisfyConstraintsFailIfAnyValueIsMissing) {
  std::vector<TestType> options;
  for (int i = 0; i < 100; i++) options.push_back(TestType(i));
  Context context =
      Context()
          .WithVariable("A", MTestType().AddConstraint(OneOf(options)))
          .WithVariable("B", MTestType().AddConstraint(OneOf(options)))
          .WithValue<MTestType>("A", options[4]);

  // FIXME: Determine semantics of satisfies constraints when a value is
  // missing.
  EXPECT_THAT(
      [&] {
        (void)AllVariablesSatisfyConstraints(context.Variables(),
                                             context.Values());
      },
      ThrowsValueNotFound("B"));
}

TEST(AnalysisBootstrapTest,
     AllVariablesSatisfyConstraintsSucceedsIfThereAreExtraValues) {
  std::vector<TestType> options;
  for (int i = 0; i < 100; i++) options.push_back(TestType(i));
  Context context =
      Context()
          .WithVariable("A", MTestType().AddConstraint(OneOf(options)))
          .WithVariable("B", MTestType().AddConstraint(OneOf(options)))
          .WithValue<MTestType>("A", options[30])
          .WithValue<MTestType>("B", options[40])
          .WithValue<MTestType>("C", options[50]);

  EXPECT_EQ(
      AllVariablesSatisfyConstraints(context.Variables(), context.Values()),
      std::nullopt);
}

TEST(AnalysisBootstrapTest,
     AllVariablesSatisfyConstraintsWorksForDependentVariables) {
  // B gets one by default, plus one from A
  Context context =
      Context()
          .WithVariable("A", MTestType())
          .WithVariable("B", MTestType().SetAdder("A"))
          .WithValue<MTestType>("A", MTestType::kGeneratedValue)
          .WithValue<MTestType>("B", 2 * MTestType::kGeneratedValue);

  EXPECT_EQ(
      AllVariablesSatisfyConstraints(context.Variables(), context.Values()),
      std::nullopt);
}

TEST(AnalysisBootstrapTest,
     AllVariablesSatisfyConstraintsFailsIfMissingValues) {
  Context context = Context().WithVariable("A", MTestType());

  EXPECT_THAT(
      [&] {
        (void)AllVariablesSatisfyConstraints(context.Variables(),
                                             context.Values());
      },
      ThrowsValueNotFound("A"));
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
