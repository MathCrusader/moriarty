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

#include "src/simple_io.h"

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "absl/log/absl_check.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/context.h"
#include "src/internal/variable_set.h"
#include "src/io_config.h"
#include "src/test_case.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace {

using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::StrEq;
using ::testing::VariantWith;

// -----------------------------------------------------------------------------
//  SimpleIO

TEST(SimpleIOTest, AddLineIsRetrievableViaLinesPerTestCase) {
  SimpleIO s;
  s.AddLine("hello", "world!").AddLine("how", StringLiteral("are"), "you?");

  EXPECT_THAT(
      s.LinesPerTestCase(),
      ElementsAre(ElementsAre(VariantWith<std::string>("hello"),
                              VariantWith<std::string>("world!")),
                  ElementsAre(VariantWith<std::string>("how"),
                              VariantWith<StringLiteral>(StringLiteral("are")),
                              VariantWith<std::string>("you?"))));
}

TEST(SimpleIOTest, AddLineWithSpanIsRetrievableViaLinesPerTestCase) {
  SimpleIO s;
  s.AddLine(std::vector<std::string>{"hello", "world!"})
      .AddLine("how", StringLiteral("are"), "you?");

  EXPECT_THAT(
      s.LinesPerTestCase(),
      ElementsAre(ElementsAre(VariantWith<std::string>("hello"),
                              VariantWith<std::string>("world!")),
                  ElementsAre(VariantWith<std::string>("how"),
                              VariantWith<StringLiteral>(StringLiteral("are")),
                              VariantWith<std::string>("you?"))));
}

TEST(SimpleIOTest, AddHeaderLineIsRetrievableViaLinesInHeader) {
  SimpleIO s;
  s.AddHeaderLine(std::vector<std::string>{"hello", "header!"})
      .AddHeaderLine("how", StringLiteral("are"), "you?");

  EXPECT_THAT(
      s.LinesInHeader(),
      ElementsAre(ElementsAre(VariantWith<std::string>("hello"),
                              VariantWith<std::string>("header!")),
                  ElementsAre(VariantWith<std::string>("how"),
                              VariantWith<StringLiteral>(StringLiteral("are")),
                              VariantWith<std::string>("you?"))));
}

TEST(SimpleIOTest, AddHeaderLineIsRetrievableViaLinesInFooter) {
  SimpleIO s;
  s.AddFooterLine(std::vector<std::string>{"hello", "footer!"})
      .AddFooterLine("how", StringLiteral("are"), "you?");

  EXPECT_THAT(
      s.LinesInFooter(),
      ElementsAre(ElementsAre(VariantWith<std::string>("hello"),
                              VariantWith<std::string>("footer!")),
                  ElementsAre(VariantWith<std::string>("how"),
                              VariantWith<StringLiteral>(StringLiteral("are")),
                              VariantWith<std::string>("you?"))));
}

TEST(SimpleIOTest, UsingAllAddLineVariationsDoNotInteractPoorly) {
  SimpleIO s;
  s.AddFooterLine("footer").AddHeaderLine("header").AddLine("line");

  EXPECT_THAT(s.LinesInFooter(),
              ElementsAre(ElementsAre(VariantWith<std::string>("footer"))));
  EXPECT_THAT(s.LinesInHeader(),
              ElementsAre(ElementsAre(VariantWith<std::string>("header"))));
  EXPECT_THAT(s.LinesPerTestCase(),
              ElementsAre(ElementsAre(VariantWith<std::string>("line"))));
}

// -----------------------------------------------------------------------------
//  SimpleIOExporter

TEST(SimpleIOExporterTest, ExporterSimpleCaseShouldWork) {
  moriarty_internal::VariableSet variables;
  ABSL_CHECK_OK(variables.AddVariable("N", MInteger()));

  std::vector<ConcreteTestCase> test_cases = {
      ConcreteTestCase().SetValue<MInteger>("N", 10),
      ConcreteTestCase().SetValue<MInteger>("N", 20),
      ConcreteTestCase().SetValue<MInteger>("N", 30),
  };

  std::stringstream ss;
  ExportContext ctx(ss, variables, {});
  ExportFn exporter = SimpleIO().AddLine("N").Exporter();

  exporter(ctx, test_cases);
  EXPECT_THAT(ss.str(), StrEq("10\n20\n30\n"));
}

