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

#include "src/moriarty.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/constraints/container_constraints.h"
#include "src/constraints/numeric_constraints.h"
#include "src/context.h"
#include "src/librarian/errors.h"
#include "src/librarian/io_config.h"
#include "src/librarian/testing/gtest_helpers.h"
#include "src/librarian/testing/mtest_type.h"
#include "src/test_case.h"
#include "src/variables/marray.h"
#include "src/variables/minteger.h"
#include "src/variables/mstring.h"

namespace moriarty {
namespace {

using ::moriarty::Moriarty;
using ::moriarty_testing::MTestType;
using ::moriarty_testing::ThrowsValueNotFound;
using ::moriarty_testing::ThrowsValueTypeMismatch;
using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::Each;
using ::testing::ElementsAre;
using ::testing::Ge;
using ::testing::HasSubstr;
using ::testing::Le;
using ::testing::Pair;
using ::testing::SizeIs;
using ::testing::ThrowsMessage;

// -----------------------------------------------------------------------------
//  Import / Export / Generate Helpers
// -----------------------------------------------------------------------------

// [ {X = 1}, {X = 2}, ..., {X = n} ]
std::vector<TestCase> ImportIota(ImportContext ctx, std::string varX, int n) {
  std::vector<TestCase> cases;
  for (int i = 1; i <= n; i++) {
    cases.push_back(TestCase().SetValue<MInteger>(varX, i));
  }
  return cases;
}

// [ {R=1, S=11}, {R=2, S=22}, {R=3, S=33}, ..., {R=n,S=11*n} ]
std::vector<TestCase> ImportTwoIota(ImportContext ctx, std::string varR,
                                    std::string varS, int n) {
  std::vector<TestCase> cases;
  for (int i = 1; i <= n; i++) {
    cases.push_back(TestCase().SetValue<MInteger>(varR, i).SetValue<MInteger>(
        varS, i * 11));
  }
  return cases;
}

// [ {A = read()}, {A = read()}, ..., {A = read()} ]
std::vector<TestCase> ReadSingleInteger(ImportContext ctx, std::string varA,
                                        Whitespace separator, int n) {
  std::vector<TestCase> cases;
  for (int i = 0; i < n; i++) {
    if (i) ctx.ReadWhitespace(separator);
    int64_t A = std::stoll(ctx.ReadToken());
    cases.push_back(TestCase().SetValue<MInteger>(varA, A));
  }
  ctx.ReadEof();
  return cases;
}

std::vector<int64_t> ExportSingleIntegerToVector(
    ExportContext ctx, std::string variable, std::span<const TestCase> cases) {
  std::vector<int64_t> values;
  for (const TestCase& c : cases) {
    values.push_back(c.GetValue<MInteger>(variable));
  }
  return values;
}

std::vector<std::pair<int64_t, int64_t>> ExportTwoIntegersToVector(
    ExportContext ctx, std::string varR, std::string varS,
    std::span<const TestCase> cases) {
  std::vector<std::pair<int64_t, int64_t>> values;
  for (const TestCase& c : cases) {
    values.emplace_back(c.GetValue<MInteger>(varR), c.GetValue<MInteger>(varS));
  }
  return values;
}

std::vector<std::pair<std::string, std::string>> ExportTwoStringsToVector(
    ExportContext ctx, std::string varR, std::string varS,
    std::span<const TestCase> cases) {
  std::vector<std::pair<std::string, std::string>> values;
  for (const TestCase& c : cases) {
    values.emplace_back(c.GetValue<MString>(varR), c.GetValue<MString>(varS));
  }
  return values;
}

void PrintSingleVariable(ExportContext ctx, std::string varX,
                         std::span<const TestCase> cases) {
  for (const TestCase& c : cases) {
    ctx.PrintVariableFrom(varX, c);
    ctx.PrintWhitespace(Whitespace::kNewline);
  }
}

GenerateFn GenerateTwoVariables(std::string A, int64_t a, std::string B,
                                int64_t b) {
  return [=](GenerateContext ctx) -> std::vector<MTestCase> {
    return {MTestCase().SetValue<MInteger>(A, a).SetValue<MInteger>(B, b)};
  };
};

// -----------------------------------------------------------------------------

TEST(MoriartyTest, SetNumCasesWorksForValidInput) {
  Moriarty M;

  // The following will crash if the seed is invalid
  M.SetNumCases(0);
  M.SetNumCases(10);

  // TODO(b/182810006): Add tests that check at least this many cases are
  // generated once Generators are added to Moriarty.
}

TEST(MoriartyTest, SetNumCasesWithNegativeInputShouldCrash) {
  Moriarty M;
  EXPECT_THAT([] { Moriarty().SetNumCases(-1); },
              ThrowsMessage<ConfigurationError>(
                  HasSubstr("num_cases must be non-negative")));
  EXPECT_THAT([] { Moriarty().SetNumCases(-100); },
              ThrowsMessage<ConfigurationError>(
                  HasSubstr("num_cases must be non-negative")));
}

// TODO(b/182810006): Update seed when seed/name requirements are enabled
TEST(MoriartyTest, SetSeedWorksForValidInput) {
  Moriarty M;

  // The following will crash if the seed is invalid
  M.SetSeed("abcde0123456789");
}

TEST(MoriartyTest, ShouldSeedWithInvalidInputShouldCrash) {
  Moriarty M;

  EXPECT_THAT([] { Moriarty().SetSeed(""); },
              ThrowsMessage<ConfigurationError>(HasSubstr("seed's length")));
  EXPECT_THAT([] { Moriarty().SetSeed("abcde"); },
              ThrowsMessage<ConfigurationError>(HasSubstr("seed's length")));
}

TEST(MoriartyTest, AddVariableWorks) {
  Moriarty M;

  M.AddVariable("A", MTestType());
  M.AddVariable("B", MTestType());
  M.AddVariable("C", MTestType()).AddVariable("D", MTestType());
}

TEST(MoriartyDeathTest, AddTwoVariablesWithTheSameNameShouldCrash) {
  EXPECT_THAT(
      [] {
        Moriarty().AddVariable("X", MTestType()).AddVariable("X", MTestType());
      },
      ThrowsMessage<std::invalid_argument>(HasSubstr("`X` multiple times")));
}

TEST(MoriartyTest,
     ExportAllTestCasesShouldExportProperlyWithASingleGeneratorMultipleTimes) {
  Moriarty M;
  M.SetSeed("abcde0123456789");
  M.GenerateTestCases(
      [](GenerateContext ctx) -> std::vector<MTestCase> {
        return {MTestCase().SetValue<MInteger>("N", 0)};
      },
      {.num_calls = 3});

  std::vector<int64_t> result;
  M.ExportTestCases(
      [&result](ExportContext ctx, std::span<const TestCase> cases) {
        result = ExportSingleIntegerToVector(ctx, "N", cases);
      });
  EXPECT_THAT(result, ElementsAre(0, 0, 0));
}

TEST(MoriartyTest,
     ExportAllTestCasesShouldExportCasesProperlyWithMultipleGenerators) {
  Moriarty M;
  M.SetSeed("abcde0123456789");
  M.GenerateTestCases(GenerateTwoVariables("R", 1, "S", 11));
  M.GenerateTestCases(GenerateTwoVariables("R", 2, "S", 22));
  M.GenerateTestCases(GenerateTwoVariables("R", 3, "S", 33));

  std::vector<std::pair<int64_t, int64_t>> result;
  M.ExportTestCases(
      [&result](ExportContext ctx, std::span<const TestCase> cases) {
        result = ExportTwoIntegersToVector(ctx, "R", "S", cases);
      });
  EXPECT_THAT(result, ElementsAre(Pair(1, 11), Pair(2, 22), Pair(3, 33)));
}

TEST(
    MoriartyTest,
    ExportAllTestCasesShouldExportProperlyWithMultipleGeneratorsMultipleTimes) {
  Moriarty M;
  M.SetSeed("abcde0123456789");
  M.GenerateTestCases(GenerateTwoVariables("R", 1, "S", 11), {.num_calls = 2});
  M.GenerateTestCases(GenerateTwoVariables("R", 2, "S", 22), {.num_calls = 3});

  std::vector<std::pair<int64_t, int64_t>> result;
  M.ExportTestCases(
      [&result](ExportContext ctx, std::span<const TestCase> cases) {
        result = ExportTwoIntegersToVector(ctx, "R", "S", cases);
      });
  EXPECT_THAT(result, ElementsAre(Pair(1, 11), Pair(1, 11), Pair(2, 22),
                                  Pair(2, 22), Pair(2, 22)));
}

TEST(MoriartyTest, ExportAskingForNonExistentVariableShouldThrow) {
  Moriarty M;
  M.SetSeed("abcde0123456789");
  M.GenerateTestCases(GenerateTwoVariables("R", 1, "S", 11));

  // Exporter requests N, but only R and S are set...
  EXPECT_THAT(
      [&] {
        M.ExportTestCases(
            [](ExportContext ctx, std::span<const TestCase> cases) {
              ExportSingleIntegerToVector(ctx, "N", cases);
            });
      },
      ThrowsValueNotFound("N"));
}

TEST(MoriartyTest, ExportAskingForVariableOfTheWrongTypeShouldCrash) {
  Moriarty M;
  M.SetSeed("abcde0123456789");
  M.GenerateTestCases(GenerateTwoVariables("R", 1, "S", 2));

  // Exporter requests for MString, but R and S are MIntegers.
  EXPECT_THAT(
      [&] {
        M.ExportTestCases(
            [](ExportContext ctx, std::span<const TestCase> cases) {
              ExportTwoStringsToVector(ctx, "R", "S", cases);
            });
      },
      AnyOf(ThrowsValueTypeMismatch("R", "MString"),
            ThrowsValueTypeMismatch("S", "MString")));
}

TEST(MoriartyTest, GeneralConstraintsSetValueAreConsideredInGenerators) {
  Moriarty M;
  M.SetSeed("abcde0123456789");
  M.AddVariable("N", MInteger(Exactly(5)));

  EXPECT_NO_THROW({
    M.GenerateTestCases(
        [](GenerateContext ctx) -> std::vector<MTestCase> {
          return {MTestCase().SetValue<MInteger>("N", 5)};
        },
        {});
  });

  EXPECT_THAT(
      [&] {
        M.GenerateTestCases(
            [](GenerateContext ctx) -> std::vector<MTestCase> {
              return {MTestCase().SetValue<MInteger>("N", 1)};
            },
            GenerateOptions{});
      },
      ThrowsMessage<GenerationError>(HasSubstr("N")));
}

TEST(MoriartyTest, GeneralConstraintsAreConsideredInGenerators) {
  Moriarty M;
  M.SetSeed("abcde0123456789");
  M.AddVariable("R", MInteger(Between(3, 50)));
  M.GenerateTestCases([](GenerateContext ctx) -> std::vector<MTestCase> {
    return {
        MTestCase().ConstrainVariable<MInteger>("R", MInteger(Between(1, 10)))};
  });

  std::vector<int64_t> result;
  M.ExportTestCases(
      [&result](ExportContext ctx, std::span<const TestCase> cases) {
        result = ExportSingleIntegerToVector(ctx, "R", cases);
      });

  // // General says 3 <= R <= 50. Generator says 1 <= R <= 10.
  EXPECT_THAT(result, AllOf(SizeIs(1), Each(AllOf(Ge(3), Le(10)))));
}

TEST(MoriartyTest, GenerateOnlyGeneratesTheSubsetOfVariablesRequested) {
  Moriarty M;
  M.SetSeed("abcde0123456789");
  M.AddVariable("N", MInteger(Exactly(5)))
      .AddVariable("M", MInteger(Between(1, 10)));

  EXPECT_NO_THROW({
    M.GenerateTestCases(
        [](GenerateContext ctx) -> std::vector<MTestCase> {
          return {MTestCase().SetValue<MInteger>("N", 5)};
        },
        {.variables_to_generate = {{"N"}},
         .validation = ValidationStyle::kOnlySetValues});
  });

  EXPECT_THAT(
      [&] {
        M.GenerateTestCases(
            [](GenerateContext ctx) -> std::vector<MTestCase> {
              return {MTestCase().SetValue<MInteger>("N", 1)};
            },
            GenerateOptions{});
      },
      ThrowsMessage<GenerationError>(HasSubstr("N")));
}

TEST(MoriartyTest, GenerateTestCasesCrashesOnValidationErrorIfRequested) {
  Moriarty M;
  M.SetSeed("abcde0123456789");
  M.AddVariable("N", MInteger(Exactly(5)))
      .AddVariable("M", MInteger(Between(1, 10)));

  EXPECT_NO_THROW({
    M.GenerateTestCases(
        [](GenerateContext ctx) -> std::vector<MTestCase> {
          return {MTestCase().SetValue<MInteger>("N", 5)};
        },
        {.variables_to_generate = {{"N"}},
         .validation = ValidationStyle::kOnlySetValues});
  });

  EXPECT_THAT(
      [&] {
        M.GenerateTestCases(
            [](GenerateContext ctx) -> std::vector<MTestCase> {
              return {MTestCase().SetValue<MInteger>("N", 5)};
            },
            {.validation = ValidationStyle::kAllVariables});
      },
      ThrowsMessage<ValidationError>(
          HasSubstr("No value assigned to variable `M`")));

  EXPECT_NO_THROW({
    M.GenerateTestCases(
        [](GenerateContext ctx) -> std::vector<MTestCase> {
          return {MTestCase().SetValue<MInteger>("N", 5)};
        },
        {.validation = ValidationStyle::kOnlySetVariables});
  });

  EXPECT_THAT(
      [&] {
        M.GenerateTestCases(
            [](GenerateContext ctx) -> std::vector<MTestCase> {
              return {MTestCase().SetValue<MInteger>("N", 5)};
            },
            {.validation = ValidationStyle::kEverything});
      },
      ThrowsMessage<ValidationError>(
          HasSubstr("No value assigned to variable `M`")));
}

TEST(MoriartyTest, ImportAndExportShouldWorkTypicalCase) {
  {  // Single variable
    Moriarty M;
    M.ImportTestCases(
        [](ImportContext ctx) { return ImportIota(ctx, "X", 5); });

    std::vector<int64_t> result;
    M.ExportTestCases(
        [&result](ExportContext ctx, std::span<const TestCase> cases) {
          result = ExportSingleIntegerToVector(ctx, "X", cases);
        });
    EXPECT_THAT(result, ElementsAre(1, 2, 3, 4, 5));
  }
  {  // Multiple variables
    Moriarty M;
    M.ImportTestCases(
        [](ImportContext ctx) { return ImportTwoIota(ctx, "R", "S", 3); });

    std::vector<std::pair<int64_t, int64_t>> result;
    M.ExportTestCases(
        [&result](ExportContext ctx, std::span<const TestCase> cases) {
          result = ExportTwoIntegersToVector(ctx, "R", "S", cases);
        });
    EXPECT_THAT(result, ElementsAre(Pair(1, 11), Pair(2, 22), Pair(3, 33)));
  }
}

TEST(MoriartyTest, ImportAndExportWithIOStreamsShouldWork) {
  std::stringstream ss_in("1 2 3 4 5 6");
  Moriarty M;
  M.AddVariable("A", MInteger());
  M.ImportTestCases(
      [](ImportContext ctx) {
        return ReadSingleInteger(ctx, "A", Whitespace::kSpace, 6);
      },
      ImportOptions{.is = ss_in});

  std::stringstream ss_out;
  M.ExportTestCases(
      [](ExportContext ctx, std::span<const TestCase> cases) {
        PrintSingleVariable(ctx, "A", cases);
      },
      {.os = ss_out});
  EXPECT_EQ(ss_out.str(), "1\n2\n3\n4\n5\n6\n");
}

TEST(MoriartyTest, ImportSingleArrayShouldWork) {
  std::string expected = "6\n1 2 3 4 5 6\n";
  std::stringstream ss_in(expected);
  Moriarty M;
  M.AddVariable("N", MInteger());
  M.AddVariable(
      "A", MArray<MInteger>(Elements<MInteger>(Between(1, 10)), Length("N")));
  M.ImportTestCases(
      [](ImportContext ctx) -> std::vector<TestCase> {
        TestCase C = {};
        ctx.ReadVariableTo("N", C);
        ctx.ReadWhitespace(Whitespace::kNewline);
        ctx.ReadVariableTo("A", C);
        ctx.ReadWhitespace(Whitespace::kNewline);
        return {C};
      },
      ImportOptions{.is = ss_in});

  std::stringstream ss_out;
  M.ExportTestCases(
      [](ExportContext ctx, std::span<const TestCase> cases) {
        ctx.PrintVariableFrom("N", cases[0]);
        ctx.PrintWhitespace(Whitespace::kNewline);
        ctx.PrintVariableFrom("A", cases[0]);
        ctx.PrintWhitespace(Whitespace::kNewline);
      },
      {.os = ss_out});
  EXPECT_EQ(ss_out.str(), expected);
}

TEST(MoriartyTest, ValidateAllTestCasesWorksWhenAllVariablesAreValid) {
  Moriarty M;
  M.AddVariable("X", MInteger(Between(1, 5)));
  M.ImportTestCases([](ImportContext ctx) { return ImportIota(ctx, "X", 5); });
  EXPECT_TRUE(M.ValidateTestCases().IsValid());
}

TEST(MoriartyTest, ValidateAllTestCasesFailsWhenSingleVariableInvalid) {
  Moriarty M;
  M.AddVariable("X", MInteger(Between(1, 3)));
  M.ImportTestCases([](ImportContext ctx) { return ImportIota(ctx, "X", 5); },
                    ImportOptions{.validation = ValidationStyle::kNone});
  ValidationResults results = M.ValidateTestCases();
  EXPECT_FALSE(results.IsValid());
  EXPECT_THAT(results.DescribeFailures(), HasSubstr("Case #4 invalid"));
}

TEST(MoriartyTest, ValidateAllTestCasesFailsWhenSomeVariableInvalid) {
  Moriarty M;
  M.AddVariable("R", MInteger(Between(1, 3)))
      .AddVariable("S", MInteger(Between(10, 30)));
  M.ImportTestCases(
      [](ImportContext ctx) { return ImportTwoIota(ctx, "R", "S", 4); },
      ImportOptions{.validation = ValidationStyle::kNone});
  ValidationResults results = M.ValidateTestCases();
  EXPECT_FALSE(results.IsValid());
  EXPECT_THAT(results.DescribeFailures(), HasSubstr("Case #3 invalid"));
}

TEST(MoriartyTest, ValidateAllTestCasesHandlesMissingVariableOrValue) {
  Moriarty M;
  M.AddVariable("R", MInteger(Between(0, 5)))
      .AddVariable("q", MInteger(Between(10, 30)));  // Importer uses S, not q
  M.ImportTestCases(
      [](ImportContext ctx) { return ImportTwoIota(ctx, "R", "S", 4); },
      {.validation = ValidationStyle::kNone});

  ValidationResults results =
      M.ValidateTestCases({.validation = ValidationStyle::kAllVariables});
  EXPECT_FALSE(results.IsValid());
  EXPECT_THAT(results.DescribeFailures(),
              HasSubstr("No value assigned to variable `q`"));

  results =
      M.ValidateTestCases({.validation = ValidationStyle::kOnlySetVariables});
  EXPECT_TRUE(results.IsValid()) << results.DescribeFailures();

  results =
      M.ValidateTestCases({.validation = ValidationStyle::kOnlySetValues});
  EXPECT_FALSE(results.IsValid());
  EXPECT_THAT(
      results.DescribeFailures(),
      HasSubstr("No variable found for `S`, but a value was set for it"));

  results = M.ValidateTestCases({.validation = ValidationStyle::kEverything});
  EXPECT_FALSE(results.IsValid());
  EXPECT_THAT(
      results.DescribeFailures(),
      AnyOf(HasSubstr("No variable found for S, but a value was set for it"),
            HasSubstr("No value assigned to variable `q`")));
}

TEST(MoriartyTest, ImportTestCasesCrashesOnValidationErrorIfRequested) {
  Moriarty M;
  M.AddVariable("R", MInteger(Between(0, 5)))
      .AddVariable("q", MInteger(Between(10, 30)));  // Importer uses S, not q

  EXPECT_THAT(
      [&] {
        M.ImportTestCases(
            [](ImportContext ctx) { return ImportTwoIota(ctx, "R", "S", 4); },
            {.validation = ValidationStyle::kAllVariables});
      },
      ThrowsMessage<ValidationError>(
          HasSubstr("No value assigned to variable `q`")));

  EXPECT_NO_THROW(M.ImportTestCases(
      [](ImportContext ctx) { return ImportTwoIota(ctx, "R", "S", 4); },
      {.validation = ValidationStyle::kOnlySetVariables}));

  EXPECT_THAT(
      [&] {
        M.ImportTestCases(
            [](ImportContext ctx) { return ImportTwoIota(ctx, "R", "S", 4); },
            {.validation = ValidationStyle::kOnlySetValues});
      },
      ThrowsMessage<ValidationError>(
          HasSubstr("No variable found for `S`, but a value was set for it")));
}

TEST(MoriartyTest, ImportTestCasesHandlesMissingVariableOrValue) {
  Moriarty M;
  M.AddVariable("R", MInteger(Between(0, 5)))
      .AddVariable("q", MInteger(Between(10, 30)));  // Importer uses S, not q

  EXPECT_THAT(
      [&] {
        M.ImportTestCases(
            [](ImportContext ctx) { return ImportTwoIota(ctx, "R", "S", 4); },
            {.validation = ValidationStyle::kAllVariables});
      },
      ThrowsMessage<ValidationError>(
          HasSubstr("No value assigned to variable `q`")));
}

TEST(MoriartyTest, ValidateAllTestCasesHandFailsIfAVariableIsMissing) {
  Moriarty M;
  M.AddVariable("R", MInteger(Between(1, 3)))
      .AddVariable("q", MInteger(Between(10, 30)));  // Importer uses S, not q
  M.ImportTestCases(
      [](ImportContext ctx) { return ImportTwoIota(ctx, "R", "S", 4); },
      {.validation = ValidationStyle::kNone});

  ValidationResults results = M.ValidateTestCases();
  EXPECT_FALSE(results.IsValid());
  EXPECT_THAT(results.DescribeFailures(),
              HasSubstr("No value assigned to variable `q`"));
}

TEST(MoriartyTest, VariableNameValidationShouldWork) {
  Moriarty().AddVariable("good", MInteger());
  Moriarty().AddVariable("a1_b", MInteger());
  Moriarty().AddVariable("AbC_3c", MInteger());

  EXPECT_THAT([] { Moriarty().AddVariable("", MInteger()); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("empty")));

  EXPECT_THAT([] { Moriarty().AddVariable("1", MInteger()); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("start")));
  EXPECT_THAT([] { Moriarty().AddVariable("_", MInteger()); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("start")));

  EXPECT_THAT([] { Moriarty().AddVariable(" ", MInteger()); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("A-Za-z0-9_")));
  EXPECT_THAT([] { Moriarty().AddVariable("$", MInteger()); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("A-Za-z0-9_")));
  EXPECT_THAT([] { Moriarty().AddVariable("a$", MInteger()); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("A-Za-z0-9_")));
  EXPECT_THAT([] { Moriarty().AddVariable("a b", MInteger()); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("A-Za-z0-9_")));
}

}  // namespace
}  // namespace moriarty
