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

#include "src/variables/minteger.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/contexts/librarian_context.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/errors.h"
#include "src/testing/gtest_helpers.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/constraints/numeric_constraints.h"
#include "src/variables/constraints/size_constraints.h"

namespace moriarty {
namespace {

using ::moriarty_testing::Context;
using ::moriarty_testing::Generate;
using ::moriarty_testing::GeneratedValuesAre;
using ::moriarty_testing::GenerateEdgeCases;
using ::moriarty_testing::GenerateSameValues;
using ::moriarty_testing::GenerateThrowsGenerationError;
using ::moriarty_testing::GetUniqueValue;
using ::moriarty_testing::IsNotSatisfiedWith;
using ::moriarty_testing::IsSatisfiedWith;
using ::moriarty_testing::Print;
using ::moriarty_testing::Read;
using ::moriarty_testing::ThrowsImpossibleToSatisfy;
using ::moriarty_testing::ThrowsVariableNotFound;
using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::HasSubstr;
using ::testing::IsSupersetOf;
using ::testing::Le;
using ::testing::Optional;
using ::testing::Throws;
using ::testing::ThrowsMessage;
using ::testing::UnorderedElementsAre;

TEST(MIntegerTest, TypenameIsCorrect) {
  EXPECT_EQ(MInteger().Typename(), "MInteger");
}

TEST(MIntegerTest, PrintShouldSucceed) {
  EXPECT_EQ(Print(MInteger(), -1), "-1");
  EXPECT_EQ(Print(MInteger(), 0), "0");
  EXPECT_EQ(Print(MInteger(), 1), "1");
}

TEST(MIntegerTest, ValidReadShouldSucceed) {
  EXPECT_EQ(Read(MInteger(), "123"), 123);
  EXPECT_EQ(Read(MInteger(), "456 "), 456);
  EXPECT_EQ(Read(MInteger(), "-789"), -789);

  // Extremes
  int64_t min = std::numeric_limits<int64_t>::min();
  int64_t max = std::numeric_limits<int64_t>::max();
  EXPECT_EQ(Read(MInteger(), std::to_string(min)), min);
  EXPECT_EQ(Read(MInteger(), std::to_string(max)), max);
}

TEST(MIntegerTest, ReadWithTokensAfterwardsIsFine) {
  EXPECT_EQ(Read(MInteger(), "-123 you should ignore this"), -123);
}

TEST(MIntegerTest, InvalidReadShouldFail) {
  // EOF
  EXPECT_THROW({ (void)Read(MInteger(), ""); }, IOError);
  // EOF with whitespace
  EXPECT_THROW({ (void)Read(MInteger(), " "); }, IOError);
  // Double negative
  EXPECT_THROW({ (void)Read(MInteger(), "--123"); }, IOError);
  // Invalid character start/middle/end
  EXPECT_THROW({ (void)Read(MInteger(), "c123"); }, IOError);
  EXPECT_THROW({ (void)Read(MInteger(), "12c3"); }, IOError);
  EXPECT_THROW({ (void)Read(MInteger(), "123c"); }, IOError);
}

TEST(MIntegerTest, ListEdgeCasesIncludesManyInterestingValues) {
  EXPECT_THAT(GenerateEdgeCases(MInteger()),
              IsSupersetOf({0LL, 1LL, 2LL, -1LL, 1LL << 32, (1LL << 62) - 1}));
}

TEST(MIntegerTest, ListEdgeCasesIncludesIntMinAndMaxByDefault) {
  EXPECT_THAT(GenerateEdgeCases(MInteger()),
              IsSupersetOf({std::numeric_limits<int64_t>::min(),
                            std::numeric_limits<int64_t>::max()}));
}

TEST(MIntegerTest, ListEdgeCasesIncludesMinAndMaxValues) {
  EXPECT_THAT(GenerateEdgeCases(MInteger(Between(123, 234))),
              IsSupersetOf({123, 234}));
}

TEST(MIntegerTest, ListEdgeCasesValuesAreNotRepeated) {
  EXPECT_THAT(GenerateEdgeCases(MInteger(Between(-1, 1))),
              UnorderedElementsAre(-1, 0, 1));
}

TEST(MIntegerTest, BetweenShouldRestrictTheRangeProperly) {
  EXPECT_THAT(MInteger(Between(5, 5)), GeneratedValuesAre(5));
  EXPECT_THAT(MInteger(Between(5, 10)),
              GeneratedValuesAre(AllOf(Ge(5), Le(10))));
  EXPECT_THAT(MInteger(Between(-1, 1)),
              GeneratedValuesAre(AllOf(Ge(-1), Le(1))));
}

TEST(MIntegerTest, RepeatedBetweenCallsShouldBeIntersectedTogether) {
  // All possible valid intersections
  EXPECT_TRUE(
      GenerateSameValues(MInteger(Between(0, 30), Between(1, 10)),
                         MInteger(Between(1, 10))));  // First is a superset
  EXPECT_TRUE(
      GenerateSameValues(MInteger(Between(1, 10), Between(0, 30)),
                         MInteger(Between(1, 10))));  // Second is a superset
  EXPECT_TRUE(
      GenerateSameValues(MInteger(Between(0, 10), Between(1, 30)),
                         MInteger(Between(1, 10))));  // First on the left
  EXPECT_TRUE(
      GenerateSameValues(MInteger(Between(1, 30), Between(0, 10)),
                         MInteger(Between(1, 10))));  // First on the right
  EXPECT_TRUE(GenerateSameValues(MInteger(Between(1, 8), Between(8, 10)),
                                 MInteger(Between(8, 8))));  // Singleton Range

  // Several chained calls to Between should work
  EXPECT_TRUE(GenerateSameValues(
      MInteger(Between(1, 20), Between(3, 21), Between(2, 5)),
      MInteger(Between(3, 5))));
}

TEST(MIntegerTest, InvalidBoundsShouldCrash) {
  // Need to use AtMost/AtLeast here since Between will crash on its own.
  // Min > Max
  EXPECT_THAT(MInteger(AtLeast(0), AtMost(-1)),
              GenerateThrowsGenerationError("", Context()));

  // Empty intersection (First interval to the left)
  EXPECT_THAT(MInteger(Between(1, 10), Between(20, 30)),
              GenerateThrowsGenerationError("", Context()));

  // Empty intersection (First interval to the right)
  EXPECT_THAT(MInteger(Between(20, 30), Between(1, 10)),
              GenerateThrowsGenerationError("", Context()));
}

// TODO(darcybest): MInteger should have an equality operator instead of this.
TEST(MIntegerTest, MergeFromCorrectlyMerges) {
  // All possible valid intersections
  EXPECT_TRUE(GenerateSameValues(
      MInteger(Between(0, 30)).MergeFrom(MInteger(Between(1, 10))),
      MInteger(Between(0, 30), Between(1, 10))));  // First superset
  EXPECT_TRUE(GenerateSameValues(
      MInteger(Between(1, 10)).MergeFrom(MInteger(Between(0, 30))),
      MInteger(Between(1, 10), Between(0, 30))));  // Second superset
  EXPECT_TRUE(GenerateSameValues(
      MInteger(Between(0, 10)).MergeFrom(MInteger(Between(1, 30))),
      MInteger(Between(0, 10), Between(1, 30))));  // First on left
  EXPECT_TRUE(GenerateSameValues(
      MInteger(Between(1, 30)).MergeFrom(MInteger(Between(0, 10))),
      MInteger(Between(1, 30), Between(0, 10))));  // First on  right
  EXPECT_TRUE(GenerateSameValues(
      MInteger(Between(1, 8)).MergeFrom(MInteger(Between(8, 10))),
      MInteger(Between(1, 8), Between(8, 10))));  // Singleton Range
}

TEST(MIntegerTest, IsSatisfiedWithWorksForGoodData) {
  EXPECT_THAT(MInteger(Between(1, 10)), IsSatisfiedWith(5));   // Middle
  EXPECT_THAT(MInteger(Between(1, 10)), IsSatisfiedWith(1));   // Low
  EXPECT_THAT(MInteger(Between(1, 10)), IsSatisfiedWith(10));  // High

  // Whole range
  EXPECT_THAT(MInteger(), IsSatisfiedWith(0));
  EXPECT_THAT(MInteger(), IsSatisfiedWith(std::numeric_limits<int64_t>::min()));
  EXPECT_THAT(MInteger(), IsSatisfiedWith(std::numeric_limits<int64_t>::max()));
}

TEST(MIntegerTest, IsSatisfiedWithWorksForBadData) {
  EXPECT_THAT(MInteger(Between(1, 10)), IsNotSatisfiedWith(0, "between"));
  EXPECT_THAT(MInteger(Between(1, 10)), IsNotSatisfiedWith(11, "between"));

  // Empty range
  EXPECT_THAT(MInteger(AtLeast(1), AtMost(-1)),
              AnyOf(IsNotSatisfiedWith(0, "at least"),
                    IsNotSatisfiedWith(0, "at most")));
}

TEST(MIntegerTest, IsSatisfiedWithWithExpressionsShouldWorkForGoodData) {
  EXPECT_THAT(
      MInteger(Between(1, "3 * N + 1")),
      IsSatisfiedWith(5, Context().WithValue<MInteger>("N", 10)));  // Mid
  EXPECT_THAT(
      MInteger(Between(1, "3 * N + 1")),
      IsSatisfiedWith(1, Context().WithValue<MInteger>("N", 10)));  // Lo
  EXPECT_THAT(
      MInteger(Between(1, "3 * N + 1")),
      IsSatisfiedWith(31, Context().WithValue<MInteger>("N", 10)));  // High
}

TEST(MIntegerTest, IsSatisfiedWithWithExpressionsShouldWorkForBadData) {
  EXPECT_THAT(
      MInteger(Between(1, "3 * N + 1")),
      IsNotSatisfiedWith(0, "between", Context().WithValue<MInteger>("N", 10)));

  EXPECT_THAT(MInteger(AtLeast(1), AtMost(-1)),
              AnyOf(IsNotSatisfiedWith(0, "at least"),
                    IsNotSatisfiedWith(0, "at most")));

  moriarty_internal::ValueSet values;
  moriarty_internal::VariableSet variables;
  librarian::AnalysisContext ctx("_", variables, values);
  // Could be VariableNotFound as well (impl detail)
  EXPECT_THAT(
      [&] { (void)MInteger(Between(1, "3 * N + 1")).IsSatisfiedWith(ctx, 2); },
      ThrowsVariableNotFound("N"));
}

TEST(MIntegerTest, AtMostAndAtLeastShouldLimitTheOutputRange) {
  EXPECT_THAT(MInteger(AtMost(10), AtLeast(-5)),
              GeneratedValuesAre(AllOf(Le(10), Ge(-5))));
}

TEST(MIntegerTest, AtMostLargerThanAtLeastShouldFail) {
  EXPECT_THAT(MInteger(AtLeast(10), AtMost(0)),
              GenerateThrowsGenerationError("", Context()));
  EXPECT_THAT(MInteger(AtLeast(10), AtMost("3 * N + 1")),
              GenerateThrowsGenerationError(
                  "", Context().WithValue<MInteger>("N", -3)));
}

TEST(MIntegerTest, AtMostAtLeastBetweenWithUnparsableExpressionsShouldFail) {
  EXPECT_THAT([] { MInteger(AtLeast("3 + ")); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("operation")));
  EXPECT_THAT([] { MInteger(AtMost("+ X +")); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("operation")));
  EXPECT_THAT([] { MInteger(Between("N + 2", "* M + M")); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("operation")));
}

TEST(MIntegerTest, AtMostAndAtLeastWithExpressionsShouldLimitTheOutputRange) {
  EXPECT_THAT(MInteger(AtMost("3 * N + 1"), AtLeast(0)),
              GeneratedValuesAre(AllOf(Ge(0), Le(31)),
                                 Context().WithValue<MInteger>("N", 10)));
  EXPECT_THAT(MInteger(AtLeast("3 * N + 1"), AtMost(50)),
              GeneratedValuesAre(AllOf(Ge(31), Le(50)),
                                 Context().WithValue<MInteger>("N", 10)));
}

TEST(
    MIntegerTest,
    MultipleExpressionsAndConstantsInAtLeastAtMostBetweenShouldRestrictOutput) {
  EXPECT_THAT(
      MInteger(AtLeast(0), AtMost("3 * N + 1"), Between("N + M", 100)),
      GeneratedValuesAre(
          AllOf(Ge(0), Le(31), Ge(25), Le(100)),
          Context().WithValue<MInteger>("N", 10).WithValue<MInteger>("M", 15)));

  EXPECT_THAT(
      MInteger(AtLeast("3 * N + 1"), AtLeast("3 * M + 3"), AtMost(50),
               AtMost("M ^ 2")),
      GeneratedValuesAre(
          AllOf(Ge(19), Ge(18), Le(50), Le(25)),
          Context().WithValue<MInteger>("N", 6).WithValue<MInteger>("M", 5)));
}

TEST(MIntegerTest, AllOverloadsOfExactlyAreEffective) {
  {  // No variables
    EXPECT_THAT(MInteger(Exactly(10)), GeneratedValuesAre(10));
    EXPECT_THAT(MInteger(Exactly("10")), GeneratedValuesAre(10));
    EXPECT_THAT(MInteger(ExactlyIntegerExpression("10")),
                GeneratedValuesAre(10));
  }
  {  // With variables
    EXPECT_THAT(MInteger(Exactly("3 * N + 1")),
                GeneratedValuesAre(31, Context().WithValue<MInteger>("N", 10)));
    EXPECT_THAT(MInteger(ExactlyIntegerExpression("3 * N + 1")),
                GeneratedValuesAre(31, Context().WithValue<MInteger>("N", 10)));
  }
}

TEST(MIntegerTest, AllOverloadsOfBetweenAreEffective) {
  EXPECT_THAT(MInteger(Between(1, 10)),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
  EXPECT_THAT(MInteger(Between(1, "10")),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
  EXPECT_THAT(MInteger(Between("1", 10)),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
  EXPECT_THAT(MInteger(Between("1", "10")),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
}

TEST(MIntegerTest, IsMIntegerExpressionShouldRestrictInput) {
  EXPECT_THAT(MInteger(Exactly("3 * N + 1")),
              GeneratedValuesAre(31, Context().WithValue<MInteger>("N", 10)));
}

TEST(MIntegerTest, GetUniqueValueWorksWhenUniqueValueKnown) {
  EXPECT_THAT(GetUniqueValue(MInteger(Between("N", "N")),
                             Context().WithValue<MInteger>("N", 10)),
              Optional(10));

  EXPECT_THAT(GetUniqueValue(MInteger(Between(20, "2 * N")),
                             Context().WithValue<MInteger>("N", 10)),
              Optional(20));
}

TEST(MIntegerTest, GetUniqueValueWithNestedDependenciesShouldWork) {
  EXPECT_THAT(
      GetUniqueValue(MInteger(Between("X", "Y")),
                     Context()
                         .WithVariable<MInteger>("X", MInteger(Exactly(5)))
                         .WithVariable<MInteger>("Y", MInteger(Between(5, "N")))
                         .WithValue<MInteger>("N", 5)),
      Optional(5));
}

TEST(MIntegerTest, GetUniqueValueFailsWhenAVariableIsUnknown) {
  EXPECT_THAT([&] { (void)GetUniqueValue(MInteger(Between("N", "N"))); },
              ThrowsVariableNotFound("N"));
}

TEST(MIntegerTest, GetUniqueValueShouldSucceedIfTheValueIsUnique) {
  EXPECT_THAT(GetUniqueValue(MInteger(Between(123, 123))), Optional(123));
  EXPECT_THAT(GetUniqueValue(MInteger(Exactly(456))), Optional(456));
}

TEST(MIntegerTest, GetUniqueValueFailsWhenTheValueIsNotUnique) {
  EXPECT_EQ(GetUniqueValue(MInteger(Between(8, 10))), std::nullopt);
}

TEST(MIntegerTest, WithSizeGivesAppropriatelySizedValues) {
  // These values here are fuzzy and may need to be changed over time. "small"
  // might be changed over time. The bounds here are mostly just to check the
  // approximate sizes are considered.

  EXPECT_THAT(MInteger(Between(1, "10^9"), SizeCategory::Min()),
              GeneratedValuesAre(Eq(1)));
  EXPECT_THAT(MInteger(Between(1, "10^9"), SizeCategory::Tiny()),
              GeneratedValuesAre(Le(30)));
  EXPECT_THAT(MInteger(Between(1, "10^9"), SizeCategory::Small()),
              GeneratedValuesAre(Le(2000)));
  EXPECT_THAT(MInteger(Between(1, "10^9"), SizeCategory::Medium()),
              GeneratedValuesAre(Le(1000000)));
  EXPECT_THAT(MInteger(Between(1, "10^9"), SizeCategory::Large()),
              GeneratedValuesAre(Ge(1000000)));
  EXPECT_THAT(MInteger(Between(1, "10^9"), SizeCategory::Huge()),
              GeneratedValuesAre(Ge(500000000)));
  EXPECT_THAT(MInteger(Between(1, "10^9"), SizeCategory::Max()),
              GeneratedValuesAre(Eq(1000000000)));
}

TEST(MIntegerTest, WithSizeBehavesWithMergeFrom) {
  MInteger small = MInteger(Between(1, "10^9"), SizeCategory::Small());
  MInteger tiny = MInteger(Between(1, "10^9"), SizeCategory::Tiny());
  MInteger large = MInteger(Between(1, "10^9"), SizeCategory::Large());
  MInteger any = MInteger(Between(1, "10^9"), SizeCategory::Any());

  {
    EXPECT_FALSE(GenerateSameValues(small, tiny));
    small.MergeFrom(tiny);
    EXPECT_TRUE(GenerateSameValues(small, tiny));
  }
  {
    EXPECT_FALSE(GenerateSameValues(any, large));
    any.MergeFrom(large);
    EXPECT_TRUE(GenerateSameValues(any, large));
  }

  EXPECT_THAT([&] { return small.MergeFrom(large); },
              Throws<std::runtime_error>());
}

TEST(MIntegerTest, InvalidSizeCombinationsShouldThrow) {
  EXPECT_THAT(
      [] { return MInteger(SizeCategory::Small(), SizeCategory::Large()); },
      Throws<std::runtime_error>());
  EXPECT_THAT(
      [] { return MInteger(SizeCategory::Small(), SizeCategory::Max()); },
      Throws<std::runtime_error>());
  EXPECT_THAT(
      [] {
        return MInteger(SizeCategory::Small(), SizeCategory::Tiny())
            .MergeFrom(MInteger(SizeCategory::Huge()));
      },
      Throws<std::runtime_error>());
  EXPECT_THAT(
      [] { return MInteger(SizeCategory::Tiny(), SizeCategory::Medium()); },
      Throws<std::runtime_error>());
}

TEST(MIntegerTest, InvalidExpressionsShouldFail) {
  EXPECT_THAT([] { (void)Generate(MInteger(Exactly("N + "))); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("operation")));
  EXPECT_THAT([] { (void)Generate(MInteger(AtMost("N + "))); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("operation")));
  EXPECT_THAT([] { (void)Generate(MInteger(AtLeast("N + "))); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("operation")));
  EXPECT_THAT(
      [] { (void)Generate(MInteger(Between("& x", "N + "))); },
      ThrowsMessage<std::invalid_argument>(HasSubstr("Unknown character")));
}

TEST(MIntegerTest, ExactlyAndOneOfConstraintsWithNoVariablesShouldWork) {
  {  // IsSatisfiedWith
    EXPECT_THAT(MInteger(Exactly(5)), IsSatisfiedWith(5));
    EXPECT_THAT(MInteger(Exactly(5)), IsNotSatisfiedWith(6, "exactly"));

    EXPECT_THAT(
        MInteger(OneOf({5, 6, 7})),
        AllOf(IsSatisfiedWith(5), IsSatisfiedWith(6), IsSatisfiedWith(7)));
    EXPECT_THAT(MInteger(OneOf({5, 6, 7})), IsNotSatisfiedWith(8, "one of"));

    EXPECT_THAT(MInteger(Exactly(5), OneOf({5, 6, 7})), IsSatisfiedWith(5));
    EXPECT_THAT(MInteger(Exactly(5), OneOf({4, 5, 6}), Between(5, 1000000)),
                IsSatisfiedWith(5));
    EXPECT_THAT(MInteger(Exactly(5), OneOf({5, 6, 7})),
                IsNotSatisfiedWith(6, "exactly"));
    EXPECT_THAT([] { MInteger(Exactly(5), OneOf({6, 7, 8})); },
                ThrowsImpossibleToSatisfy("one of"));
  }
  {  // Generate
    EXPECT_THAT(MInteger(Exactly(5)), GeneratedValuesAre(5));
    EXPECT_THAT(MInteger(OneOf({5, 6, 7})), GeneratedValuesAre(AnyOf(5, 6, 7)));
    EXPECT_THAT(MInteger(Exactly(5), OneOf({5, 6, 7})), GeneratedValuesAre(5));
    EXPECT_THAT(MInteger(Exactly(5), OneOf({4, 5, 6}), Between(5, 10)),
                GeneratedValuesAre(5));
    EXPECT_THAT(MInteger(Exactly(5), Between(1, 10)), GeneratedValuesAre(5));
    EXPECT_THAT(MInteger(OneOf({5, 6, 7}), Between(1, 6)),
                GeneratedValuesAre(AnyOf(5, 6)));
  }
  {  // GetUniqueValue
    EXPECT_THAT(GetUniqueValue(MInteger(Exactly(5))), Optional(5));
    EXPECT_THAT(GetUniqueValue(MInteger(OneOf({5, 6, 7}))), std::nullopt);
    EXPECT_THAT(GetUniqueValue(MInteger(OneOf({5, 6, 7, 8}), Exactly(5))),
                Optional(5));
    EXPECT_THAT(GetUniqueValue(MInteger(OneOf({5, 6, 7, 8}), OneOf({9, 7, 4}))),
                Optional(7));
  }
}

TEST(MIntegerTest, ExactlyAndOneOfConstraintsWithVariablesShouldWork) {
  {  // IsSatisfiedWith
    EXPECT_THAT(MInteger(Exactly("N")),
                IsSatisfiedWith(10, Context().WithValue<MInteger>("N", 10)));
    EXPECT_THAT(MInteger(Exactly("N")),
                IsNotSatisfiedWith(11, "exactly",
                                   Context().WithValue<MInteger>("N", 10)));

    EXPECT_THAT(
        MInteger(OneOf({"N", "N+1", "N+2", "123"})),
        AllOf(IsSatisfiedWith(5, Context().WithValue<MInteger>("N", 5)),
              IsSatisfiedWith(6, Context().WithValue<MInteger>("N", 5)),
              IsSatisfiedWith(7, Context().WithValue<MInteger>("N", 5)),
              IsSatisfiedWith(123, Context().WithValue<MInteger>("N", 5))));
    EXPECT_THAT(
        MInteger(OneOf({"N", "N+1", "N+2"})),
        IsNotSatisfiedWith(8, "one of", Context().WithValue<MInteger>("N", 5)));

    EXPECT_THAT(MInteger(Exactly("N"), OneOf({"N-1", "N+1", "N*1", "N/1"})),
                IsSatisfiedWith(5, Context().WithValue<MInteger>("N", 5)));
    EXPECT_THAT(MInteger(Exactly("N + 1"), OneOf({"N-1", "N+1", "N*1", "N/1"}),
                         Between("N-1", 1000)),
                IsSatisfiedWith(6, Context().WithValue<MInteger>("N", 5)));
    EXPECT_THAT(MInteger(Exactly("N"), OneOf({"N", "N+1", "N+2"})),
                IsNotSatisfiedWith(6, "exactly",
                                   Context().WithValue<MInteger>("N", 5)));
    EXPECT_THAT(
        MInteger(Exactly("N"), OneOf({6, 7, 8})),
        IsNotSatisfiedWith(5, "one of", Context().WithValue<MInteger>("N", 5)));
    EXPECT_THAT(
        MInteger(Exactly("N"), OneOf({"N+1", "N+2"})),
        IsNotSatisfiedWith(5, "one of", Context().WithValue<MInteger>("N", 5)));
  }
  {  // Generate
    EXPECT_THAT(MInteger(Exactly(5)), GeneratedValuesAre(5));
    EXPECT_THAT(MInteger(OneOf({5, 6, 7})), GeneratedValuesAre(AnyOf(5, 6, 7)));
    EXPECT_THAT(MInteger(Exactly(5), OneOf({5, 6, 7})), GeneratedValuesAre(5));
    EXPECT_THAT(MInteger(Exactly(5), OneOf({4, 5, 6}), Between(5, 10)),
                GeneratedValuesAre(5));
    EXPECT_THAT(MInteger(Exactly(5), Between(1, 10)), GeneratedValuesAre(5));
    EXPECT_THAT(MInteger(OneOf({5, 6, 7}), Between(1, 6)),
                GeneratedValuesAre(AnyOf(5, 6)));
  }
  {  // GetUniqueValue
    EXPECT_THAT(GetUniqueValue(MInteger(Exactly("N")),
                               Context().WithValue<MInteger>("N", 10)),
                Optional(10));
    EXPECT_THAT(GetUniqueValue(MInteger(OneOf({"N", "N+1", "N+4"})),
                               Context().WithValue<MInteger>("N", 10)),
                std::nullopt);

    EXPECT_THAT(GetUniqueValue(MInteger(OneOf({"N", "N+1"}), Exactly("N")),
                               Context().WithValue<MInteger>("N", 10)),
                Optional(10));

    // We cannot figure this out today, but we should be able to with a little
    // work.
    // EXPECT_THAT(GetUniqueValue(MInteger(OneOf({"N", "N+1"}), OneOf({9, 10})),
    //                            Context().WithValue<MInteger>("N", 10)),
    //             Optional(10));
  }
}

}  // namespace
}  // namespace moriarty
