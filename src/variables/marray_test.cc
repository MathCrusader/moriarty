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

#include "src/variables/marray.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <sstream>
#include <unordered_set>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/constraints/base_constraints.h"
#include "src/constraints/container_constraints.h"
#include "src/constraints/numeric_constraints.h"
#include "src/constraints/string_constraints.h"
#include "src/contexts/librarian_context.h"
#include "src/internal/abstract_variable.h"
#include "src/librarian/errors.h"
#include "src/librarian/io_config.h"
#include "src/librarian/mvariable.h"
#include "src/librarian/policies.h"
#include "src/librarian/testing/gtest_helpers.h"
#include "src/variables/minteger.h"
#include "src/variables/mstring.h"
#include "src/variables/mtuple.h"

namespace moriarty {
namespace {

using ::moriarty_testing::Context;
using ::moriarty_testing::GeneratedValuesAre;
using ::moriarty_testing::GenerateEdgeCases;
using ::moriarty_testing::GenerateSameValues;
using ::moriarty_testing::GenerateThrowsGenerationError;
using ::moriarty_testing::IsNotSatisfiedWith;
using ::moriarty_testing::IsSatisfiedWith;
using ::moriarty_testing::Print;
using ::moriarty_testing::Read;
using ::moriarty_testing::ThrowsVariableNotFound;
using ::testing::AllOf;
using ::testing::Each;
using ::testing::ElementsAre;
using ::testing::Ge;
using ::testing::IsEmpty;
using ::testing::IsSupersetOf;
using ::testing::Le;
using ::testing::Not;
using ::testing::SizeIs;
using ::testing::Throws;
using ::testing::UnorderedElementsAre;

TEST(MArrayTest, TypenameIsCorrect) {
  EXPECT_EQ(MArray(MInteger()).Typename(), "MArray<MInteger>");
  EXPECT_EQ(NestedMArray(MArray(MInteger())).Typename(),
            "MArray<MArray<MInteger>>");
  EXPECT_EQ(MArray(MTuple(MInteger(), MInteger(), MInteger())).Typename(),
            "MArray<MTuple<MInteger, MInteger, MInteger>>");
  EXPECT_EQ(
      MArray(MTuple(MInteger(), MTuple(MInteger(), MInteger()))).Typename(),
      "MArray<MTuple<MInteger, MTuple<MInteger, MInteger>>>");
}

MATCHER(HasDuplicateIntegers,
        negation ? "has no duplicate values" : "has duplicate values") {
  std::unordered_set<int64_t> seen;
  for (int64_t c : arg) {
    auto [it, inserted] = seen.insert(c);
    if (!inserted) {
      *result_listener << "duplicate integer = " << c;
      return true;
    }
  }
  return false;
}
TEST(MArrayTest, TypicalPrintCaseWorks) {
  EXPECT_EQ(Print(MArray<MInteger>(Length(6)), {1, 2, 3, 4, 5, 6}),
            "1 2 3 4 5 6");
  EXPECT_EQ(Print(MArray<MString>(Length(5)),
                  {"Hello,", "World!", "Welcome", "to", "Moriarty!"}),
            "Hello, World! Welcome to Moriarty!");
}

TEST(MArrayTest, ReadingTheWrongLengthOfMArrayShouldFail) {
  EXPECT_THROW((void)Read(MArray<MInteger>(Length(6)), "1 2 3 4"), IOError);
}

TEST(MArrayTest, SimpleGenerateCaseWorks) {
  EXPECT_THAT(MArray<MInteger>(Elements<MInteger>(Between(1, 10)),
                               Length(Between(5, 50))),
              GeneratedValuesAre(AllOf(SizeIs(AllOf(Ge(5), Le(50))),
                                       Each(AllOf(Ge(1), Le(10))))));
}

TEST(MArrayTest, NestedMArrayWorks) {
  EXPECT_THAT(
      NestedMArray(
          MArray<MInteger>(Elements<MInteger>(Between(1, 10)), Length(3)),
          Length(5)),
      GeneratedValuesAre(AllOf(
          SizeIs(5), Each(AllOf(SizeIs(3), Each(AllOf(Ge(1), Le(10))))))));
  {
    // Silly deeply nested case
    MArray A = NestedMArray(
        NestedMArray(
            NestedMArray(NestedMArray(MArray<MInteger>(
                                          Elements<MInteger>(Between(1, 100)),
                                          Length(Between(2, 2))),
                                      Length(Between(1, 3))),
                         Length(Between(2, 3))),
            Length(Between(3, 3))),
        Length(Between(2, 3)));

    EXPECT_THAT(
        A, GeneratedValuesAre(AllOf(
               SizeIs(AllOf(Ge(2), Le(3))), Each(SizeIs(AllOf(Ge(3), Le(3)))),
               Each(Each(SizeIs(AllOf(Ge(2), Le(3))))),
               Each(Each(Each(SizeIs(AllOf(Ge(1), Le(3)))))),
               Each(Each(Each(Each(SizeIs(AllOf(Ge(2), Le(2))))))),
               Each(Each(Each(Each(Each(AllOf(Ge(1), Le(100))))))))));
  }
}

TEST(MArrayTest, RepeatedLengthCallsShouldBeIntersectedTogether) {
  auto gen_given_length1 = [](int lo, int hi) {
    return MArray<MInteger>(Elements<MInteger>(Between(1, 10)),
                            Length(Between(lo, hi)));
  };
  auto gen_given_length2 = [](int lo1, int hi1, int lo2, int hi2) {
    return MArray<MInteger>(Elements<MInteger>(Between(1, 10)),
                            Length(Between(lo1, hi1)),
                            Length(Between(lo2, hi2)));
  };

  // All possible valid intersections
  EXPECT_TRUE(
      GenerateSameValues(gen_given_length2(0, 30, 1, 10),
                         gen_given_length1(1, 10)));  // First is superset
  EXPECT_TRUE(
      GenerateSameValues(gen_given_length2(1, 10, 0, 30),
                         gen_given_length1(1, 10)));  // Second is superset
  EXPECT_TRUE(
      GenerateSameValues(gen_given_length2(0, 10, 1, 30),
                         gen_given_length1(1, 10)));  // First on the left
  EXPECT_TRUE(
      GenerateSameValues(gen_given_length2(1, 30, 0, 10),
                         gen_given_length1(1, 10)));  // First on the right
  EXPECT_TRUE(GenerateSameValues(gen_given_length2(1, 8, 8, 10),
                                 gen_given_length1(8, 8)));  // Singleton Range
}

TEST(MArrayTest, InvalidLengthShouldFail) {
  EXPECT_THAT(MArray<MInteger>(Length(-1)),
              GenerateThrowsGenerationError(".length", Context()));
  EXPECT_THAT(MArray<MInteger>(Length(AtMost(10), AtLeast(21))),
              GenerateThrowsGenerationError(".length", Context()));
}

TEST(MArrayTest, LengthZeroProducesTheEmptyArray) {
  EXPECT_THAT(MArray<MInteger>(Length(0)), GeneratedValuesAre(IsEmpty()));
}

TEST(MArrayTest, MergeFromCorrectlyMergesOnLength) {
  // The alphabet is irrelevant for the tests
  auto get_arr = [](int lo, int hi) {
    return MArray<MInteger>(Elements<MInteger>(Between(1, 10)),
                            Length(Between(lo, hi)));
  };

  // All possible valid intersection of length
  EXPECT_TRUE(GenerateSameValues(get_arr(0, 30).MergeFrom(get_arr(1, 10)),
                                 get_arr(1, 10)));  // First is a superset
  EXPECT_TRUE(GenerateSameValues(get_arr(1, 10).MergeFrom(get_arr(0, 30)),
                                 get_arr(1, 10)));  // Second is a superset
  EXPECT_TRUE(GenerateSameValues(get_arr(0, 10).MergeFrom(get_arr(1, 30)),
                                 get_arr(1, 10)));  // First on left
  EXPECT_TRUE(GenerateSameValues(get_arr(1, 30).MergeFrom(get_arr(0, 10)),
                                 get_arr(1, 10)));  // First on right
  EXPECT_TRUE(GenerateSameValues(get_arr(1, 8).MergeFrom(get_arr(8, 10)),
                                 get_arr(8, 8)));  // Singleton range

  EXPECT_THAT(get_arr(1, 6).MergeFrom(get_arr(10, 20)),
              GenerateThrowsGenerationError(".length", Context()));
}

TEST(MArrayTest, MergeFromCorrectlyMergesElementConstraints) {
  auto int_array = [](int lo, int hi) {
    return MArray<MInteger>(Elements<MInteger>(Between(lo, hi)), Length(20));
  };

  EXPECT_TRUE(GenerateSameValues(int_array(1, 10).MergeFrom(int_array(6, 12)),
                                 int_array(6, 10)));

  EXPECT_THAT(int_array(1, 6).MergeFrom(int_array(10, 20)),
              GenerateThrowsGenerationError(".elem[0]", Context()));
}

TEST(MArrayTest, LengthIsSatisfied) {
  // Includes small and large ranges
  for (int size = 0; size < 10; size++) {
    EXPECT_THAT(MArray<MInteger>(Length(Between(size / 2, size))),
                GeneratedValuesAre(SizeIs(AllOf(Ge(size / 2), Le(size)))));
  }
  for (int size = 900; size < 910; size++) {
    EXPECT_THAT(MArray<MInteger>(Length(Between(size / 2, size))),
                GeneratedValuesAre(SizeIs(AllOf(Ge(size / 2), Le(size)))));
  }
}

TEST(MArrayTest, IsSatisfiedWithChecksMArrayContentsProperly) {
  EXPECT_THAT(MArray<MInteger>(Elements<MInteger>(Between(1, 10))),
              IsSatisfiedWith(std::vector<int64_t>({5, 6, 1, 4, 3, 9, 10})));

  EXPECT_THAT(MArray<MInteger>(Elements<MInteger>(Between(1, 10))),
              IsNotSatisfiedWith(std::vector<int64_t>({5, 6, 7, 8, 9, 10, 11}),
                                 "index 6"));
  EXPECT_THAT(
      MArray<MInteger>(Elements<MInteger>(Between(1, 10))),
      IsNotSatisfiedWith(std::vector<int64_t>({5, 6, 0, 8, 9, 10}), "index 2"));
}

TEST(MArrayTest, IsSatisfiedWithChecksMArraySizeWhenItIsVariable) {
  EXPECT_THAT(MArray<MInteger>(Length("N")),
              IsSatisfiedWith(std::vector<int64_t>({10, 20, 30}),
                              Context().WithValue<MInteger>("N", 3)));
}

TEST(MArrayTest, IsSatisfiedWithFailsIfLengthDependsOnAVariableAndIsNotKnown) {
  // If N is definitely unique, it should pass.
  EXPECT_THAT(MArray<MInteger>(Length(3)),
              IsSatisfiedWith(std::vector<int64_t>({10, 20, 30})));
  EXPECT_THAT(
      MArray<MInteger>(Length("N")),
      IsSatisfiedWith(std::vector<int64_t>({10, 20, 30}),
                      Context().WithVariable("N", MInteger(Exactly(3)))));

  // If there is a unique value, then we might be able to determine the length.
  EXPECT_THAT(
      MArray<MInteger>(Length("N")),
      IsSatisfiedWith(std::vector<int64_t>({10, 20, 30}),
                      Context().WithVariable("N", MInteger(Between(3, 3)))));
}

TEST(MArrayTest, IsSatisfiedWithWithNoConstraintsShouldAcceptAnything) {
  EXPECT_THAT(MArray<MInteger>(), IsSatisfiedWith(std::vector<int64_t>({})));
  EXPECT_THAT(MArray<MInteger>(), IsSatisfiedWith(std::vector<int64_t>({1})));
  EXPECT_THAT(MArray<MInteger>(),
              IsSatisfiedWith(std::vector<int64_t>({1, 2, 3})));
}

TEST(MArrayTest, IsSatisfiedWithChecksLengthConstraintsProperly) {
  auto constraints = [](Length length_constraint) {
    return MArray<MInteger>(Elements<MInteger>(Between(1, 10)),
                            std::move(length_constraint));
  };

  {  // Exact length
    EXPECT_THAT(constraints(Length(7)),
                IsSatisfiedWith(std::vector<int64_t>({1, 2, 3, 4, 5, 6, 7})));
    EXPECT_THAT(constraints(Length(Between(7, 7))),
                IsSatisfiedWith(std::vector<int64_t>({1, 2, 3, 4, 5, 6, 7})));
    EXPECT_THAT(constraints(Length(0)),
                IsSatisfiedWith(std::vector<int64_t>({})));

    EXPECT_THAT(constraints(Length(6)),
                IsNotSatisfiedWith(std::vector<int64_t>({1, 2, 3, 4, 5, 6, 7}),
                                   "length"));
    EXPECT_THAT(constraints(Length(8)),
                IsNotSatisfiedWith(std::vector<int64_t>({1, 2, 3, 4, 5, 6, 7}),
                                   "length"));

    EXPECT_THAT(constraints(Length(0)),
                IsNotSatisfiedWith(std::vector<int64_t>({1}), "length"));
    EXPECT_THAT(constraints(Length(1)),
                IsNotSatisfiedWith(std::vector<int64_t>(), "length"));
  }

  {  // Ranges for `Length`
     // Middle
    EXPECT_THAT(constraints(Length(Between(6, 8))),
                IsSatisfiedWith(std::vector<int64_t>({1, 2, 3, 4, 5, 6, 7})));
    // Low
    EXPECT_THAT(constraints(Length(Between(7, 9))),
                IsSatisfiedWith(std::vector<int64_t>({1, 2, 3, 4, 5, 6, 7})));
    // High
    EXPECT_THAT(constraints(Length(Between(5, 7))),
                IsSatisfiedWith(std::vector<int64_t>({1, 2, 3, 4, 5, 6, 7})));
  }
}

TEST(MArrayTest, IsSatisfiedWithShouldCheckForDistinctElements) {
  EXPECT_THAT(MArray<MInteger>(Elements<MInteger>(Between(1, 5)),
                               DistinctElements(), Length(3)),
              IsSatisfiedWith(std::vector<int64_t>({1, 2, 5})));

  EXPECT_THAT(MArray<MInteger>(Elements<MInteger>(Between(1, 5)),
                               DistinctElements(), Length(3)),
              IsNotSatisfiedWith(std::vector<int64_t>({1, 2, 2}), "distinct"));
}

TEST(MArrayTest, WithDistinctElementsReturnsOnlyDistinctValues) {
  EXPECT_THAT(
      MArray<MInteger>(Elements<MInteger>(Between(1, 10)), Length(10),
                       DistinctElements()),
      GeneratedValuesAre(UnorderedElementsAre(1, 2, 3, 4, 5, 6, 7, 8, 9, 10)));

  EXPECT_THAT(MArray<MInteger>(Elements<MInteger>(Between(1, 100)), Length(50),
                               DistinctElements()),
              GeneratedValuesAre(Not(HasDuplicateIntegers())));
}

TEST(MArrayTest, WithDistinctElementsWithNotEnoughDistinctValuesFails) {
  EXPECT_THAT(MArray<MInteger>(Elements<MInteger>(Between(1, 5)), Length(10),
                               DistinctElements()),
              GenerateThrowsGenerationError("", Context()));
}

TEST(MArrayTest, WithDistinctElementsIsAbleToGenerateWithHugeValue) {
  // This is asking for all integers between 1 and 500. This should
  // succeed relatively quickly. Testing if there are enough retries.
  EXPECT_THAT(MArray<MInteger>(Elements<MInteger>(Between(1, 500)), Length(500),
                               DistinctElements()),
              GeneratedValuesAre(SizeIs(500)));
}

TEST(MArrayTest, WhitespaceSeparatorShouldAffectPrint) {
  EXPECT_EQ(
      Print(MArray<MInteger>(MArrayFormat().NewlineSeparated()), {1, 2, 3}),
      "1\n2\n3");  // Note no newline after
}

TEST(MArrayTest, WhitespaceSeparatorShouldAffectRead) {
  EXPECT_EQ(Read(MArray<MInteger>(MArrayFormat().NewlineSeparated(), Length(6)),
                 "1\n2\n3\n4\n5\n6"),
            (std::vector<int64_t>{1, 2, 3, 4, 5, 6}));

  EXPECT_EQ(Read(MArray<MInteger>(
                     MArrayFormat().WithSeparator(Whitespace::kTab), Length(3)),
                 "3\t2\t1"),
            (std::vector<int64_t>{3, 2, 1}));

  // Read wrong whitespace between characters.
  EXPECT_THROW(
      (void)Read(MArray<MInteger>(MArrayFormat().NewlineSeparated(), Length(6)),
                 "1\n2\n3 4\n5\n6"),
      IOError);
}

TEST(MArrayTest, WhitespaceSeparatorWithMultipleSameTypesPasses) {
  EXPECT_EQ(Read(MArray<MInteger>(MArrayFormat().NewlineSeparated(),
                                  MArrayFormat().NewlineSeparated(), Length(6)),
                 "1\n2\n3\n4\n5\n6"),
            (std::vector<int64_t>{1, 2, 3, 4, 5, 6}));
}

TEST(MArrayTest, ReadShouldBeAbleToDetermineLengthFromAnotherVariable) {
  EXPECT_EQ(Read(MArray<MInteger>(Length("N")), "1 2 3",
                 Context().WithValue<MInteger>("N", 3)),
            (std::vector<int64_t>{1, 2, 3}));

  EXPECT_EQ(Read(MArray<MInteger>(Length("N")), "1 2 3",
                 Context().WithVariable("N", MInteger(Between(3, 3)))),
            (std::vector<int64_t>{1, 2, 3}));

  EXPECT_EQ(Read(MArray<MInteger>(Length("N")), "1 2 3",
                 Context().WithVariable("N", MInteger(Exactly<int64_t>(3)))),
            (std::vector<int64_t>{1, 2, 3}));

  EXPECT_EQ(Read(MArray<MInteger>(Length(3)), "1 2 3"),
            (std::vector<int64_t>{1, 2, 3}));

  EXPECT_EQ(Read(MArray<MInteger>(Length(Between(3, 3))), "1 2 3"),
            (std::vector<int64_t>{1, 2, 3}));
}

TEST(MArrayTest,
     ReadShouldFailIfLengthDependsOnAnUnknownVariableOrNonUniqueInteger) {
  EXPECT_THAT([] { (void)Read(MArray<MInteger>(Length("N")), "1 2 3"); },
              ThrowsVariableNotFound("N"));

  EXPECT_THROW(
      [] { (void)Read(MArray<MInteger>(Length(Between(2, 3))), "1 2 3"); }(),
      IOError);

  EXPECT_THROW(
      [] { (void)Read(MArray<MInteger>(Length(Between(3, 4))), "1 2 3"); }(),
      IOError);
}

TEST(MArrayTest, DirectlyUsingChunkedReaderShouldWork) {
  MArray<MInteger> arr(Elements<MInteger>(Between(1, 10)), Length(6));

  Context context;
  std::istringstream input("1\n2\n3\n4\n5\n6\n");
  InputCursor cursor(input, WhitespaceStrictness::kPrecise);
  librarian::ReaderContext ctx("A", cursor, Context().Variables(),
                               Context().Values());
  MArray<MInteger>::Reader reader(ctx, 6, arr);

  for (int i = 0; i < 6; i++) {
    reader.ReadNext(ctx);
    ASSERT_EQ(input.get(), '\n');  // Consume the separator
  }
  EXPECT_THAT(std::move(reader).Finalize(), ElementsAre(1, 2, 3, 4, 5, 6));
}

TEST(MArrayTest, IndirectlyUsingChunkedReaderShouldWork) {
  Context context;
  std::istringstream input("1\n2\n3\n4\n5\n6\n");
  InputCursor cursor(input, WhitespaceStrictness::kPrecise);

  MArray<MInteger> arr(Elements<MInteger>(Between(1, 10)), Length(6));
  std::unique_ptr<moriarty_internal::ChunkedReader> reader =
      static_cast<moriarty_internal::AbstractVariable&>(arr).GetChunkedReader(
          "A", 6, cursor, context.Variables(), context.Values());

  for (int i = 0; i < 6; i++) {
    reader->ReadNext();
    ASSERT_EQ(input.get(), '\n');  // Consume the separator
  }
  std::move(*reader).Finalize();
  EXPECT_THAT(context.Values().Get<MArray<MInteger>>("A"),
              ElementsAre(1, 2, 3, 4, 5, 6));
}

TEST(MArrayTest, ListEdgeCasesContainsLengthCases) {
  EXPECT_THAT(GenerateEdgeCases(MArray<MInteger>(Length(Between(0, 1000)))),
              IsSupersetOf({SizeIs(0), SizeIs(1), SizeIs(1000)}));
}

TEST(MArrayTest, ListEdgeCasesNoLengthFails) {
  EXPECT_THAT([] { (void)GenerateEdgeCases(MArray<MInteger>()); },
              Throws<ConfigurationError>());
}

TEST(MArrayTest, ExactlyConstraintWorks) {
  Exactly<std::vector<int64_t>> x = Exactly(std::vector<int64_t>{1, 2, 3});
  EXPECT_THAT(MArray<MInteger>(x), GeneratedValuesAre(ElementsAre(1, 2, 3)));
}

MATCHER(IsSorted, "is sorted in ascending order") {
  return std::is_sorted(arg.begin(), arg.end());
}
MATCHER_P(IsSortedBy, comp, "is sorted by custom comparator") {
  return std::is_sorted(
      arg.begin(), arg.end(),
      [&](const auto& a, const auto& b) { return std::invoke(comp, a, b); });
}

TEST(MArrayTest, SortedConstraintWorks) {
  EXPECT_THAT(MArray<MInteger>(Length(10), Sorted<MInteger>()),
              GeneratedValuesAre(IsSorted()));
  EXPECT_THAT(MArray<MString>(Elements<MString>(Length(3), Alphabet("abc")),
                              Length(20), Sorted<MString>()),
              GeneratedValuesAre(IsSorted()));
  EXPECT_THAT(MArray<MString>(DistinctElements(),
                              Elements<MString>(Length(3), Alphabet("abc")),
                              Length(20), Sorted<MString>()),
              GeneratedValuesAre(IsSorted()));
  EXPECT_THAT(MArray<MString>(DistinctElements(),
                              Elements<MString>(Length(3), Alphabet("abc")),
                              Length(20), Sorted<MString, std::greater<>>()),
              GeneratedValuesAre(IsSortedBy(std::greater<>{})));

  EXPECT_THAT(MArray<MArray<MInteger>>(
                  DistinctElements(),
                  Elements<MArray<MInteger>>(
                      Length(3), Elements<MInteger>(Between(1, 10))),
                  Length(20), Sorted<MArray<MInteger>>()),
              GeneratedValuesAre(IsSorted()));
}

}  // namespace
}  // namespace moriarty
