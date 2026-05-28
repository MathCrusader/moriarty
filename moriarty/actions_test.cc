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

#include "moriarty/actions.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <span>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "moriarty/constraints/container_constraints.h"
#include "moriarty/constraints/numeric_constraints.h"
#include "moriarty/context.h"
#include "moriarty/librarian/errors.h"
#include "moriarty/librarian/policies.h"
#include "moriarty/librarian/testing/gtest_helpers.h"
#include "moriarty/problem.h"
#include "moriarty/simple_io.h"
#include "moriarty/variables/marray.h"
#include "moriarty/variables/minteger.h"

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
            InputFormat(Line("N")));

  std::istringstream input("5\n");
  EXPECT_NO_THROW(ValidateInput(p).ReadInputUsing({.istream = input}).Run());
}

TEST(ValidateInputTest, ValidateInputWithMultilineFormatShouldWork) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10))),
                      Var("A", MArray<MInteger>(Elements(Between(22, 23)),
                                                Length("N")))),
            InputFormat(Line("N"), Multiline(NumLines("N"), "A")));

  {
    std::istringstream input("2\n22\n23\n");
    EXPECT_NO_THROW(ValidateInput(p).ReadInputUsing({.istream = input}).Run());
  }
  {
    std::istringstream input("2\n22 23\n");
    EXPECT_THAT(
        SingleCall(
            [&] { ValidateInput(p).ReadInputUsing({.istream = input}).Run(); }),
        ThrowsMessage<IOError>(HasSubstr("Expected '\\n', but got ' '")));
  }
}

TEST(ValidateInputTest, InvalidVariableShouldFail) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))),
            InputFormat(Line("N")));

  std::istringstream input("123\n");
  EXPECT_THAT(SingleCall([&] {
                ValidateInput(p).ReadInputUsing({.istream = input}).Run();
              }),
              ThrowsMessage<IOError>(HasSubstr("too large")));
}

TEST(ValidateInputTest, InvalidWhitespaceShouldFail) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))),
            InputFormat(Line("N")));

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
            InputFormat(Line("N", "X")));
  std::istringstream input(" 7 \t  \n \n21");
  EXPECT_NO_THROW(ValidateInput(p)
                      .ReadInputUsing({.istream = input,
                                       .whitespace_strictness =
                                           WhitespaceStrictness::kFlexible})
                      .Run());
}

// -----------------------------------------------------------------------------

TEST(ValidateOutputTest, WorksForValidOutput) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10))),
                      Var("X", MInteger(Between(20, 25)))),
            InputFormat(Line("N")), OutputFormat(Line("X")));

  std::istringstream input("5\n");
  std::istringstream output("21\n");
  EXPECT_NO_THROW(ValidateOutput(p)
                      .ReadInputUsing({.istream = input})
                      .ReadOutputUsing({.istream = output})
                      .Run());
}

TEST(ValidateOutputTest, InvalidInputShouldFail) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10))),
                      Var("X", MInteger(Between(20, 25)))),
            InputFormat(Line("N")), OutputFormat(Line("X")));

  std::istringstream input("50\n");
  std::istringstream output("21\n");
  EXPECT_THAT(SingleCall([&]() {
                ValidateOutput(p)
                    .ReadInputUsing({.istream = input})
                    .ReadOutputUsing({.istream = output})
                    .Run();
              }),
              ThrowsMessage<IOError>(HasSubstr("too large")));
}

TEST(ValidateOutputTest, InvalidOutputShouldFail) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10))),
                      Var("X", MInteger(Between(20, 25)))),
            InputFormat(Line("N")), OutputFormat(Line("X")));

  std::istringstream input("5\n");
  std::istringstream output("30\n");
  EXPECT_THAT(SingleCall([&]() {
                ValidateOutput(p)
                    .ReadInputUsing({.istream = input})
                    .ReadOutputUsing({.istream = output})
                    .Run();
              }),
              ThrowsMessage<IOError>(HasSubstr("too large")));
}

