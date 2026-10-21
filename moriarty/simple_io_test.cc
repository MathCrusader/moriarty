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

#include "moriarty/simple_io.h"

#include <cstdint>
#include <sstream>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "moriarty/context.h"
#include "moriarty/librarian/errors.h"
#include "moriarty/librarian/io_config.h"
#include "moriarty/librarian/testing/gtest_helpers.h"
#include "moriarty/test_case.h"
#include "moriarty/variables/marray.h"
#include "moriarty/variables/minteger.h"

namespace moriarty {
namespace {

using ::moriarty::MArray;
using ::moriarty::MInteger;
using ::moriarty_testing::Context;
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::StrEq;
using ::testing::ThrowsMessage;
using ::testing::UnorderedElementsAre;

// -----------------------------------------------------------------------------
//  SimpleIOWriter

TEST(SimpleIOWriterTest, WriterSimpleCaseShouldWork) {
  Context context = Context().WithVariable("N", MInteger());

  std::vector<TestCase> test_cases = {
      TestCase().SetValue<MInteger>("N", 10),
      TestCase().SetValue<MInteger>("N", 20),
      TestCase().SetValue<MInteger>("N", 30),
  };

  std::stringstream ss;
  WriteContext ctx(ss, context.Variables(), context.Values());
  WriterFn writer = SimpleIO().AddLine("N").Writer();

  writer(ctx, test_cases);
  EXPECT_THAT(ss.str(), StrEq("10\n20\n30\n"));
}

TEST(SimpleIOWriterTest, WriteHeaderAndFooterLinesShouldWork) {
  Context context = Context()
                        .WithVariable("a", MInteger())
                        .WithVariable("b", MInteger())
                        .WithVariable("c", MInteger());

  std::vector<TestCase> test_cases = {TestCase()
                                          .SetValue<MInteger>("a", 10)
                                          .SetValue<MInteger>("b", 20)
                                          .SetValue<MInteger>("c", 30),
                                      TestCase()
                                          .SetValue<MInteger>("a", 11)
                                          .SetValue<MInteger>("b", 21)
                                          .SetValue<MInteger>("c", 31),
                                      TestCase()
                                          .SetValue<MInteger>("a", 12)
                                          .SetValue<MInteger>("b", 22)
                                          .SetValue<MInteger>("c", 32)};

  std::stringstream ss;
  WriteContext ctx(ss, context.Variables(), context.Values());
  WriterFn writer = SimpleIO()
                        .AddHeaderLine(StringLiteral("start"))
                        .AddLine(StringLiteral("line"), "a", "b", "c")
                        .AddFooterLine(StringLiteral("end"))
                        .Writer();

  writer(ctx, test_cases);
  EXPECT_THAT(ss.str(), StrEq(R"(start
line 10 20 30
line 11 21 31
line 12 22 32
end
)"));
}

TEST(SimpleIOWriterTest, WriteWithNumberOfTestCasesShouldWriteProperly) {
  Context context = Context()
                        .WithVariable("a", MInteger())
                        .WithVariable("b", MInteger())
                        .WithVariable("c", MInteger());

  std::vector<TestCase> test_cases = {TestCase()
                                          .SetValue<MInteger>("a", 10)
                                          .SetValue<MInteger>("b", 20)
                                          .SetValue<MInteger>("c", 30),
                                      TestCase()
                                          .SetValue<MInteger>("a", 11)
                                          .SetValue<MInteger>("b", 21)
                                          .SetValue<MInteger>("c", 31)};

  std::stringstream ss;
  WriteContext ctx(ss, context.Variables(), context.Values());
  WriterFn writer = SimpleIO()
                        .WithNumberOfTestCasesInHeader()
                        .AddLine("a", "b", "c")
                        .Writer();

  writer(ctx, test_cases);
  EXPECT_THAT(ss.str(), StrEq(R"(2
10 20 30
11 21 31
)"));
}

TEST(SimpleIOWriterTest, MultilineSectionShouldWork) {
  Context context =
      Context()
          .WithVariable("N", MInteger())
          .WithVariable("A",
                        MArray<MInteger>(MArrayFormat().NewlineSeparated()))
          .WithVariable("B",
                        MArray<MInteger>(MArrayFormat().NewlineSeparated()));

  std::vector<TestCase> test_cases = {
      TestCase()
          .SetValue<MInteger>("N", 3)
          .SetValue<MArray<MInteger>>("A", std::vector<int64_t>{1, 2, 3})
          .SetValue<MArray<MInteger>>("B", std::vector<int64_t>{11, 22, 33}),
  };

  std::stringstream ss;
  WriteContext ctx(ss, context.Variables(), context.Values());
  WriterFn writer =
      SimpleIO().AddMultilineSection("(N - 1) + 1", "A", "B").Writer();

  writer(ctx, test_cases);
  EXPECT_THAT(ss.str(), StrEq("1 11\n2 22\n3 33\n"));
}

TEST(SimpleIOWriterTest, ZeroLineSectionWritesNothing) {
  Context context =
      Context()
          .WithVariable("N", MInteger())
          .WithVariable("A",
                        MArray<MInteger>(MArrayFormat().NewlineSeparated()))
          .WithVariable("B",
                        MArray<MInteger>(MArrayFormat().NewlineSeparated()))
          .WithVariable("T", MInteger());
  std::vector<TestCase> test_cases = {
      TestCase()
          .SetValue<MInteger>("N", 0)
          .SetValue<MArray<MInteger>>("A", std::vector<int64_t>{})
          .SetValue<MArray<MInteger>>("B", std::vector<int64_t>{})
          .SetValue<MInteger>("T", 7),
  };

  std::stringstream ss;
  WriteContext ctx(ss, context.Variables(), context.Values());
  SimpleIO()
      .AddLine("N")
      .AddMultilineSection("N", "A", "B")
      .AddLine("T")
      .Writer()(ctx, test_cases);
  EXPECT_THAT(ss.str(), StrEq("0\n7\n"));
}

TEST(SimpleIOWriterTest, MultilineSectionRejectsMissingRows) {
  Context context =
      Context()
          .WithVariable("N", MInteger())
          .WithVariable("A",
                        MArray<MInteger>(MArrayFormat().NewlineSeparated()))
          .WithVariable("B",
                        MArray<MInteger>(MArrayFormat().NewlineSeparated()))
          .WithVariable("M", MInteger());
  std::vector<TestCase> test_cases = {
      TestCase()
          .SetValue<MInteger>("N", 3)
          .SetValue<MArray<MInteger>>("A", std::vector<int64_t>{7})
          .SetValue<MArray<MInteger>>("B", std::vector<int64_t>{})
          .SetValue<MInteger>("M", 5),
  };

  std::stringstream ss;
  WriteContext ctx(ss, context.Variables(), context.Values());
  WriterFn writer = SimpleIO()
                        .AddLine("N")
                        .AddMultilineSection("N", "A", "B")
                        .AddLine("M")
                        .Writer();
  EXPECT_THAT([&] { writer(ctx, test_cases); },
              ThrowsMessage<WriteError>(StrEq(
                  "Expected variable `A` to print 3 lines, but it printed 1. "
                  "The line was \"7\".")));
}

// -----------------------------------------------------------------------------
//  SimpleIOReader

MATCHER_P2(IntVariableIs, variable_name, value, "") {
  TestCase tc = arg;
  return tc.GetValue<MInteger>(variable_name) == value;
}

MATCHER_P2(ArrayIntVariableIs, variable_name, value, "") {
  TestCase tc = arg;
  return tc.GetValue<MArray<MInteger>>(variable_name) == value;
}

TEST(SimpleIOReaderTest, ReadInBasicCaseShouldWork) {
  Context context =
      Context().WithVariable("R", MInteger()).WithVariable("S", MInteger());

  std::stringstream ss("1 11\n2 22\n3 33\n4 44\n");
  ReaderFn reader = SimpleIO().AddLine("R", "S").Reader(4);

  InputCursor cursor(ss, {}, WhitespaceStrictness::kPrecise);
  ReadContext ctx(context.Variables(), cursor);

  EXPECT_THAT(
      reader(ctx),
      ElementsAre(AllOf(IntVariableIs("R", 1), IntVariableIs("S", 11)),
                  AllOf(IntVariableIs("R", 2), IntVariableIs("S", 22)),
                  AllOf(IntVariableIs("R", 3), IntVariableIs("S", 33)),
                  AllOf(IntVariableIs("R", 4), IntVariableIs("S", 44))));
}

TEST(SimpleIOReaderTest, WriteHeaderAndFooterLinesShouldWork) {
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
  ReaderFn reader = SimpleIO()
                        .AddHeaderLine(StringLiteral("hello"))
                        .AddLine("R", StringLiteral("XX"))
                        .AddLine("S")
                        .AddFooterLine(StringLiteral("end"))
                        .Reader(4);

  InputCursor cursor(ss, {}, WhitespaceStrictness::kPrecise);
  ReadContext ctx(context.Variables(), cursor);

  EXPECT_THAT(
      reader(ctx),
      ElementsAre(AllOf(IntVariableIs("R", 1), IntVariableIs("S", 11)),
                  AllOf(IntVariableIs("R", 2), IntVariableIs("S", 22)),
                  AllOf(IntVariableIs("R", 3), IntVariableIs("S", 33)),
                  AllOf(IntVariableIs("R", 4), IntVariableIs("S", 44))));
}

TEST(SimpleIOReaderTest, ReadWrongTokenFails) {
  std::stringstream ss("these are wrong words");
  ReaderFn reader = SimpleIO()
                        .AddLine(StringLiteral("these"), StringLiteral("are"),
                                 StringLiteral("right"), StringLiteral("words"))
                        .Reader();

  Context context;
  InputCursor cursor(ss, {}, WhitespaceStrictness::kPrecise);
  ReadContext ctx(context.Variables(), cursor);
  EXPECT_THROW({ reader(ctx); }, ReadError);
}

TEST(SimpleIOReaderTest, UnknownVariableKeepsItsErrorType) {
  std::stringstream ss("1\n");
  ReaderFn reader = SimpleIO().AddLine("missing").Reader();
  Context context;
  InputCursor cursor(ss, {}, WhitespaceStrictness::kPrecise);
  ReadContext ctx(context.Variables(), cursor);
  EXPECT_THROW((void)reader(ctx), VariableNotFound);
}

TEST(SimpleIOReaderTest, ReadWrongWhitespaceFails) {
  Context context =
      Context().WithVariable("R", MInteger()).WithVariable("S", MInteger());

  std::stringstream ss("1\t11\n");
  ReaderFn reader = SimpleIO().AddLine("R", "S").Reader();

  InputCursor cursor(ss, {}, WhitespaceStrictness::kPrecise);
  ReadContext ctx(context.Variables(), cursor);
  EXPECT_THROW({ reader(ctx); }, ReadError);
}

TEST(SimpleIOReaderTest, ReadWithNumberOfTestCasesInHeaderShouldWork) {
  Context context =
      Context().WithVariable("R", MInteger()).WithVariable("S", MInteger());

  std::stringstream ss("4\n1 11\n2 22\n3 33\n4 44\n");
  ReaderFn reader =
      SimpleIO().WithNumberOfTestCasesInHeader().AddLine("R", "S").Reader();

  InputCursor cursor(ss, {}, WhitespaceStrictness::kPrecise);
  ReadContext ctx(context.Variables(), cursor);

  EXPECT_THAT(
      reader(ctx),
      ElementsAre(AllOf(IntVariableIs("R", 1), IntVariableIs("S", 11)),
                  AllOf(IntVariableIs("R", 2), IntVariableIs("S", 22)),
                  AllOf(IntVariableIs("R", 3), IntVariableIs("S", 33)),
                  AllOf(IntVariableIs("R", 4), IntVariableIs("S", 44))));
}

TEST(SimpleIOReaderTest,
     ReadWithNumberOfTestCasesInHeaderFailsOnTooHighNumberOfCases) {
  Context context =
      Context().WithVariable("R", MInteger()).WithVariable("S", MInteger());

  std::stringstream ss("6\n1 11\n2 22\n3 33\n4 44\n");
  ReaderFn reader =
      SimpleIO().WithNumberOfTestCasesInHeader().AddLine("R", "S").Reader();

  InputCursor cursor(ss, {}, WhitespaceStrictness::kPrecise);
  ReadContext ctx(context.Variables(), cursor);
  EXPECT_THROW({ reader(ctx); }, ReadError);
}

TEST(SimpleIOReaderTest,
     ReadWithNumberOfTestCasesInHeaderFailsOnNegativeNumberOfCases) {
  Context context =
      Context().WithVariable("R", MInteger()).WithVariable("S", MInteger());

  std::stringstream ss("-44\n1 11\n2 22\n3 33\n4 44\n");
  ReaderFn reader =
      SimpleIO().WithNumberOfTestCasesInHeader().AddLine("R", "S").Reader();

  InputCursor cursor(ss, {}, WhitespaceStrictness::kPrecise);
  ReadContext ctx(context.Variables(), cursor);
  EXPECT_THROW({ reader(ctx); }, ReadError);
}

TEST(SimpleIOReaderTest, ReadWithNumberOfTestCasesInHeaderFailsOnNonInteger) {
  Context context =
      Context().WithVariable("R", MInteger()).WithVariable("S", MInteger());

  std::stringstream ss("hello\n1 11\n2 22\n3 33\n4 44\n");
  ReaderFn reader =
      SimpleIO().WithNumberOfTestCasesInHeader().AddLine("R", "S").Reader();

  InputCursor cursor(ss, {}, WhitespaceStrictness::kPrecise);
  ReadContext ctx(context.Variables(), cursor);
  EXPECT_THROW({ reader(ctx); }, ReadError);
}

TEST(SimpleIOReaderTest, MultilineSectionShouldWork) {
  Context context =
      Context()
          .WithVariable("N", MInteger())
          .WithVariable("A",
                        MArray<MInteger>(MArrayFormat().NewlineSeparated()))
          .WithVariable("B",
                        MArray<MInteger>(MArrayFormat().NewlineSeparated()));

  std::vector<TestCase> test_cases = {
      TestCase()
          .SetValue<MInteger>("N", 3)
          .SetValue<MArray<MInteger>>("A", std::vector<int64_t>{1, 2, 3})
          .SetValue<MArray<MInteger>>("B", std::vector<int64_t>{11, 22, 33}),
  };

  std::stringstream ss("3\n1 11\n2 22\n3 33\n");

  InputCursor cursor(ss, {}, WhitespaceStrictness::kPrecise);
  ReadContext ctx(context.Variables(), cursor);
  ReaderFn reader = SimpleIO()
                        .AddLine("N")
                        .AddMultilineSection("(N - 1) + 1", "A", "B")
                        .Reader();

  EXPECT_THAT(reader(ctx),
              ElementsAre(AllOf(
                  IntVariableIs("N", 3),
                  ArrayIntVariableIs("A", std::vector<int64_t>{1, 2, 3}),
                  ArrayIntVariableIs("B", std::vector<int64_t>{11, 22, 33}))));
}

TEST(SimpleIOReaderTest, ZeroLineSectionReadsNothing) {
  Context context =
      Context()
          .WithVariable("N", MInteger())
          .WithVariable("A",
                        MArray<MInteger>(MArrayFormat().NewlineSeparated()))
          .WithVariable("B",
                        MArray<MInteger>(MArrayFormat().NewlineSeparated()))
          .WithVariable("T", MInteger());
  std::stringstream ss("0\n7\n");
  InputCursor cursor(ss, {}, WhitespaceStrictness::kPrecise);
  ReadContext ctx(context.Variables(), cursor);
  ReaderFn reader = SimpleIO()
                        .AddLine("N")
                        .AddMultilineSection("N", "A", "B")
                        .AddLine("T")
                        .Reader();

  EXPECT_THAT(reader(ctx),
              ElementsAre(AllOf(IntVariableIs("N", 0),
                                ArrayIntVariableIs("A", std::vector<int64_t>{}),
                                ArrayIntVariableIs("B", std::vector<int64_t>{}),
                                IntVariableIs("T", 7))));

  std::stringstream extra_blank_line("0\n\n7\n");
  InputCursor bad_cursor(extra_blank_line, {}, WhitespaceStrictness::kPrecise);
  ReadContext bad_ctx(context.Variables(), bad_cursor);
  EXPECT_THROW(reader(bad_ctx), ReadError);
}

TEST(SimpleIOReaderTest, MultilineSectionRejectsMissingRows) {
  Context context =
      Context()
          .WithVariable("N", MInteger())
          .WithVariable("A",
                        MArray<MInteger>(MArrayFormat().NewlineSeparated()))
          .WithVariable("B",
                        MArray<MInteger>(MArrayFormat().NewlineSeparated()))
          .WithVariable("M", MInteger());
  std::stringstream ss("3\n7\n5\n");
  InputCursor cursor(ss, {}, WhitespaceStrictness::kPrecise);
  ReadContext ctx(context.Variables(), cursor);
  ReaderFn reader = SimpleIO()
                        .AddLine("N")
                        .AddMultilineSection("N", "A", "B")
                        .AddLine("M")
                        .Reader();

  EXPECT_THROW(reader(ctx), ReadError);
}

TEST(SimpleIOTest, DependenciesShouldReturnAllVariablesUsed) {
  SimpleIO io = SimpleIO()
                    .AddHeaderLine("H1", "H2")
                    .AddLine("A", "B", StringLiteral("C"))
                    .AddMultilineSection("2 * N + M", "D", "E")
                    .AddFooterLine(StringLiteral("F"), "G");

  EXPECT_THAT(
      io.GetDependencies(),
      UnorderedElementsAre("H1", "H2", "N", "M", "A", "B", "D", "E", "G"));
}

}  // namespace
}  // namespace moriarty
