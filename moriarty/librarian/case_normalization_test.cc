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

#include "moriarty/librarian/case_normalization.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "gtest/gtest.h"
#include "moriarty/librarian/policies.h"
#include "moriarty/types/graph.h"

namespace moriarty {
namespace librarian {
namespace {

template <typename L, typename R>
concept CanCompare = requires(const L& lhs, const R& rhs) {
  AreEqual(lhs, rhs, CaseNormalization::kNone);
};

TEST(CaseNormalizationTest, RejectsMixedNumericTypes) {
  static_assert(!CanCompare<int, int64_t>);
  static_assert(!CanCompare<int, double>);
  static_assert(!CanCompare<char, int>);
  static_assert(!CanCompare<int, char>);
  static_assert(CanCompare<char, char>);
  static_assert(CanCompare<int, int>);
  static_assert(CanCompare<double, double>);
}

TEST(CaseNormalizationTest, RejectsDifferentTextAndContainerTypes) {
  static_assert(!CanCompare<std::string, std::string_view>);
  static_assert(!CanCompare<std::vector<int>, std::vector<long>>);
}

TEST(CaseNormalizationTest, DefaultsAndOperationPoliciesPreserveCase) {
  EXPECT_EQ(ConstraintOptions().case_normalization, CaseNormalization::kNone);
  EXPECT_EQ(GenerationConstraintOptions().case_normalization,
            CaseNormalization::kNone);
  EXPECT_EQ(WritingConstraintOptions().case_normalization,
            CaseNormalization::kNone);
  EXPECT_EQ(AnalysisConstraintOptions().case_normalization,
            CaseNormalization::kNone);
}

TEST(CaseNormalizationTest, ConvertsAsciiLetters) {
  const std::string lower = "abcdefghijklmnopqrstuvwxyz";
  const std::string upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  EXPECT_EQ(NormalizeCase(upper, CaseNormalization::kLowercase), lower);
  EXPECT_EQ(NormalizeCase(lower, CaseNormalization::kLowercase), lower);
  EXPECT_EQ(NormalizeCase(lower, CaseNormalization::kUppercase), upper);
  EXPECT_EQ(NormalizeCase(upper, CaseNormalization::kUppercase), upper);
  EXPECT_EQ(NormalizeCase('A', CaseNormalization::kLowercase), 'a');
  EXPECT_EQ(NormalizeCase('z', CaseNormalization::kUppercase), 'Z');
}

TEST(CaseNormalizationTest, PreservesEveryByteWithNone) {
  std::string bytes;
  for (int byte = 0; byte < 256; ++byte) {
    char c = static_cast<char>(byte);
    EXPECT_EQ(NormalizeCase(c, CaseNormalization::kNone), c);
    bytes.push_back(c);
  }
  EXPECT_EQ(NormalizeCase(bytes, CaseNormalization::kNone), bytes);
}

TEST(CaseNormalizationTest, PreservesNonLettersIncludingNonAsciiBytes) {
  std::string bytes;
  for (int byte = 0; byte < 256; ++byte) {
    if ((byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z')) continue;
    char c = static_cast<char>(byte);
    EXPECT_EQ(NormalizeCase(c, CaseNormalization::kLowercase), c);
    EXPECT_EQ(NormalizeCase(c, CaseNormalization::kUppercase), c);
    bytes.push_back(c);
  }
  EXPECT_EQ(NormalizeCase(bytes, CaseNormalization::kLowercase), bytes);
  EXPECT_EQ(NormalizeCase(bytes, CaseNormalization::kUppercase), bytes);
}

TEST(CaseNormalizationTest, ConvertsMixedTextWithoutChangingOriginal) {
  const std::string original = "  PuSh\t123\n";
  EXPECT_EQ(NormalizeCase(original, CaseNormalization::kLowercase),
            "  push\t123\n");
  EXPECT_EQ(NormalizeCase(original, CaseNormalization::kUppercase),
            "  PUSH\t123\n");
  EXPECT_EQ(original, "  PuSh\t123\n");
}

TEST(CaseNormalizationTest, HandlesEmptyStringsAndEmbeddedNulls) {
  for (CaseNormalization policy :
       {CaseNormalization::kNone, CaseNormalization::kLowercase,
        CaseNormalization::kUppercase}) {
    EXPECT_EQ(NormalizeCase("", policy), "");
  }
  EXPECT_EQ(
      NormalizeCase(std::string("A\0Z", 3), CaseNormalization::kLowercase),
      std::string("a\0z", 3));
  EXPECT_EQ(
      NormalizeCase(std::string("a\0z", 3), CaseNormalization::kUppercase),
      std::string("A\0Z", 3));
}

TEST(CaseNormalizationTest, EqualityRespectsCasePolicy) {
  EXPECT_TRUE(AreEqual(std::string_view("PuSh"), std::string_view("PuSh"),
                       CaseNormalization::kNone));
  EXPECT_FALSE(AreEqual(std::string_view("PuSh"), std::string_view("push"),
                        CaseNormalization::kNone));
  for (CaseNormalization policy :
       {CaseNormalization::kLowercase, CaseNormalization::kUppercase}) {
    EXPECT_TRUE(
        AreEqual(std::string_view("PuSh"), std::string_view("push"), policy));
    EXPECT_TRUE(
        AreEqual(std::string_view("push"), std::string_view("PUSH"), policy));
    EXPECT_FALSE(
        AreEqual(std::string_view("push"), std::string_view("pop"), policy));
    EXPECT_FALSE(
        AreEqual(std::string_view("push"), std::string_view("push!"), policy));
    EXPECT_FALSE(
        AreEqual(std::string_view("push!"), std::string_view("push"), policy));
    EXPECT_FALSE(AreEqual(std::string_view("push "), std::string_view("push\t"),
                          policy));
  }
}

TEST(CaseNormalizationTest, EqualityHandlesEmptyStringsAndEmbeddedNulls) {
  for (CaseNormalization policy :
       {CaseNormalization::kNone, CaseNormalization::kLowercase,
        CaseNormalization::kUppercase}) {
    EXPECT_TRUE(AreEqual(std::string_view(""), std::string_view(""), policy));
    EXPECT_FALSE(AreEqual(std::string_view(""), std::string_view("a"), policy));
    EXPECT_FALSE(AreEqual(std::string_view("a"), std::string_view(""), policy));
    EXPECT_FALSE(AreEqual(std::string_view("a\0b", 3),
                          std::string_view("a\0c", 3), policy));
  }
  EXPECT_TRUE(AreEqual(std::string_view("A\0B", 3), std::string_view("a\0b", 3),
                       CaseNormalization::kLowercase));
}

TEST(CaseNormalizationTest, EqualityPreservesNonAsciiBytes) {
  for (CaseNormalization policy :
       {CaseNormalization::kNone, CaseNormalization::kLowercase,
        CaseNormalization::kUppercase}) {
    EXPECT_TRUE(AreEqual(std::string_view("\xc3\x89"),
                         std::string_view("\xc3\x89"), policy));
    EXPECT_FALSE(AreEqual(std::string_view("\xc3\x89"),
                          std::string_view("\xc3\xa9"), policy));
  }
}

TEST(CaseNormalizationTest, VectorComparisonChecksLetterCaseWithExactCase) {
  EXPECT_FALSE(AreEqual(std::vector<std::string>{"A", "B"},
                        std::vector<std::string>{"a", "b"},
                        CaseNormalization::kNone));
}

TEST(CaseNormalizationTest,
     VectorComparisonRejectsDifferentSizesWithExactCase) {
  EXPECT_FALSE(AreEqual(std::vector<std::string>{"A", "B"},
                        std::vector<std::string>{"a"},
                        CaseNormalization::kNone));
}

TEST(CaseNormalizationTest, VectorComparisonPreservesOrderWithExactCase) {
  EXPECT_FALSE(AreEqual(std::vector<std::string>{"A", "B"},
                        std::vector<std::string>{"b", "a"},
                        CaseNormalization::kNone));
}

TEST(CaseNormalizationTest, VectorComparisonAcceptsEmptyVectorsWithExactCase) {
  EXPECT_TRUE(AreEqual(std::vector<std::string>{}, std::vector<std::string>{},
                       CaseNormalization::kNone));
}

TEST(CaseNormalizationTest,
     CharacterVectorComparisonChecksLetterCaseWithExactCase) {
  EXPECT_FALSE(AreEqual(std::vector<char>{'A'}, std::vector<char>{'a'},
                        CaseNormalization::kNone));
}

TEST(CaseNormalizationTest, BooleanVectorComparisonChecksValuesWithExactCase) {
  EXPECT_TRUE(AreEqual(std::vector<bool>{true, false},
                       std::vector<bool>{true, false},
                       CaseNormalization::kNone));
  EXPECT_FALSE(AreEqual(std::vector<bool>{true}, std::vector<bool>{false},
                        CaseNormalization::kNone));
}

TEST(CaseNormalizationTest, VectorComparisonChecksLetterCaseAfterLowercase) {
  EXPECT_TRUE(AreEqual(std::vector<std::string>{"A", "B"},
                       std::vector<std::string>{"a", "b"},
                       CaseNormalization::kLowercase));
}

TEST(CaseNormalizationTest,
     VectorComparisonRejectsDifferentSizesAfterLowercase) {
  EXPECT_FALSE(AreEqual(std::vector<std::string>{"A", "B"},
                        std::vector<std::string>{"a"},
                        CaseNormalization::kLowercase));
}

TEST(CaseNormalizationTest, VectorComparisonPreservesOrderAfterLowercase) {
  EXPECT_FALSE(AreEqual(std::vector<std::string>{"A", "B"},
                        std::vector<std::string>{"b", "a"},
                        CaseNormalization::kLowercase));
}

TEST(CaseNormalizationTest, VectorComparisonAcceptsEmptyVectorsAfterLowercase) {
  EXPECT_TRUE(AreEqual(std::vector<std::string>{}, std::vector<std::string>{},
                       CaseNormalization::kLowercase));
}

TEST(CaseNormalizationTest,
     CharacterVectorComparisonChecksLetterCaseAfterLowercase) {
  EXPECT_TRUE(AreEqual(std::vector<char>{'A'}, std::vector<char>{'a'},
                       CaseNormalization::kLowercase));
}

TEST(CaseNormalizationTest, BooleanVectorComparisonChecksValuesAfterLowercase) {
  EXPECT_TRUE(AreEqual(std::vector<bool>{true, false},
                       std::vector<bool>{true, false},
                       CaseNormalization::kLowercase));
  EXPECT_FALSE(AreEqual(std::vector<bool>{true}, std::vector<bool>{false},
                        CaseNormalization::kLowercase));
}

TEST(CaseNormalizationTest, VectorComparisonChecksLetterCaseAfterUppercase) {
  EXPECT_TRUE(AreEqual(std::vector<std::string>{"A", "B"},
                       std::vector<std::string>{"a", "b"},
                       CaseNormalization::kUppercase));
}

TEST(CaseNormalizationTest,
     VectorComparisonRejectsDifferentSizesAfterUppercase) {
  EXPECT_FALSE(AreEqual(std::vector<std::string>{"A", "B"},
                        std::vector<std::string>{"a"},
                        CaseNormalization::kUppercase));
}

TEST(CaseNormalizationTest, VectorComparisonPreservesOrderAfterUppercase) {
  EXPECT_FALSE(AreEqual(std::vector<std::string>{"A", "B"},
                        std::vector<std::string>{"b", "a"},
                        CaseNormalization::kUppercase));
}

TEST(CaseNormalizationTest, VectorComparisonAcceptsEmptyVectorsAfterUppercase) {
  EXPECT_TRUE(AreEqual(std::vector<std::string>{}, std::vector<std::string>{},
                       CaseNormalization::kUppercase));
}

TEST(CaseNormalizationTest,
     CharacterVectorComparisonChecksLetterCaseAfterUppercase) {
  EXPECT_TRUE(AreEqual(std::vector<char>{'A'}, std::vector<char>{'a'},
                       CaseNormalization::kUppercase));
}

TEST(CaseNormalizationTest, BooleanVectorComparisonChecksValuesAfterUppercase) {
  EXPECT_TRUE(AreEqual(std::vector<bool>{true, false},
                       std::vector<bool>{true, false},
                       CaseNormalization::kUppercase));
  EXPECT_FALSE(AreEqual(std::vector<bool>{true}, std::vector<bool>{false},
                        CaseNormalization::kUppercase));
}

TEST(CaseNormalizationTest, TuplesCompareEveryMemberWithExactCase) {
  EXPECT_FALSE(AreEqual(std::tuple{std::string("A"), 1},
                        std::tuple{std::string("a"), 1},
                        CaseNormalization::kNone));
  EXPECT_FALSE(AreEqual(std::tuple{std::string("A"), 1},
                        std::tuple{std::string("a"), 2},
                        CaseNormalization::kNone));
  EXPECT_TRUE(
      AreEqual(std::tuple<>{}, std::tuple<>{}, CaseNormalization::kNone));
}

TEST(CaseNormalizationTest, TuplesCompareEveryMemberAfterLowercase) {
  EXPECT_TRUE(AreEqual(std::tuple{std::string("A"), 1},
                       std::tuple{std::string("a"), 1},
                       CaseNormalization::kLowercase));
  EXPECT_FALSE(AreEqual(std::tuple{std::string("A"), 1},
                        std::tuple{std::string("a"), 2},
                        CaseNormalization::kLowercase));
  EXPECT_TRUE(
      AreEqual(std::tuple<>{}, std::tuple<>{}, CaseNormalization::kLowercase));
}

TEST(CaseNormalizationTest, TuplesCompareEveryMemberAfterUppercase) {
  EXPECT_TRUE(AreEqual(std::tuple{std::string("A"), 1},
                       std::tuple{std::string("a"), 1},
                       CaseNormalization::kUppercase));
  EXPECT_FALSE(AreEqual(std::tuple{std::string("A"), 1},
                        std::tuple{std::string("a"), 2},
                        CaseNormalization::kUppercase));
  EXPECT_TRUE(
      AreEqual(std::tuple<>{}, std::tuple<>{}, CaseNormalization::kUppercase));
}

TEST(CaseNormalizationTest, VariantsPreserveAlternativeIdentityWithExactCase) {
  using Variant = std::variant<std::string, std::string>;
  EXPECT_FALSE(AreEqual(Variant(std::in_place_index<1>, "A"),
                        Variant(std::in_place_index<1>, "a"),
                        CaseNormalization::kNone));
  EXPECT_FALSE(AreEqual(Variant(std::in_place_index<0>, "a"),
                        Variant(std::in_place_index<1>, "a"),
                        CaseNormalization::kNone));
}

TEST(CaseNormalizationTest, VariantsPreserveAlternativeIdentityAfterLowercase) {
  using Variant = std::variant<std::string, std::string>;
  EXPECT_TRUE(AreEqual(Variant(std::in_place_index<1>, "A"),
                       Variant(std::in_place_index<1>, "a"),
                       CaseNormalization::kLowercase));
  EXPECT_FALSE(AreEqual(Variant(std::in_place_index<0>, "a"),
                        Variant(std::in_place_index<1>, "a"),
                        CaseNormalization::kLowercase));
}

TEST(CaseNormalizationTest, VariantsPreserveAlternativeIdentityAfterUppercase) {
  using Variant = std::variant<std::string, std::string>;
  EXPECT_TRUE(AreEqual(Variant(std::in_place_index<1>, "A"),
                       Variant(std::in_place_index<1>, "a"),
                       CaseNormalization::kUppercase));
  EXPECT_FALSE(AreEqual(Variant(std::in_place_index<0>, "a"),
                        Variant(std::in_place_index<1>, "a"),
                        CaseNormalization::kUppercase));
}

TEST(CaseNormalizationTest, ComparesNestedContainersWithExactCase) {
  using Choice = std::variant<std::string, std::string>;
  using Count = std::tuple<std::string, int>;
  using Row = std::tuple<Choice, std::vector<Count>>;
  const std::vector<Row> uppercase{
      Row{Choice(std::in_place_index<1>, "A"), std::vector{Count{"B", 2}}}};
  const std::vector<Row> lowercase{
      Row{Choice(std::in_place_index<1>, "a"), std::vector{Count{"b", 2}}}};
  const std::vector<Row> different_count{
      Row{Choice(std::in_place_index<1>, "a"), std::vector{Count{"b", 3}}}};

  EXPECT_FALSE(AreEqual(uppercase, lowercase, CaseNormalization::kNone));
  EXPECT_FALSE(AreEqual(uppercase, different_count, CaseNormalization::kNone));
}

TEST(CaseNormalizationTest, ComparesNestedContainersAfterLowercase) {
  using Choice = std::variant<std::string, std::string>;
  using Count = std::tuple<std::string, int>;
  using Row = std::tuple<Choice, std::vector<Count>>;
  const std::vector<Row> uppercase{
      Row{Choice(std::in_place_index<1>, "A"), std::vector{Count{"B", 2}}}};
  const std::vector<Row> lowercase{
      Row{Choice(std::in_place_index<1>, "a"), std::vector{Count{"b", 2}}}};
  const std::vector<Row> different_count{
      Row{Choice(std::in_place_index<1>, "a"), std::vector{Count{"b", 3}}}};

  EXPECT_TRUE(AreEqual(uppercase, lowercase, CaseNormalization::kLowercase));
  EXPECT_FALSE(
      AreEqual(uppercase, different_count, CaseNormalization::kLowercase));
}

TEST(CaseNormalizationTest, ComparesNestedContainersAfterUppercase) {
  using Choice = std::variant<std::string, std::string>;
  using Count = std::tuple<std::string, int>;
  using Row = std::tuple<Choice, std::vector<Count>>;
  const std::vector<Row> uppercase{
      Row{Choice(std::in_place_index<1>, "A"), std::vector{Count{"B", 2}}}};
  const std::vector<Row> lowercase{
      Row{Choice(std::in_place_index<1>, "a"), std::vector{Count{"b", 2}}}};
  const std::vector<Row> different_count{
      Row{Choice(std::in_place_index<1>, "a"), std::vector{Count{"b", 3}}}};

  EXPECT_TRUE(AreEqual(uppercase, lowercase, CaseNormalization::kUppercase));
  EXPECT_FALSE(
      AreEqual(uppercase, different_count, CaseNormalization::kUppercase));
}

TEST(CaseNormalizationTest, DoesNotCopyContainerElementsWithExactCase) {
  struct NonCopyable {
    int value;
    explicit NonCopyable(int v) : value(v) {}
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable(NonCopyable&&) = default;
    bool operator==(const NonCopyable&) const = default;
  };
  std::vector<NonCopyable> lhs, rhs;
  lhs.emplace_back(3);
  rhs.emplace_back(3);
  EXPECT_TRUE(AreEqual(lhs, rhs, CaseNormalization::kNone));
  rhs[0].value = 4;
  EXPECT_FALSE(AreEqual(lhs, rhs, CaseNormalization::kNone));
}

TEST(CaseNormalizationTest, DoesNotCopyContainerElementsAfterLowercase) {
  struct NonCopyable {
    int value;
    explicit NonCopyable(int v) : value(v) {}
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable(NonCopyable&&) = default;
    bool operator==(const NonCopyable&) const = default;
  };
  std::vector<NonCopyable> lhs, rhs;
  lhs.emplace_back(3);
  rhs.emplace_back(3);
  EXPECT_TRUE(AreEqual(lhs, rhs, CaseNormalization::kLowercase));
  rhs[0].value = 4;
  EXPECT_FALSE(AreEqual(lhs, rhs, CaseNormalization::kLowercase));
}

TEST(CaseNormalizationTest, DoesNotCopyContainerElementsAfterUppercase) {
  struct NonCopyable {
    int value;
    explicit NonCopyable(int v) : value(v) {}
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable(NonCopyable&&) = default;
    bool operator==(const NonCopyable&) const = default;
  };
  std::vector<NonCopyable> lhs, rhs;
  lhs.emplace_back(3);
  rhs.emplace_back(3);
  EXPECT_TRUE(AreEqual(lhs, rhs, CaseNormalization::kUppercase));
  rhs[0].value = 4;
  EXPECT_FALSE(AreEqual(lhs, rhs, CaseNormalization::kUppercase));
}

TEST(CaseNormalizationTest, HandlesValuelessVariantsWithExactCase) {
  struct ThrowingMove {
    ThrowingMove() = default;
    ThrowingMove(ThrowingMove&&) { throw std::runtime_error("move"); }
    ThrowingMove& operator=(ThrowingMove&&) = default;
    bool operator==(const ThrowingMove&) const = default;
  };
  using Variant = std::variant<int, ThrowingMove>;
  Variant lhs(0), rhs(0), source(std::in_place_index<1>);
  EXPECT_THROW(lhs = std::move(source), std::runtime_error);
  EXPECT_THROW(rhs = std::move(source), std::runtime_error);
  ASSERT_TRUE(lhs.valueless_by_exception());
  ASSERT_TRUE(rhs.valueless_by_exception());
  EXPECT_TRUE(AreEqual(lhs, rhs, CaseNormalization::kNone));
  EXPECT_FALSE(AreEqual(lhs, Variant(0), CaseNormalization::kNone));
}

TEST(CaseNormalizationTest, HandlesValuelessVariantsAfterLowercase) {
  struct ThrowingMove {
    ThrowingMove() = default;
    ThrowingMove(ThrowingMove&&) { throw std::runtime_error("move"); }
    ThrowingMove& operator=(ThrowingMove&&) = default;
    bool operator==(const ThrowingMove&) const = default;
  };
  using Variant = std::variant<int, ThrowingMove>;
  Variant lhs(0), rhs(0), source(std::in_place_index<1>);
  EXPECT_THROW(lhs = std::move(source), std::runtime_error);
  EXPECT_THROW(rhs = std::move(source), std::runtime_error);
  ASSERT_TRUE(lhs.valueless_by_exception());
  ASSERT_TRUE(rhs.valueless_by_exception());
  EXPECT_TRUE(AreEqual(lhs, rhs, CaseNormalization::kLowercase));
  EXPECT_FALSE(AreEqual(lhs, Variant(0), CaseNormalization::kLowercase));
}

TEST(CaseNormalizationTest, HandlesValuelessVariantsAfterUppercase) {
  struct ThrowingMove {
    ThrowingMove() = default;
    ThrowingMove(ThrowingMove&&) { throw std::runtime_error("move"); }
    ThrowingMove& operator=(ThrowingMove&&) = default;
    bool operator==(const ThrowingMove&) const = default;
  };
  using Variant = std::variant<int, ThrowingMove>;
  Variant lhs(0), rhs(0), source(std::in_place_index<1>);
  EXPECT_THROW(lhs = std::move(source), std::runtime_error);
  EXPECT_THROW(rhs = std::move(source), std::runtime_error);
  ASSERT_TRUE(lhs.valueless_by_exception());
  ASSERT_TRUE(rhs.valueless_by_exception());
  EXPECT_TRUE(AreEqual(lhs, rhs, CaseNormalization::kUppercase));
  EXPECT_FALSE(AreEqual(lhs, Variant(0), CaseNormalization::kUppercase));
}

TEST(CaseNormalizationTest, GraphsCompareLabelsAndStructureWithExactCase) {
  Graph<std::string, std::string> lhs(2), rhs(2);
  lhs.SetNodeLabels({"A", "B"});
  rhs.SetNodeLabels({"a", "b"});
  lhs.AddEdge(0, 1, "EDGE");
  rhs.AddEdge(0, 1, "edge");
  EXPECT_FALSE(AreEqual(lhs, rhs, CaseNormalization::kNone));
  EXPECT_FALSE(
      AreEqual(std::vector{lhs}, std::vector{rhs}, CaseNormalization::kNone));

  Graph<std::string, std::string> reversed(2);
  reversed.SetNodeLabels({"a", "b"});
  reversed.AddEdge(1, 0, "edge");
  EXPECT_FALSE(AreEqual(lhs, reversed, CaseNormalization::kNone));
  EXPECT_FALSE(AreEqual(lhs, Graph<std::string, std::string>(3),
                        CaseNormalization::kNone));
  rhs.SetNodeLabels({"a", "c"});
  EXPECT_FALSE(AreEqual(lhs, rhs, CaseNormalization::kNone));
}

TEST(CaseNormalizationTest, GraphsCompareLabelsAndStructureAfterLowercase) {
  Graph<std::string, std::string> lhs(2), rhs(2);
  lhs.SetNodeLabels({"A", "B"});
  rhs.SetNodeLabels({"a", "b"});
  lhs.AddEdge(0, 1, "EDGE");
  rhs.AddEdge(0, 1, "edge");
  EXPECT_TRUE(AreEqual(lhs, rhs, CaseNormalization::kLowercase));
  EXPECT_TRUE(AreEqual(std::vector{lhs}, std::vector{rhs},
                       CaseNormalization::kLowercase));

  Graph<std::string, std::string> reversed(2);
  reversed.SetNodeLabels({"a", "b"});
  reversed.AddEdge(1, 0, "edge");
  EXPECT_FALSE(AreEqual(lhs, reversed, CaseNormalization::kLowercase));
  EXPECT_FALSE(AreEqual(lhs, Graph<std::string, std::string>(3),
                        CaseNormalization::kLowercase));
  rhs.SetNodeLabels({"a", "c"});
  EXPECT_FALSE(AreEqual(lhs, rhs, CaseNormalization::kLowercase));
}

TEST(CaseNormalizationTest, GraphsCompareLabelsAndStructureAfterUppercase) {
  Graph<std::string, std::string> lhs(2), rhs(2);
  lhs.SetNodeLabels({"A", "B"});
  rhs.SetNodeLabels({"a", "b"});
  lhs.AddEdge(0, 1, "EDGE");
  rhs.AddEdge(0, 1, "edge");
  EXPECT_TRUE(AreEqual(lhs, rhs, CaseNormalization::kUppercase));
  EXPECT_TRUE(AreEqual(std::vector{lhs}, std::vector{rhs},
                       CaseNormalization::kUppercase));

  Graph<std::string, std::string> reversed(2);
  reversed.SetNodeLabels({"a", "b"});
  reversed.AddEdge(1, 0, "edge");
  EXPECT_FALSE(AreEqual(lhs, reversed, CaseNormalization::kUppercase));
  EXPECT_FALSE(AreEqual(lhs, Graph<std::string, std::string>(3),
                        CaseNormalization::kUppercase));
  rhs.SetNodeLabels({"a", "c"});
  EXPECT_FALSE(AreEqual(lhs, rhs, CaseNormalization::kUppercase));
}

TEST(CaseNormalizationTest, GraphLabelsCanContainContainersWithExactCase) {
  Graph<std::vector<std::string>> lhs(1), rhs(1);
  lhs.AddEdge(0, 0, {"A"});
  rhs.AddEdge(0, 0, {"a"});
  EXPECT_FALSE(AreEqual(lhs, rhs, CaseNormalization::kNone));
  rhs.AddEdge(0, 0, {"b"});
  EXPECT_FALSE(AreEqual(lhs, rhs, CaseNormalization::kNone));
}

TEST(CaseNormalizationTest, GraphLabelsCanContainContainersAfterLowercase) {
  Graph<std::vector<std::string>> lhs(1), rhs(1);
  lhs.AddEdge(0, 0, {"A"});
  rhs.AddEdge(0, 0, {"a"});
  EXPECT_TRUE(AreEqual(lhs, rhs, CaseNormalization::kLowercase));
  rhs.AddEdge(0, 0, {"b"});
  EXPECT_FALSE(AreEqual(lhs, rhs, CaseNormalization::kLowercase));
}

TEST(CaseNormalizationTest, GraphLabelsCanContainContainersAfterUppercase) {
  Graph<std::vector<std::string>> lhs(1), rhs(1);
  lhs.AddEdge(0, 0, {"A"});
  rhs.AddEdge(0, 0, {"a"});
  EXPECT_TRUE(AreEqual(lhs, rhs, CaseNormalization::kUppercase));
  rhs.AddEdge(0, 0, {"b"});
  EXPECT_FALSE(AreEqual(lhs, rhs, CaseNormalization::kUppercase));
}

}  // namespace
}  // namespace librarian
}  // namespace moriarty