TEST(ValidateOutputTest, InvalidWhitespaceInOutputShouldRespectStrictness) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10))),
                      Var("X", MInteger(Between(20, 25)))),
            InputFormat(Line("N")), OutputFormat(Line("X")));

  {  // Extra space in output
    std::istringstream input("5\n");
    std::istringstream output("21 \n");
    EXPECT_THAT(
        SingleCall([&]() {
          ValidateOutput(p)
              .ReadInputUsing({.istream = input})
              .ReadOutputUsing({.istream = output})
              .Run();
        }),
        ThrowsMessage<IOError>(HasSubstr("Expected '\\n', but got ' '")));
  }
  {  // Missing newline in output
    std::istringstream input("5\n");
    std::istringstream output("21");
    EXPECT_THAT(
        SingleCall([&]() {
          ValidateOutput(p)
              .ReadInputUsing({.istream = input})
              .ReadOutputUsing({.istream = output})
              .Run();
        }),
        ThrowsMessage<IOError>(HasSubstr("Expected '\\n', but got EOF")));
  }
  {  // Missing newline in input
    std::istringstream input("5");
    std::istringstream output("21\n");
    EXPECT_THAT(
        SingleCall([&]() {
          ValidateOutput(p)
              .ReadInputUsing({.istream = input})
              .ReadOutputUsing({.istream = output})
              .Run();
        }),
        ThrowsMessage<IOError>(HasSubstr("Expected '\\n', but got EOF")));
  }
  {  // Extra space in output, but with flexible whitespace strictness
    std::istringstream input("5\n");
    std::istringstream output("21 \n");
    EXPECT_NO_THROW(ValidateOutput(p)
                        .ReadInputUsing({.istream = input})
                        .ReadOutputUsing({.istream = output,
                                          .whitespace_strictness =
                                              WhitespaceStrictness::kFlexible})
                        .Run());
  }
  {  // Missing newline in output, but with flexible whitespace strictness
    std::istringstream input("5\n");
    std::istringstream output("21");
    EXPECT_NO_THROW(ValidateOutput(p)
                        .ReadInputUsing({.istream = input})
                        .ReadOutputUsing({.istream = output,
                                          .whitespace_strictness =
                                              WhitespaceStrictness::kFlexible})
                        .Run());
  }
  {  // Missing newline in input, but with flexible whitespace strictness
    std::istringstream input("5");
    std::istringstream output("21\n");
    EXPECT_NO_THROW(ValidateOutput(p)
                        .ReadInputUsing({.istream = input,
                                         .whitespace_strictness =
                                             WhitespaceStrictness::kFlexible})
                        .ReadOutputUsing({.istream = output})
                        .Run());
  }
}

TEST(ValidateOutputTest, ExtraContentAtEOFShouldFail) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10))),
                      Var("X", MInteger(Between(20, 25)))),
            InputFormat(Line("N")), OutputFormat(Line("X")));

  {
    std::istringstream input("5\n");
    std::istringstream output("21\nextra\n");
    EXPECT_THAT(
        SingleCall([&]() {
          ValidateOutput(p)
              .ReadInputUsing({.istream = input})
              .ReadOutputUsing(
                  {.istream = output,
                   .whitespace_strictness = WhitespaceStrictness::kFlexible})
              .Run();
        }),
        ThrowsMessage<IOError>(HasSubstr("Expected EOF, but got more input")));
  }
  {
    std::istringstream input("5\nextra\n");
    std::istringstream output("21\n");
    EXPECT_THAT(
        SingleCall([&]() {
          ValidateOutput(p)
              .ReadInputUsing(
                  {.istream = input,
                   .whitespace_strictness = WhitespaceStrictness::kFlexible})
              .ReadOutputUsing(
                  {.istream = output,
                   .whitespace_strictness = WhitespaceStrictness::kFlexible})
              .Run();
        }),
        ThrowsMessage<IOError>(HasSubstr("Expected EOF, but got more input")));
  }
}

