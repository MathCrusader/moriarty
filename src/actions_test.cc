// Copyright 2025 Darcy Best
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

#include "src/actions.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/constraints/numeric_constraints.h"
#include "src/librarian/errors.h"
#include "src/librarian/policies.h"
#include "src/librarian/testing/gtest_helpers.h"
#include "src/problem.h"
#include "src/simple_io.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace {

using ::moriarty_testing::SingleCall;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::SizeIs;
using ::testing::ThrowsMessage;

// -----------------------------------------------------------------------------

TEST(ValidateInputTest, WorksForValidInput) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))),
            InputFormat(SimpleIO().AddLine("N")));

  std::istringstream input("5\n");
  EXPECT_NO_THROW(ValidateInput(p).ReadInputUsing({.istream = input}).Run());
}

TEST(ValidateInputTest, InvalidVariableShouldFail) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))),
            InputFormat(SimpleIO().AddLine("N")));

  std::istringstream input("123\n");
  EXPECT_THAT(SingleCall([&] {
                ValidateInput(p).ReadInputUsing({.istream = input}).Run();
              }),
              ThrowsMessage<IOError>(HasSubstr("between 1 and 10")));
}

TEST(ValidateInputTest, InvalidWhitespaceShouldFail) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))),
            InputFormat(SimpleIO().AddLine("N")));

  {
    std::istringstream input(" 5\n");
    EXPECT_THAT(
        SingleCall(
            [&] { ValidateInput(p).ReadInputUsing({.istream = input}).Run(); }),
        ThrowsMessage<IOError>(HasSubstr("Expected a token, but got ' '")));
  }
  {
    std::istringstream input("5");
    EXPECT_THAT(
        SingleCall(
            [&] { ValidateInput(p).ReadInputUsing({.istream = input}).Run(); }),
        ThrowsMessage<IOError>(HasSubstr("Expected '\\n', but got EOF")));
  }
  {
    std::istringstream input("");
    EXPECT_THAT(
        SingleCall(
            [&] { ValidateInput(p).ReadInputUsing({.istream = input}).Run(); }),
        ThrowsMessage<IOError>(HasSubstr("Expected a token, but got EOF")));
  }
}

TEST(ValidateInputTest, ValidMultipleTestCasesShouldSucceed) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10))),
                      Var("X", MInteger(Between(20, 25)))),
            InputFormat(
                SimpleIO().WithNumberOfTestCasesInHeader().AddLine("N", "X")));

  std::istringstream input(R"(3
7 21
1 22
5 25
)");
  EXPECT_NO_THROW(ValidateInput(p).ReadInputUsing({.istream = input}).Run());
}

TEST(ValidateInputTest, InvalidMultipleTestCasesShouldFail) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10))),
                      Var("X", MInteger(Between(20, 25)))),
            InputFormat(
                SimpleIO().WithNumberOfTestCasesInHeader().AddLine("N", "X")));
  {
    std::istringstream input(R"(4
7 21
1 22
5 25
)");
    EXPECT_THAT(
        SingleCall(
            [&] { ValidateInput(p).ReadInputUsing({.istream = input}).Run(); }),
        ThrowsMessage<IOError>(HasSubstr("Expected a token, but got EOF")));
  }
  {
    std::istringstream input(R"(2
7 21
1 22
5 25
)");
    EXPECT_THAT(
        SingleCall(
            [&] { ValidateInput(p).ReadInputUsing({.istream = input}).Run(); }),
        ThrowsMessage<IOError>(HasSubstr("Expected EOF, but got more input")));
  }
}

TEST(ValidateInputTest, WhitespaceStrictnessShouldBeRespected) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10))),
                      Var("X", MInteger(Between(20, 25)))),
            InputFormat(SimpleIO().AddLine("N", "X")));
  std::istringstream input(" 7 \t  \n \n21");
  EXPECT_NO_THROW(ValidateInput(p)
                      .ReadInputUsing({.istream = input,
                                       .whitespace_strictness =
                                           WhitespaceStrictness::kFlexible})
                      .Run());
}

// -----------------------------------------------------------------------------

TEST(GenerateTest, MissingSeedShouldThrow) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))));

  EXPECT_THAT(SingleCall([&] {
                Generate(p)
                    .Using("TestGenerator",
                           [](GenerateContext ctx) { return TestCase(); })
                    .Run();
              }),
              ThrowsMessage<ConfigurationError>(
                  HasSubstr("seed is not set when generating")));
}

TEST(GenerateTest, NumCasesShouldBeRespected) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))), Seed("test_seed"));

  int generator_calls = 0;
  int expected_calls = 5;
  auto test_cases = Generate(p)
                        .Using("TestGenerator",
                               [&generator_calls](GenerateContext ctx) {
                                 generator_calls++;
                                 return TestCase();
                               },
                               {.num_calls = expected_calls})
                        .Run();

  EXPECT_EQ(generator_calls, expected_calls);
  EXPECT_THAT(test_cases, SizeIs(expected_calls));
}

