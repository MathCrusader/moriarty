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

#include <cstdint>
#include <sstream>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/context.h"
#include "src/librarian/errors.h"
#include "src/librarian/io_config.h"
#include "src/test_case.h"
#include "src/testing/gtest_helpers.h"
#include "src/variables/constraints/io_constraints.h"
#include "src/variables/marray.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace {

using ::moriarty::MArray;
using ::moriarty::MInteger;
using ::moriarty_testing::Context;
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::StrEq;

// -----------------------------------------------------------------------------
//  SimpleIOExporter

TEST(SimpleIOExporterTest, ExporterSimpleCaseShouldWork) {
  Context context = Context().WithVariable("N", MInteger());

  std::vector<ConcreteTestCase> test_cases = {
      ConcreteTestCase().SetValue<MInteger>("N", 10),
      ConcreteTestCase().SetValue<MInteger>("N", 20),
      ConcreteTestCase().SetValue<MInteger>("N", 30),
  };

  std::stringstream ss;
  ExportContext ctx(ss, context.Variables(), context.Values());
  ExportFn exporter = SimpleIO().AddLine("N").Exporter();

  exporter(ctx, test_cases);
  EXPECT_THAT(ss.str(), StrEq("10\n20\n30\n"));
}

TEST(SimpleIOExporterTest, ExportHeaderAndFooterLinesShouldWork) {
  Context context = Context()
                        .WithVariable("a", MInteger())
                        .WithVariable("b", MInteger())
                        .WithVariable("c", MInteger());

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
  ExportContext ctx(ss, context.Variables(), context.Values());
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
  Context context = Context()
                        .WithVariable("a", MInteger())
                        .WithVariable("b", MInteger())
                        .WithVariable("c", MInteger());

  std::vector<ConcreteTestCase> test_cases = {ConcreteTestCase()
                                                  .SetValue<MInteger>("a", 10)
                                                  .SetValue<MInteger>("b", 20)
                                                  .SetValue<MInteger>("c", 30),
                                              ConcreteTestCase()
                                                  .SetValue<MInteger>("a", 11)
                                                  .SetValue<MInteger>("b", 21)
                                                  .SetValue<MInteger>("c", 31)};

  std::stringstream ss;
  ExportContext ctx(ss, context.Variables(), context.Values());
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

TEST(SimpleIOExporterTest, MultilineSectionShouldWork) {
  Context context =
      Context()
          .WithVariable("N", MInteger())
          .WithVariable("A", MArray<MInteger>(IOSeparator::Newline()))
          .WithVariable("B", MArray<MInteger>(IOSeparator::Newline()));

  std::vector<ConcreteTestCase> test_cases = {
      ConcreteTestCase()
          .SetValue<MInteger>("N", 3)
          .SetValue<MArray<MInteger>>("A", std::vector<int64_t>{1, 2, 3})
          .SetValue<MArray<MInteger>>("B", std::vector<int64_t>{11, 22, 33}),
  };

  std::stringstream ss;
  ExportContext ctx(ss, context.Variables(), context.Values());
  ExportFn exporter =
      SimpleIO().AddMultilineSection("(N - 1) + 1", "A", "B").Exporter();

  exporter(ctx, test_cases);
  EXPECT_THAT(ss.str(), StrEq("1 11\n2 22\n3 33\n"));
}

// -----------------------------------------------------------------------------
//  SimpleIOImporter

MATCHER_P2(IntVariableIs, variable_name, value, "") {
  ConcreteTestCase tc = arg;
  return tc.GetValue<MInteger>(variable_name) == value;
}

MATCHER_P2(ArrayIntVariableIs, variable_name, value, "") {
  ConcreteTestCase tc = arg;
  return tc.GetValue<MArray<MInteger>>(variable_name) == value;
}

TEST(SimpleIOImporterTest, ImportInBasicCaseShouldWork) {
  Context context =
      Context().WithVariable("R", MInteger()).WithVariable("S", MInteger());

  std::stringstream ss("1 11\n2 22\n3 33\n4 44\n");
  ImportFn importer = SimpleIO().AddLine("R", "S").Importer(4);

  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);
  ImportContext ctx(context.Variables(), cursor);

  EXPECT_THAT(
      importer(ctx),
      ElementsAre(AllOf(IntVariableIs("R", 1), IntVariableIs("S", 11)),
                  AllOf(IntVariableIs("R", 2), IntVariableIs("S", 22)),
                  AllOf(IntVariableIs("R", 3), IntVariableIs("S", 33)),
                  AllOf(IntVariableIs("R", 4), IntVariableIs("S", 44))));
}

TEST(SimpleIOImporterTest, ExportHeaderAndFooterLinesShouldWork) {
  Context context =
      Context().WithVariable("R", MInteger()).WithVariable("S", MInteger());

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

  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);
  ImportContext ctx(context.Variables(), cursor);

  EXPECT_THAT(
      importer(ctx),
      ElementsAre(AllOf(IntVariableIs("R", 1), IntVariableIs("S", 11)),
                  AllOf(IntVariableIs("R", 2), IntVariableIs("S", 22)),
                  AllOf(IntVariableIs("R", 3), IntVariableIs("S", 33)),
                  AllOf(IntVariableIs("R", 4), IntVariableIs("S", 44))));
}