TEST(ValidateOutputTest, OutputShouldHaveAccessToInputVariables) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10))),
                      Var("X", MInteger(Between(1, "N")))),
            InputFormat(Line("N")), OutputFormat(Line("X")));

  {
    std::istringstream input("5\n");
    std::istringstream output("3\n");
    EXPECT_NO_THROW(ValidateOutput(p)
                        .ReadInputUsing({.istream = input})
                        .ReadOutputUsing({.istream = output,
                                          .whitespace_strictness =
                                              WhitespaceStrictness::kFlexible})
                        .Run());
  }
  {
    std::istringstream input("5\n");
    std::istringstream output("6\n");
    EXPECT_THAT(SingleCall([&]() {
                  ValidateOutput(p)
                      .ReadInputUsing({.istream = input})
                      .ReadOutputUsing({.istream = output,
                                        .whitespace_strictness =
                                            WhitespaceStrictness::kFlexible})
                      .Run();
                }),
                ThrowsMessage<IOError>(HasSubstr("too large")));
  }
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
            Seed("test_seed"), InputFormat(Line("N")));

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
            Seed("test_seed"), InputFormat(Multiline(NumLines("Y"), "N")));

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

TEST(GenerateTest, GenerateShouldBeOkayWithNonImportantVariablesDependingOnIt) {
  {  // World 1: x is not asked to be generated
    Problem p(Title(""), Seed("temporary_fixme_seed"),
              Variables(Var("N", MInteger(Between("1", "10"))),
                        Var("x", MInteger(Mod("0", "N")))),
              InputFormat(Line("N")), OutputFormat());

    auto test_cases =
        Generate(p)
            .Using("TestGenerator",
                   [](GenerateContext ctx) { return MTestCase(); })
            .Run();

    EXPECT_THAT(test_cases, SizeIs(1));
    const auto& test_case = test_cases[0];
    EXPECT_TRUE(test_case.UnsafeGetValues().Contains("N"));
    EXPECT_FALSE(test_case.UnsafeGetValues().Contains("x"));
  }
  {  // World 2: x is asked to be generated, but it depends on N
    Problem p(Title(""), Seed("temporary_fixme_seed"),
              Variables(Var("N", MInteger(Between("1", "10"))),
                        Var("x", MInteger(Mod("0", "N")))),
              InputFormat(Line("N", "x")), OutputFormat());

    auto test_cases =
        Generate(p)
            .Using("TestGenerator",
                   [](GenerateContext ctx) { return MTestCase(); })
            .Run();

    EXPECT_THAT(test_cases, SizeIs(1));
    const auto& test_case = test_cases[0];
    EXPECT_TRUE(test_case.UnsafeGetValues().Contains("N"));
    EXPECT_TRUE(test_case.UnsafeGetValues().Contains("x"));
  }
}

TEST(GenerateTest, WritingInputShouldWork) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))), Seed("test_seed"),
            InputFormat(Line("N")));

  std::ostringstream output;
  auto test_cases = Generate(p)
                        .Using("TestGenerator",
                               [](GenerateContext ctx) { return MTestCase(); },
                               {.num_calls = 3})
                        .WriteTo(WriteStreams{.input = output})
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
            InputFormat(), OutputFormat(Line("N")));

  std::ostringstream input;
  std::ostringstream output;
  auto test_cases = Generate(p)
                        .Using("TestGenerator",
                               [](GenerateContext ctx) { return MTestCase(); },
                               {.num_calls = 3})
                        .WriteTo(WriteStreams{.input = input, .output = output})
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