TEST(GenerateTest, ConstraintsShouldBeRespected) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))), Seed("test_seed"));

  auto test_cases = Generate(p)
                        .Using("TestGenerator",
                               [](GenerateContext ctx) {
                                 return MTestCase().ConstrainVariable(
                                     "N", MInteger(Between(8, 20)));
                               },
                               {.num_calls = 50})
                        .Run();

  EXPECT_THAT(test_cases, SizeIs(50));
  std::set<int> observed_values;
  for (const auto& tc : test_cases)
    observed_values.insert(tc.GetValue<MInteger>("N"));
  EXPECT_THAT(observed_values, ElementsAre(8, 9, 10));
}

TEST(GenerateTest, GenerateShouldOnlyAutoGenerateTheInputVariables) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10))),
                      Var("M", MInteger(Between(1, 10)))),
            Seed("test_seed"), InputFormat(SimpleIO().AddLine("N")));

  auto test_cases = Generate(p)
                        .Using("TestGenerator",
                               [](GenerateContext ctx) { return MTestCase(); })
                        .Run();

  EXPECT_THAT(test_cases, SizeIs(1));
  const auto& test_case = test_cases[0];
  EXPECT_TRUE(test_case.UnsafeGetValues().Contains("N"));
  EXPECT_FALSE(test_case.UnsafeGetValues().Contains("M"));
}

TEST(GenerateTest, GenerateShouldAutoGenerateDependenciesOfInputVariables) {
  Problem p(Variables(Var("N", MInteger(Between(1, "M"))),
                      Var("M", MInteger(Between(1, 10))),
                      Var("X", MInteger(Between(1, 10))),
                      Var("Y", MInteger(Between(1, "X")))),
            Seed("test_seed"),
            InputFormat(SimpleIO().AddMultilineSection("Y", "N")));

  auto test_cases = Generate(p)
                        .Using("TestGenerator",
                               [](GenerateContext ctx) { return MTestCase(); })
                        .Run();

  EXPECT_THAT(test_cases, SizeIs(1));
  const auto& test_case = test_cases[0];
  EXPECT_TRUE(test_case.UnsafeGetValues().Contains("N"));
  EXPECT_TRUE(test_case.UnsafeGetValues().Contains("M"));  // N depends on me
  EXPECT_TRUE(test_case.UnsafeGetValues().Contains("Y"));
  EXPECT_TRUE(test_case.UnsafeGetValues().Contains("X"));  // Y depends on me
}

TEST(GenerateTest, WritingInputShouldWork) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))), Seed("test_seed"),
            InputFormat(SimpleIO().AddLine("N")));

  std::ostringstream output;
  auto test_cases = Generate(p)
                        .Using("TestGenerator",
                               [](GenerateContext ctx) { return MTestCase(); },
                               {.num_calls = 3})
                        .WriteInputUsing({.ostream = output})
                        .Run();

  EXPECT_THAT(test_cases, SizeIs(3));

  std::istringstream input_stream(output.str());
  for (int i = 0; i < 3; i++) {
    int n;
    input_stream >> n;
    EXPECT_GE(n, 1);
    EXPECT_LE(n, 10);
  }
}

TEST(GenerateTest, WritingOutputShouldWork) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))), Seed("test_seed"),
            OutputFormat(SimpleIO().AddLine("N")));

  std::ostringstream output;
  auto test_cases = Generate(p)
                        .Using("TestGenerator",
                               [](GenerateContext ctx) { return MTestCase(); },
                               {.num_calls = 3})
                        .WriteOutputUsing({.ostream = output})
                        .Run();

  EXPECT_THAT(test_cases, SizeIs(3));

  std::istringstream output_stream(output.str());
  for (int i = 0; i < 3; i++) {
    int n;
    output_stream >> n;
    EXPECT_GE(n, 1);
    EXPECT_LE(n, 10);
  }
}

TEST(GenerateTest, WritingWithoutCorrespondingFormatShouldThrow) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))), Seed("test_seed"));

  EXPECT_THAT(SingleCall([&] {
                Generate(p)
                    .Using("TestGenerator",
                           [](GenerateContext ctx) { return MTestCase(); })
                    .WriteInputUsing({.ostream = std::cout})
                    .Run();
              }),
              ThrowsMessage<ConfigurationError>(
                  HasSubstr("No InputFormat specified in Problem")));
  EXPECT_THAT(SingleCall([&] {
                Generate(p)
                    .Using("TestGenerator",
                           [](GenerateContext ctx) { return MTestCase(); })
                    .WriteOutputUsing({.ostream = std::cout})
                    .Run();
              }),
              ThrowsMessage<ConfigurationError>(
                  HasSubstr("No OutputFormat specified in Problem")));
}

TEST(GenerateTest, CallingRunWithNoGeneratorsShouldThrow) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))), Seed("test_seed"));

  EXPECT_THAT(
      SingleCall([&] { Generate(p).Run(); }),
      ThrowsMessage<ConfigurationError>(HasSubstr("No generators specified")));
}

}  // namespace
}  // namespace moriarty
