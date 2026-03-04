// Copyright 2026 Darcy Best
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

#include "src/internal/value_printer.h"

#include <cstdint>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace custom_printer_test {

struct GraphLike {
  int num_nodes;
  int num_edges;
};

std::string PrettyPrintValue(const GraphLike& graph, int max_len) {
  std::string full = "Graph(n=" + std::to_string(graph.num_nodes) +
                     ", m=" + std::to_string(graph.num_edges) + ")";
  if (full.size() <= max_len) return full;
  return "Graph(...)";
}

}  // namespace custom_printer_test

namespace moriarty {
namespace moriarty_internal {
namespace {

using ::testing::HasSubstr;

TEST(ValuePrinterTest, SimpleIntegerWorks) {
  EXPECT_EQ(ValuePrinter(5), "5");
  EXPECT_EQ(ValuePrinter(-10), "-10");
  EXPECT_EQ(ValuePrinter(0), "0");
  EXPECT_EQ(ValuePrinter(1234567890123456789ULL), "1234567890123456789");
}

TEST(ValuePrinterTest, FloatingPointWorks) {
  EXPECT_THAT(ValuePrinter(0.0), HasSubstr("0"));
  EXPECT_THAT(ValuePrinter(-0.0), HasSubstr("-0"));

  EXPECT_THAT(ValuePrinter(1.5), HasSubstr("1.5"));
  EXPECT_THAT(ValuePrinter(-0.125), HasSubstr("-0.125"));

  long double value = 3.25L;
  EXPECT_THAT(ValuePrinter(value), HasSubstr("3.25"));
}

TEST(ValuePrinterTest, AdlCustomPrinterInDifferentNamespaceWorks) {
  custom_printer_test::GraphLike graph{5, 8};
  EXPECT_EQ(ValuePrinter(graph), "Graph(n=5, m=8)");
  EXPECT_EQ(ValuePrinter(graph, 4), "Graph(...)");
}

TEST(ValuePrinterTest, SmallStringsWork) {
  EXPECT_EQ(ValuePrinter("hello"), "\"hello\"");
  EXPECT_EQ(ValuePrinter("world"), "\"world\"");
  EXPECT_EQ(ValuePrinter(""), "\"\"");
  EXPECT_EQ(ValuePrinter("1234567890"), "\"1234567890\"");
}

TEST(ValuePrinterTest, StringLengthLimitWorks) {
  EXPECT_EQ(ValuePrinter("abcdefghij", 6), "\"ab...\"");
  EXPECT_EQ(ValuePrinter("abcdefghij", 12), "\"abcdefghij\"");
}

TEST(ValuePrinterTest, StringVerySmallBudgetStillShowsCharacters) {
  EXPECT_EQ(ValuePrinter("abcdefghij", 1), "\"ab...\"");
  EXPECT_EQ(ValuePrinter("abcdefghij", 2), "\"ab...\"");
}

TEST(ValuePrinterTest, CharacterFormattingWorks) {
  EXPECT_EQ(ValuePrinter('A'), "'A'");
  EXPECT_EQ(ValuePrinter(static_cast<unsigned char>('B')), "'B'");
  EXPECT_EQ(ValuePrinter(static_cast<signed char>(7)), "ASCII 7");
}

TEST(ValuePrinterTest, OptionalWorks) {
  EXPECT_EQ(ValuePrinter(std::optional<int>{}), "(empty optional)");
  EXPECT_EQ(ValuePrinter(std::optional<int>{42}), "42");
  EXPECT_EQ(ValuePrinter(std::optional<std::string>{"hello"}), "\"hello\"");
}

TEST(ValuePrinterTest, OptionalRespectsNestedBudget) {
  EXPECT_EQ(ValuePrinter(std::optional<std::string>{"abcdefghij"}, 6),
            "\"ab...\"");
}

TEST(ValuePrinterTest, VariantWorks) {
  std::variant<int, std::string> v = 42;
  EXPECT_EQ(ValuePrinter(v), "42");
  v = std::string("hello");
  EXPECT_EQ(ValuePrinter(v), "\"hello\"");
}

TEST(ValuePrinterTest, VariantWithNestedContainerWorks) {
  std::variant<std::vector<int>, std::string> v = std::vector<int>{1, 2, 3, 4};
  EXPECT_EQ(ValuePrinter(v), "[1, 2, 3, 4]");
  v = std::string("hello");
  EXPECT_EQ(ValuePrinter(v), "\"hello\"");
}

TEST(ValuePrinterTest, TupleWorks) {
  std::tuple<int, std::string, int> t{1, "abc", 3};
  EXPECT_EQ(ValuePrinter(t), "(1, \"abc\", 3)");
}

TEST(ValuePrinterTest, TupleWithMoreThanMinCountAddsEllipsis) {
  std::tuple<int, int, int, int> t{1, 2, 3, 4};
  EXPECT_EQ(ValuePrinter(t), "(1, 2, 3, 4)");
}

TEST(ValuePrinterTest, TupleNestedTypesWork) {
  std::tuple<std::optional<int>, std::variant<int, std::string>> t{
      std::optional<int>{5}, std::variant<int, std::string>{"x"}};
  EXPECT_EQ(ValuePrinter(t), "(5, \"x\")");
}

TEST(ValuePrinterTest, PairWorks) {
  std::pair<int, std::string> p{7, "xy"};
  EXPECT_EQ(ValuePrinter(p), "(7, \"xy\")");
}

TEST(ValuePrinterTest, PairWithNestedContainerWorks) {
  std::pair<int, std::vector<int>> p{7, {1, 2, 3, 4}};
  EXPECT_EQ(ValuePrinter(p), "(7, [1, 2, 3, 4])");
}

TEST(ValuePrinterTest, ContainerWorks) {
  EXPECT_EQ(ValuePrinter(std::vector<int>{}), "[]");
  EXPECT_EQ(ValuePrinter(std::vector<int>{1, 2, 3}), "[1, 2, 3]");
  EXPECT_EQ(ValuePrinter(std::vector<int>{1, 2, 3, 4}), "[1, 2, 3, 4]");
}

TEST(ValuePrinterTest, ContainerForcedMinCountBudgetingWorks) {
  std::vector<std::string> values = {"abcdefghij", "klmnopqrst", "uvwxyzabcd",
                                     "efghijklmn"};
  EXPECT_EQ(ValuePrinter(values, 20), "[\"ab...\", \"kl...\", \"uv...\", ...]");
}

TEST(ValuePrinterTest, NestedContainerWorks) {
  std::vector<std::vector<int>> values = {{1, 2, 3, 4}, {5, 6}, {7, 8}};
  EXPECT_EQ(ValuePrinter(values), "[[1, 2, 3, 4], [5, 6], [7, 8]]");

  std::vector<std::vector<std::string>> str_values = {
      {"is", "this", "the", "real", "life"},
      {"is", "this", "just", "fantasy"},
      {"caught", "in", "a", "landslide"},
      {"no", "escape", "from", "reality"}};
  EXPECT_EQ(ValuePrinter(str_values, 100),
            "[[\"is\", \"this\", \"the\", \"real\", ...], [\"is\", \"this\", "
            "\"just\", ...], [\"caught\", \"in\", \"a\", ...], ...]");
}

TEST(ValuePrinterTest, ContainerOfOptionalWorks) {
  std::vector<std::optional<int>> values = {1, std::nullopt, 3};
  EXPECT_EQ(ValuePrinter(values), "[1, (empty optional), 3]");
}

TEST(ValuePrinterTest, ContainerRespectsElementBudgeting) {
  std::vector<std::string> values = {"abcdefghij", "klmnopqrst", "uvwxyzabcd",
                                     "efghijklmn"};
  EXPECT_EQ(ValuePrinter(values, 12), "[\"ab...\", \"kl...\", \"uv...\", ...]");
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