TEST(GenerateTest, WritingToFilenamePatternShouldCreateFiles) {
  const char* test_tmpdir = std::getenv("TEST_TMPDIR");
  ASSERT_NE(test_tmpdir, nullptr);

  namespace fs = std::filesystem;
  fs::path base_path =
      fs::path(test_tmpdir) / "WritingToFilenamePatternShouldCreateFiles";
  fs::path output_pattern = base_path / "{gen}_{call}_{idx}";

  Problem p(Variables(Var("N", MInteger(Between(1, 10)))), Seed("test_seed"),
            InputFormat(Line("N")), OutputFormat(Line("N")));

  auto test_cases = Generate(p)
                        .Using("TestGenerator",
                               [](GenerateContext ctx) { return MTestCase(); },
                               {.num_calls = 2})
                        .Using("AnotherGenerator",
                               [](GenerateContext ctx) {
                                 return MTestCase().ConstrainVariable(
                                     "N", MInteger(Exactly(4)));
                               },
                               {.num_calls = 2})
                        .WriteTo(output_pattern.string())
                        .Run();

  EXPECT_THAT(test_cases, SizeIs(4));

  for (int call = 0; call < 2; call++) {
    fs::path input_path1 =
        base_path / std::format("TestGenerator_{}_1.in", call);
    fs::path output_path1 =
        base_path / std::format("TestGenerator_{}_1.ans", call);
    fs::path input_path2 =
        base_path / std::format("AnotherGenerator_{}_1.in", call);
    fs::path output_path2 =
        base_path / std::format("AnotherGenerator_{}_1.ans", call);

    EXPECT_TRUE(fs::exists(input_path1));
    EXPECT_TRUE(fs::exists(output_path1));
    EXPECT_TRUE(fs::exists(input_path2));
    EXPECT_TRUE(fs::exists(output_path2));

    std::ifstream input_file1(input_path1);
    std::ifstream output_file1(output_path1);
    std::ifstream input_file2(input_path2);
    std::ifstream output_file2(output_path2);
    ASSERT_TRUE(input_file1.is_open());
    ASSERT_TRUE(output_file1.is_open());
    ASSERT_TRUE(input_file2.is_open());
    ASSERT_TRUE(output_file2.is_open());

    int input_value;
    int output_value;
    ASSERT_TRUE(input_file1 >> input_value);
    ASSERT_TRUE(output_file1 >> output_value);
    EXPECT_GE(input_value, 1);
    EXPECT_LE(input_value, 10);
    EXPECT_EQ(output_value, input_value);

    ASSERT_TRUE(input_file2 >> input_value);
    ASSERT_TRUE(output_file2 >> output_value);
    EXPECT_EQ(input_value, 4);
    EXPECT_EQ(output_value, input_value);
  }
}

TEST(GenerateTest, WritingWithoutOutputFormatShouldOnlyCreateInputFiles) {
  const char* test_tmpdir = std::getenv("TEST_TMPDIR");
  ASSERT_NE(test_tmpdir, nullptr);

  namespace fs = std::filesystem;
  fs::path base_path = fs::path(test_tmpdir) /
                       "WritingWithoutOutputFormatShouldOnlyCreateInputFiles";
  fs::path output_pattern = base_path / "{gen}_{call}_{idx}";

  Problem p(Variables(Var("N", MInteger(Between(1, 10)))), Seed("test_seed"),
            InputFormat(Line("N")));

  auto test_cases = Generate(p)
                        .Using("TestGenerator",
                               [](GenerateContext ctx) { return MTestCase(); },
                               {.num_calls = 2})
                        .WriteTo(output_pattern.string())
                        .Run();

  EXPECT_THAT(test_cases, SizeIs(2));

  for (int call = 0; call < 2; call++) {
    fs::path input_path =
        base_path / std::format("TestGenerator_{}_1.in", call);
    fs::path output_path =
        base_path / std::format("TestGenerator_{}_1.ans", call);

    EXPECT_TRUE(fs::exists(input_path));
    EXPECT_FALSE(fs::exists(output_path));
  }
}

