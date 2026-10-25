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

#include "moriarty/problem.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "moriarty/librarian/testing/gtest_helpers.h"
#include "moriarty/variables/minteger.h"
#include "moriarty/variables/mstring.h"

namespace moriarty {
namespace {

using ::moriarty_testing::GeneratedValuesAre;
using ::testing::_;
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::Ge;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Le;
using ::testing::Optional;
using ::testing::ThrowsMessage;
using ::testing::TypedEq;

TEST(ProblemTest, TitleShouldWork) {
  EXPECT_EQ(Problem().GetTitle(), std::nullopt);

  Problem problem(Title("Sample Problem"));
  EXPECT_THAT(problem.GetTitle(),
              Optional(TypedEq<std::string>("Sample Problem")));
}

TEST(ProblemTest, SeedShouldWork) {
  EXPECT_EQ(Problem().GetSeed(), std::nullopt);

  Problem problem(Seed("theseed"));
  EXPECT_THAT(problem.GetSeed(), Optional(TypedEq<std::string>("theseed")));
}

TEST(ProblemTest, VariablesShouldWork) {
  EXPECT_THAT(Problem().UnsafeGetVariables().ListVariables(), IsEmpty());

  Problem problem(Variables(Var("X", MInteger(Exactly(10))),
                            Var("Y", MInteger(Exactly(20)))));
  const auto& problem_vars = problem.UnsafeGetVariables();

  EXPECT_TRUE(problem_vars.Contains("X"));
  EXPECT_TRUE(problem_vars.Contains("Y"));
  EXPECT_FALSE(problem_vars.Contains("Z"));
}

TEST(ProblemTest, AddingAdditionalConstraintsShouldWork) {
  Problem problem(Variables(Var("X", MInteger(Between(10, 20))),
                            Var("Y", MInteger(Exactly(20)))));
  Problem problem2 =
      problem.WithAdditionalConstraints(Var("X", MInteger(Between(5, 15))));
  const auto& problem_vars = problem2.UnsafeGetVariables();

  EXPECT_THAT((problem_vars.GetVariable<MInteger>("X")),
              GeneratedValuesAre(AllOf(Ge(10), Le(15))));

  EXPECT_TRUE(problem_vars.Contains("X"));
  EXPECT_TRUE(problem_vars.Contains("Y"));
}

TEST(ProblemTest, AddingAdditionalConstraintsWithWrongTypeShouldFail) {
  Problem problem(Variables(Var("X", MInteger(Between(10, 20))),
                            Var("Y", MInteger(Exactly(20)))));
  EXPECT_THAT([&] { problem.WithAdditionalConstraints(Var("X", MString())); },
              ThrowsMessage<ConfigurationError>(
                  HasSubstr("Type mismatch for variable 'X'")));
}

TEST(ProblemTest, AddingAdditionalConstraintsWithANewVariableShouldFail) {
  Problem problem(Variables(Var("X", MInteger(Between(10, 20))),
                            Var("Y", MInteger(Exactly(20)))));
  EXPECT_THAT(
      [&] {
        problem.WithAdditionalConstraints(Var("Z", MInteger(Exactly(30))));
      },
      ThrowsMessage<ConfigurationError>(
          HasSubstr("Cannot add new variables in WithAdditionalConstraints")));
}

TEST(ProblemTest, InputReadVariableNamesPropagateFromLines) {
  InputFormat format(Line("N"), Multiline(NumLines("N"), "A"));
  Problem problem(format);

  EXPECT_THAT(format.GetReadVariableNames(), ElementsAre("N", "A"));
  EXPECT_THAT(problem.GetInputReadVariableNames(),
              Optional(ElementsAre("N", "A")));
}

TEST(ProblemTest, OutputReadVariableNamesPropagateFromSimpleIO) {
  OutputFormat format(SimpleIO().AddMultilineSection("N", "Answers"));
  Problem problem(format);

  EXPECT_THAT(format.GetReadVariableNames(), ElementsAre("Answers"));
  EXPECT_THAT(problem.GetOutputReadVariableNames(),
              Optional(ElementsAre("Answers")));
}

TEST(ProblemTest, InputFormatShouldWork) {
  EXPECT_EQ(Problem().GetInputReader(), std::nullopt);
  EXPECT_EQ(Problem().GetInputWriter(), std::nullopt);
  EXPECT_EQ(Problem().GetInputDependencies(), std::nullopt);

  Problem problem(InputFormat(Line("A")));
  EXPECT_THAT(problem.GetInputReader(), Optional(_));
  EXPECT_THAT(problem.GetInputWriter(), Optional(_));
  EXPECT_THAT(problem.GetInputDependencies(), Optional(ElementsAre("A")));
}

TEST(ProblemTest, OutputFormatShouldWork) {
  EXPECT_EQ(Problem().GetOutputReader(), std::nullopt);
  EXPECT_EQ(Problem().GetOutputWriter(), std::nullopt);
  EXPECT_EQ(Problem().GetOutputDependencies(), std::nullopt);

  Problem problem(OutputFormat(Line("A")));
  EXPECT_THAT(problem.GetOutputReader(), Optional(_));
  EXPECT_THAT(problem.GetOutputWriter(), Optional(_));
  EXPECT_THAT(problem.GetOutputDependencies(), Optional(ElementsAre("A")));
}

}  // namespace
}  // namespace moriarty
