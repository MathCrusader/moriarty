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

#include "src/problem.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/simple_io.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace {

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Optional;
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

TEST(ProblemTest, InputFormatShouldWork) {
  EXPECT_EQ(Problem().GetInputReader(), std::nullopt);
  EXPECT_EQ(Problem().GetInputWriter(), std::nullopt);
  EXPECT_EQ(Problem().GetInputDependencies(), std::nullopt);

  Problem problem(InputFormat(SimpleIO().AddLine("A")));
  EXPECT_THAT(problem.GetInputReader(), Optional(_));
  EXPECT_THAT(problem.GetInputWriter(), Optional(_));
  EXPECT_THAT(problem.GetInputDependencies(), Optional(ElementsAre("A")));
}

TEST(ProblemTest, OutputFormatShouldWork) {
  EXPECT_EQ(Problem().GetOutputReader(), std::nullopt);
  EXPECT_EQ(Problem().GetOutputWriter(), std::nullopt);
  EXPECT_EQ(Problem().GetOutputDependencies(), std::nullopt);

  Problem problem(OutputFormat(SimpleIO().AddLine("A")));
  EXPECT_THAT(problem.GetOutputReader(), Optional(_));
  EXPECT_THAT(problem.GetOutputWriter(), Optional(_));
  EXPECT_THAT(problem.GetOutputDependencies(), Optional(ElementsAre("A")));
}

}  // namespace
}  // namespace moriarty