TEST(GenerateTest, WritingWithTrivialOutputFormatShouldOnlyCreateInputFiles) {
  const char* test_tmpdir = std::getenv("TEST_TMPDIR");
  ASSERT_NE(test_tmpdir, nullptr);

  namespace fs = std::filesystem;
  fs::path base_path =
      fs::path(test_tmpdir) /
      "WritingWithTrivialOutputFormatShouldOnlyCreateInputFiles";
  fs::path output_pattern = base_path / "{gen}_{call}_{idx}";

  Problem p(Variables(Var("N", MInteger(Between(1, 10)))), Seed("test_seed"),
            InputFormat(Line("N")), OutputFormat());

  auto test_cases = Generate(p)
                        .Using("TestGenerator",
                               [](GenerateContext ctx) { return MTestCase(); },
                               {.num_calls = 2})
                        .WriteTo(output_pattern.string())
                        .Run();

  EXPECT_THAT(test_cases, SizeIs(2));

  for (int call = 0; call < 2; call++) {
    fs::path input_path =
        base_path / std::format("TestGenerator_{}_1.in", call);
    fs::path output_path =
        base_path / std::format("TestGenerator_{}_1.ans", call);

    EXPECT_TRUE(fs::exists(input_path));
    EXPECT_FALSE(fs::exists(output_path));
  }
}

TEST(GenerateTest, WritingWithoutAllOutputVariablesShouldOnlyCreateInputFiles) {
  const char* test_tmpdir = std::getenv("TEST_TMPDIR");
  ASSERT_NE(test_tmpdir, nullptr);

  namespace fs = std::filesystem;
  fs::path base_path =
      fs::path(test_tmpdir) /
      "WritingWithoutAllOutputVariablesShouldOnlyCreateInputFiles";
  fs::path output_pattern = base_path / "{gen}_{call}_{idx}";

  Problem p(Variables(Var("N", MInteger(Between(1, 10))), Var("X", MInteger())),
            Seed("test_seed"), InputFormat(Line("N")), OutputFormat(Line("X")));

  auto test_cases = Generate(p)
                        .Using("TestGenerator",
                               [](GenerateContext ctx) { return MTestCase(); },
                               {.num_calls = 2})
                        .WriteTo(output_pattern.string())
                        .Run();

  EXPECT_THAT(test_cases, SizeIs(2));

  for (int call = 0; call < 2; call++) {
    fs::path input_path =
        base_path / std::format("TestGenerator_{}_1.in", call);
    fs::path output_path =
        base_path / std::format("TestGenerator_{}_1.ans", call);

    EXPECT_TRUE(fs::exists(input_path));
    EXPECT_FALSE(fs::exists(output_path));
  }
}

TEST(GenerateTest, WritingWithoutCorrespondingFormatShouldThrow) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))), Seed("test_seed"));

  EXPECT_THAT(SingleCall([&] {
                Generate(p)
                    .Using("TestGenerator",
                           [](GenerateContext ctx) { return MTestCase(); })
                    .WriteTo(WriteStreams{.input = std::cout})
                    .Run();
              }),
              ThrowsMessage<ConfigurationError>(
                  HasSubstr("No InputFormat specified in Problem")));

  // TODO: Consider if this is an error or if we're safe to ignore.
  // EXPECT_THAT(
  //     SingleCall([&] {
  //       Generate(p)
  //           .Using("TestGenerator",
  //                  [](GenerateContext ctx) { return MTestCase(); })
  //           .WriteTo(WriteStreams{.input = std::cout, .output = std::cerr})
  //           .Run();
  //     }),
  //     ThrowsMessage<ConfigurationError>(
  //         HasSubstr("No OutputFormat specified in Problem")));
}

TEST(GenerateTest, CallingRunWithNoGeneratorsShouldThrow) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))), Seed("test_seed"));

  EXPECT_THAT(
      SingleCall([&] { Generate(p).Run(); }),
      ThrowsMessage<ConfigurationError>(HasSubstr("No generators specified")));
}