TEST(SimpleIOExporterTest, ExportHeaderAndFooterLinesShouldWork) {
  moriarty_internal::VariableSet variables;
  ABSL_CHECK_OK(variables.AddVariable("a", MInteger()));
  ABSL_CHECK_OK(variables.AddVariable("b", MInteger()));
  ABSL_CHECK_OK(variables.AddVariable("c", MInteger()));

  std::vector<ConcreteTestCase> test_cases = {ConcreteTestCase()
                                                  .SetValue<MInteger>("a", 10)
                                                  .SetValue<MInteger>("b", 20)
                                                  .SetValue<MInteger>("c", 30),
                                              ConcreteTestCase()
                                                  .SetValue<MInteger>("a", 11)
                                                  .SetValue<MInteger>("b", 21)
                                                  .SetValue<MInteger>("c", 31),
                                              ConcreteTestCase()
                                                  .SetValue<MInteger>("a", 12)
                                                  .SetValue<MInteger>("b", 22)
                                                  .SetValue<MInteger>("c", 32)};

  std::stringstream ss;
  ExportContext ctx(ss, variables, {});
  ExportFn exporter = SimpleIO()
                          .AddHeaderLine(StringLiteral("start"))
                          .AddLine(StringLiteral("line"), "a", "b", "c")
                          .AddFooterLine(StringLiteral("end"))
                          .Exporter();

  exporter(ctx, test_cases);
  EXPECT_THAT(ss.str(), StrEq(R"(start
line 10 20 30
line 11 21 31
line 12 22 32
end
)"));
}

TEST(SimpleIOExporterTest, ExportWithNumberOfTestCasesShouldPrintProperly) {
  moriarty_internal::VariableSet variables;
  ABSL_CHECK_OK(variables.AddVariable("a", MInteger()));
  ABSL_CHECK_OK(variables.AddVariable("b", MInteger()));
  ABSL_CHECK_OK(variables.AddVariable("c", MInteger()));

  std::vector<ConcreteTestCase> test_cases = {ConcreteTestCase()
                                                  .SetValue<MInteger>("a", 10)
                                                  .SetValue<MInteger>("b", 20)
                                                  .SetValue<MInteger>("c", 30),
                                              ConcreteTestCase()
                                                  .SetValue<MInteger>("a", 11)
                                                  .SetValue<MInteger>("b", 21)
                                                  .SetValue<MInteger>("c", 31)};

  std::stringstream ss;
  ExportContext ctx(ss, variables, {});
  ExportFn exporter = SimpleIO()
                          .WithNumberOfTestCasesInHeader()
                          .AddLine("a", "b", "c")
                          .Exporter();

  exporter(ctx, test_cases);
  EXPECT_THAT(ss.str(), StrEq(R"(2
10 20 30
11 21 31
)"));
}

// -----------------------------------------------------------------------------
//  SimpleIOImporter

MATCHER_P2(VariableIs, variable_name, value, "") {
  ConcreteTestCase tc = arg;
  return tc.GetValue<MInteger>(variable_name) == value;
}

TEST(SimpleIOImporterTest, ImportInBasicCaseShouldWork) {
  moriarty_internal::VariableSet variables;
  ABSL_CHECK_OK(variables.AddVariable("R", MInteger()));
  ABSL_CHECK_OK(variables.AddVariable("S", MInteger()));

  std::stringstream ss("1 11\n2 22\n3 33\n4 44\n");
  ImportFn importer = SimpleIO().AddLine("R", "S").Importer(4);

  ImportContext ctx(variables, ss, WhitespaceStrictness::kPrecise);

  EXPECT_THAT(importer(ctx),
              ElementsAre(AllOf(VariableIs("R", 1), VariableIs("S", 11)),
                          AllOf(VariableIs("R", 2), VariableIs("S", 22)),
                          AllOf(VariableIs("R", 3), VariableIs("S", 33)),
                          AllOf(VariableIs("R", 4), VariableIs("S", 44))));
}

TEST(SimpleIOImporterTest, ExportHeaderAndFooterLinesShouldWork) {
  moriarty_internal::VariableSet variables;
  ABSL_CHECK_OK(variables.AddVariable("R", MInteger()));
  ABSL_CHECK_OK(variables.AddVariable("S", MInteger()));

  std::stringstream ss(R"(hello
1 XX
11
2 XX
22
3 XX
33
4 XX
44
end
)");
  ImportFn importer = SimpleIO()
                          .AddHeaderLine(StringLiteral("hello"))
                          .AddLine("R", StringLiteral("XX"))
                          .AddLine("S")
                          .AddFooterLine(StringLiteral("end"))
                          .Importer(4);

  ImportContext ctx(variables, ss, WhitespaceStrictness::kPrecise);

  EXPECT_THAT(importer(ctx),
              ElementsAre(AllOf(VariableIs("R", 1), VariableIs("S", 11)),
                          AllOf(VariableIs("R", 2), VariableIs("S", 22)),
                          AllOf(VariableIs("R", 3), VariableIs("S", 33)),
                          AllOf(VariableIs("R", 4), VariableIs("S", 44))));
}

TEST(SimpleIOImporterTest, ImportWrongTokenFails) {
  std::stringstream ss("these are wrong words");
  ImportFn importer =
      SimpleIO()
          .AddLine(StringLiteral("these"), StringLiteral("are"),
                   StringLiteral("right"), StringLiteral("words"))
          .Importer();

  ImportContext ctx(moriarty_internal::VariableSet(), ss,
                    WhitespaceStrictness::kPrecise);
  EXPECT_THROW({ importer(ctx); }, std::runtime_error);
}

TEST(SimpleIOImporterTest, ImportWrongWhitespaceFails) {
  moriarty_internal::VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("R", MInteger()));
  MORIARTY_ASSERT_OK(variables.AddVariable("S", MInteger()));

  std::stringstream ss("1\t11\n");
  ImportFn importer = SimpleIO().AddLine("R", "S").Importer();

  ImportContext ctx(moriarty_internal::VariableSet(), ss,
                    WhitespaceStrictness::kPrecise);
  EXPECT_THROW({ importer(ctx); }, std::runtime_error);
}

TEST(SimpleIOImporterTest, ImportWithNumberOfTestCasesInHeaderShouldWork) {
  moriarty_internal::VariableSet variables;
  ABSL_CHECK_OK(variables.AddVariable("R", MInteger()));
  ABSL_CHECK_OK(variables.AddVariable("S", MInteger()));

  std::stringstream ss("4\n1 11\n2 22\n3 33\n4 44\n");
  ImportFn importer =
      SimpleIO().WithNumberOfTestCasesInHeader().AddLine("R", "S").Importer();

  ImportContext ctx(variables, ss, WhitespaceStrictness::kPrecise);

  EXPECT_THAT(importer(ctx),
              ElementsAre(AllOf(VariableIs("R", 1), VariableIs("S", 11)),
                          AllOf(VariableIs("R", 2), VariableIs("S", 22)),
                          AllOf(VariableIs("R", 3), VariableIs("S", 33)),
                          AllOf(VariableIs("R", 4), VariableIs("S", 44))));
}

TEST(SimpleIOImporterTest,
     ImportWithNumberOfTestCasesInHeaderFailsOnTooHighNumberOfCases) {
  moriarty_internal::VariableSet variables;
  ABSL_CHECK_OK(variables.AddVariable("R", MInteger()));
  ABSL_CHECK_OK(variables.AddVariable("S", MInteger()));

  std::stringstream ss("6\n1 11\n2 22\n3 33\n4 44\n");
  ImportFn importer =
      SimpleIO().WithNumberOfTestCasesInHeader().AddLine("R", "S").Importer();

  ImportContext ctx(variables, ss, WhitespaceStrictness::kPrecise);
  EXPECT_THROW({ importer(ctx); }, std::runtime_error);
}

TEST(SimpleIOImporterTest,
     ImportWithNumberOfTestCasesInHeaderFailsOnNegativeNumberOfCases) {
  moriarty_internal::VariableSet variables;
  ABSL_CHECK_OK(variables.AddVariable("R", MInteger()));
  ABSL_CHECK_OK(variables.AddVariable("S", MInteger()));

  std::stringstream ss("-44\n1 11\n2 22\n3 33\n4 44\n");
  ImportFn importer =
      SimpleIO().WithNumberOfTestCasesInHeader().AddLine("R", "S").Importer();

  ImportContext ctx(variables, ss, WhitespaceStrictness::kPrecise);
  EXPECT_THROW({ importer(ctx); }, std::runtime_error);
}

TEST(SimpleIOImporterTest,
     ImportWithNumberOfTestCasesInHeaderFailsOnNonInteger) {
  moriarty_internal::VariableSet variables;
  ABSL_CHECK_OK(variables.AddVariable("R", MInteger()));
  ABSL_CHECK_OK(variables.AddVariable("S", MInteger()));

  std::stringstream ss("hello\n1 11\n2 22\n3 33\n4 44\n");
  ImportFn importer =
      SimpleIO().WithNumberOfTestCasesInHeader().AddLine("R", "S").Importer();

  ImportContext ctx(variables, ss, WhitespaceStrictness::kPrecise);
  EXPECT_THROW({ importer(ctx); }, std::runtime_error);
}

}  // namespace
}  // namespace moriarty