TEST(SimpleIOImporterTest, ImportWrongTokenFails) {
  std::stringstream ss("these are wrong words");
  ImportFn importer =
      SimpleIO()
          .AddLine(StringLiteral("these"), StringLiteral("are"),
                   StringLiteral("right"), StringLiteral("words"))
          .Importer();

  Context context;
  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);
  ImportContext ctx(context.Variables(), cursor);
  EXPECT_THROW({ importer(ctx); }, IOError);
}

TEST(SimpleIOImporterTest, ImportWrongWhitespaceFails) {
  Context context =
      Context().WithVariable("R", MInteger()).WithVariable("S", MInteger());

  std::stringstream ss("1\t11\n");
  ImportFn importer = SimpleIO().AddLine("R", "S").Importer();

  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);
  ImportContext ctx(context.Variables(), cursor);
  EXPECT_THROW({ importer(ctx); }, IOError);
}

TEST(SimpleIOImporterTest, ImportWithNumberOfTestCasesInHeaderShouldWork) {
  Context context =
      Context().WithVariable("R", MInteger()).WithVariable("S", MInteger());

  std::stringstream ss("4\n1 11\n2 22\n3 33\n4 44\n");
  ImportFn importer =
      SimpleIO().WithNumberOfTestCasesInHeader().AddLine("R", "S").Importer();

  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);
  ImportContext ctx(context.Variables(), cursor);

  EXPECT_THAT(
      importer(ctx),
      ElementsAre(AllOf(IntVariableIs("R", 1), IntVariableIs("S", 11)),
                  AllOf(IntVariableIs("R", 2), IntVariableIs("S", 22)),
                  AllOf(IntVariableIs("R", 3), IntVariableIs("S", 33)),
                  AllOf(IntVariableIs("R", 4), IntVariableIs("S", 44))));
}

TEST(SimpleIOImporterTest,
     ImportWithNumberOfTestCasesInHeaderFailsOnTooHighNumberOfCases) {
  Context context =
      Context().WithVariable("R", MInteger()).WithVariable("S", MInteger());

  std::stringstream ss("6\n1 11\n2 22\n3 33\n4 44\n");
  ImportFn importer =
      SimpleIO().WithNumberOfTestCasesInHeader().AddLine("R", "S").Importer();

  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);
  ImportContext ctx(context.Variables(), cursor);
  EXPECT_THROW({ importer(ctx); }, IOError);
}

TEST(SimpleIOImporterTest,
     ImportWithNumberOfTestCasesInHeaderFailsOnNegativeNumberOfCases) {
  Context context =
      Context().WithVariable("R", MInteger()).WithVariable("S", MInteger());

  std::stringstream ss("-44\n1 11\n2 22\n3 33\n4 44\n");
  ImportFn importer =
      SimpleIO().WithNumberOfTestCasesInHeader().AddLine("R", "S").Importer();

  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);
  ImportContext ctx(context.Variables(), cursor);
  EXPECT_THROW({ importer(ctx); }, IOError);
}

TEST(SimpleIOImporterTest,
     ImportWithNumberOfTestCasesInHeaderFailsOnNonInteger) {
  Context context =
      Context().WithVariable("R", MInteger()).WithVariable("S", MInteger());

  std::stringstream ss("hello\n1 11\n2 22\n3 33\n4 44\n");
  ImportFn importer =
      SimpleIO().WithNumberOfTestCasesInHeader().AddLine("R", "S").Importer();

  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);
  ImportContext ctx(context.Variables(), cursor);
  EXPECT_THROW({ importer(ctx); }, IOError);
}

TEST(SimpleIOImporterTest, MultilineSectionShouldWork) {
  Context context =
      Context()
          .WithVariable("N", MInteger())
          .WithVariable("A", MArray<MInteger>(IOSeparator::Newline()))
          .WithVariable("B", MArray<MInteger>(IOSeparator::Newline()));

  std::vector<ConcreteTestCase> test_cases = {
      ConcreteTestCase()
          .SetValue<MInteger>("N", 3)
          .SetValue<MArray<MInteger>>("A", std::vector<int64_t>{1, 2, 3})
          .SetValue<MArray<MInteger>>("B", std::vector<int64_t>{11, 22, 33}),
  };

  std::stringstream ss("3\n1 11\n2 22\n3 33\n");

  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);
  ImportContext ctx(context.Variables(), cursor);
  ImportFn importer = SimpleIO()
                          .AddLine("N")
                          .AddMultilineSection("(N - 1) + 1", "A", "B")
                          .Importer();

  EXPECT_THAT(importer(ctx),
              ElementsAre(AllOf(
                  IntVariableIs("N", 3),
                  ArrayIntVariableIs("A", std::vector<int64_t>{1, 2, 3}),
                  ArrayIntVariableIs("B", std::vector<int64_t>{11, 22, 33}))));
}

}  // namespace
}  // namespace moriarty