TEST(GenerateTest, GeneratorsShouldHaveAccessToArgs) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10000)))), Seed("test_seed"),
            InputFormat(), OutputFormat(Line("N")));

  std::ostringstream output;
  auto test_cases =
      Generate(p)
          .Using("TestGenerator",
                 [](GenerateContext ctx) {
                   return MTestCase().SetValue<MInteger>("N",
                                                         ctx.Arg<int>("value"));
                 },
                 {.num_calls = 3, .args = {{"value", "42"}}})
          .WriteTo(WriteStreams{.input = output, .output = output})
          .Run();

  EXPECT_THAT(test_cases, SizeIs(3));

  std::istringstream output_stream(output.str());
  for (int i = 0; i < 3; i++) {
    int n;
    output_stream >> n;
    EXPECT_EQ(n, 42);
  }
}

TEST(AnalyzeTest, CallingRunWithNoAnalyzersShouldThrow) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))), Seed("test_seed"),
            InputFormat(Line("N")));

  std::stringstream ss("");
  EXPECT_THAT(
      SingleCall([&] { Analyze(p).ReadInputUsing({.istream = ss}).Run(); }),
      ThrowsMessage<ConfigurationError>(HasSubstr("No analyzers specified")));
}

TEST(AnalyzeTest, CallingRunWithNoInputStreamShouldThrow) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))),
            InputFormat(Line("N")));

  EXPECT_THAT(SingleCall([&] {
                Analyze(p)
                    .Using("TestAnalyzer",
                           [](AnalyzeContext ctx, const TestCase& tc) {})
                    .Run();
              }),
              ThrowsMessage<ConfigurationError>(
                  HasSubstr("std::istream needed for input")));
}

TEST(AnalyzeTest, CallingRunWithNoInputFormatShouldThrow) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))));

  std::istringstream input("5\n");
  EXPECT_THAT(SingleCall([&] {
                Analyze(p)
                    .Using("TestAnalyzer",
                           [](AnalyzeContext ctx, const TestCase& tc) {})
                    .ReadInputUsing({.istream = input})
                    .Run();
              }),
              ThrowsMessage<ConfigurationError>(
                  HasSubstr("No InputFormat specified in Problem")));
}

TEST(AnalyzeTest, SingleTestCaseAnalyzerShouldWork) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))),
            InputFormat(Line("N")));

  std::istringstream input("5\n");
  int analyzer_calls = 0;
  int observed_value = 0;

  EXPECT_NO_THROW(Analyze(p)
                      .Using("TestAnalyzer",
                             [&](AnalyzeContext ctx, const TestCase& tc) {
                               analyzer_calls++;
                               observed_value = tc.GetValue<MInteger>("N");
                             })
                      .ReadInputUsing({.istream = input})
                      .Run());

  EXPECT_EQ(analyzer_calls, 1);
  EXPECT_EQ(observed_value, 5);
}

TEST(AnalyzeTest, MultiTestCaseAnalyzerShouldWork) {
  Problem p(
      Variables(Var("N", MInteger(Between(1, 10)))),
      InputFormat(SimpleIO().WithNumberOfTestCasesInHeader().AddLine("N")));

  std::istringstream input("3\n5\n7\n9\n");
  int analyzer_calls = 0;
  std::vector<int> observed_values;

  EXPECT_NO_THROW(
      Analyze(p)
          .Using("TestAnalyzer",
                 [&](AnalyzeContext ctx, std::span<const TestCase> cases) {
                   analyzer_calls++;
                   for (const auto& tc : cases) {
                     observed_values.push_back(tc.GetValue<MInteger>("N"));
                   }
                 })
          .ReadInputUsing({.istream = input})
          .Run());

  EXPECT_EQ(analyzer_calls, 1);
  EXPECT_THAT(observed_values, ElementsAre(5, 7, 9));
}

TEST(AnalyzeTest, MultipleAnalyzersShouldAllRun) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))),
            InputFormat(Line("N")));

  std::istringstream input("5\n");
  int analyzer1_calls = 0;
  int analyzer2_calls = 0;

  EXPECT_NO_THROW(
      Analyze(p)
          .Using("Analyzer1", [&](AnalyzeContext ctx,
                                  const TestCase& tc) { analyzer1_calls++; })
          .Using("Analyzer2", [&](AnalyzeContext ctx,
                                  const TestCase& tc) { analyzer2_calls++; })
          .ReadInputUsing({.istream = input})
          .Run());

  EXPECT_EQ(analyzer1_calls, 1);
  EXPECT_EQ(analyzer2_calls, 1);
}

TEST(AnalyzeTest, ReadingOutputShouldWork) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10))),
                      Var("Result", MInteger(Between(1, 100)))),
            InputFormat(Line("N")), OutputFormat(Line("Result")));

  std::istringstream input("5\n");
  std::istringstream output("42\n");
  int observed_n = 0;
  int observed_result = 0;

  EXPECT_NO_THROW(Analyze(p)
                      .Using("TestAnalyzer",
                             [&](AnalyzeContext ctx, const TestCase& tc) {
                               observed_n = tc.GetValue<MInteger>("N");
                               observed_result =
                                   tc.GetValue<MInteger>("Result");
                             })
                      .ReadInputUsing({.istream = input})
                      .ReadOutputUsing({.istream = output})
                      .Run());

  EXPECT_EQ(observed_n, 5);
  EXPECT_EQ(observed_result, 42);
}

TEST(AnalyzeTest, ReadingOutputWithoutOutputFormatShouldThrow) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))),
            InputFormat(Line("N")));

  std::istringstream input("5\n");
  std::istringstream output("42\n");

  EXPECT_THAT(SingleCall([&] {
                Analyze(p)
                    .Using("TestAnalyzer",
                           [&](AnalyzeContext ctx, const TestCase& tc) {})
                    .ReadInputUsing({.istream = input})
                    .ReadOutputUsing({.istream = output})
                    .Run();
              }),
              ThrowsMessage<ConfigurationError>(
                  HasSubstr("No OutputFormat specified in Problem")));
}

TEST(AnalyzeTest, MismatchedInputOutputCountsShouldThrow) {
  Problem p(
      Variables(Var("N", MInteger(Between(1, 10))),
                Var("Result", MInteger(Between(1, 100)))),
      InputFormat(SimpleIO().WithNumberOfTestCasesInHeader().AddLine("N")),
      OutputFormat(
          SimpleIO().WithNumberOfTestCasesInHeader().AddLine("Result")));

  std::istringstream input("3\n5\n7\n9\n");
  std::istringstream output("2\n42\n43\n");

  EXPECT_THAT(SingleCall([&] {
                Analyze(p)
                    .Using("TestAnalyzer",
                           [&](AnalyzeContext ctx, const TestCase& tc) {})
                    .ReadInputUsing({.istream = input})
                    .ReadOutputUsing({.istream = output})
                    .Run();
              }),
              ThrowsMessage<ValidationError>(
                  HasSubstr("Number of output test cases (2) does not match")));
}

TEST(AnalyzeTest, EmptyInputShouldThrow) {
  Problem p(
      Variables(Var("N", MInteger(Between(1, 10)))),
      InputFormat(SimpleIO().WithNumberOfTestCasesInHeader().AddLine("N")));

  std::istringstream input("0\n");

  EXPECT_THAT(SingleCall([&] {
                Analyze(p)
                    .Using("TestAnalyzer",
                           [&](AnalyzeContext ctx, const TestCase& tc) {})
                    .ReadInputUsing({.istream = input})
                    .Run();
              }),
              ThrowsMessage<ConfigurationError>(
                  HasSubstr("No Test Cases read in input")));
}

}  // namespace
}  // namespace moriarty
