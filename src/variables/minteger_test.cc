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
#include "src/contexts/librarian/analysis_context.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/test_utils.h"
#include "src/testing/status_test_util.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/constraints/numeric_constraints.h"
#include "src/variables/constraints/size_constraints.h"

namespace moriarty {
namespace {

using ::moriarty::IsOkAndHolds;
using ::moriarty_testing::Context;
using ::moriarty_testing::Generate;
using ::moriarty_testing::GeneratedValuesAre;
using ::moriarty_testing::GenerateEdgeCases;
using ::moriarty_testing::GenerateSameValues;
using ::moriarty_testing::GetUniqueValue;
using ::moriarty_testing::IsNotSatisfiedWith;
using ::moriarty_testing::IsSatisfiedWith;
using ::moriarty_testing::Print;
using ::moriarty_testing::Read;
using ::moriarty_testing::ThrowsVariableNotFound;
using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::Each;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::IsSupersetOf;
using ::testing::Le;
using ::testing::Optional;
using ::testing::Throws;
using ::testing::UnorderedElementsAre;

TEST(MIntegerTest, TypenameIsCorrect) {
  EXPECT_EQ(MInteger().Typename(), "MInteger");
}

TEST(MIntegerTest, PrintShouldSucceed) {
  EXPECT_THAT(Print(MInteger(), -1), IsOkAndHolds("-1"));
  EXPECT_THAT(Print(MInteger(), 0), IsOkAndHolds("0"));
  EXPECT_THAT(Print(MInteger(), 1), IsOkAndHolds("1"));
}

TEST(MIntegerTest, ValidReadShouldSucceed) {
  EXPECT_THAT(Read(MInteger(), "123"), IsOkAndHolds(123));
  EXPECT_THAT(Read(MInteger(), "456 "), IsOkAndHolds(456));
  EXPECT_THAT(Read(MInteger(), "-789"), IsOkAndHolds(-789));

  // Extremes
  int64_t min = std::numeric_limits<int64_t>::min();
  int64_t max = std::numeric_limits<int64_t>::max();
  EXPECT_THAT(Read(MInteger(), std::to_string(min)), IsOkAndHolds(min));
  EXPECT_THAT(Read(MInteger(), std::to_string(max)), IsOkAndHolds(max));
}

TEST(MIntegerTest, ReadWithTokensAfterwardsIsFine) {
  EXPECT_THAT(Read(MInteger(), "-123 you should ignore this"),
              IsOkAndHolds(-123));
}

TEST(MIntegerTest, InvalidReadShouldFail) {
  // EOF
  EXPECT_THROW({ Read(MInteger(), "").IgnoreError(); }, std::runtime_error);
  // EOF with whitespace
  EXPECT_THROW({ Read(MInteger(), " ").IgnoreError(); }, std::runtime_error);

  // Double negative
  EXPECT_THROW(
      { Read(MInteger(), "--123").IgnoreError(); }, std::runtime_error);
  // Invalid character start/middle/end
  EXPECT_THROW({ Read(MInteger(), "c123").IgnoreError(); }, std::runtime_error);
  EXPECT_THROW({ Read(MInteger(), "12c3").IgnoreError(); }, std::runtime_error);
  EXPECT_THROW({ Read(MInteger(), "123c").IgnoreError(); }, std::runtime_error);
}

TEST(MIntegerTest, GenerateShouldSuccessfullyComplete) {
  moriarty::MInteger variable;
  MORIARTY_EXPECT_OK(Generate(variable));
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
  EXPECT_THROW(
      { Generate(MInteger(AtLeast(0), AtMost(-1))).IgnoreError(); },
      std::runtime_error);

  // Empty intersection (First interval to the left)
  EXPECT_THROW(
      { Generate(MInteger(Between(1, 10), Between(20, 30))).IgnoreError(); },
      std::runtime_error);

  // Empty intersection (First interval to the right)
  EXPECT_THROW(
      { Generate(MInteger(Between(20, 30), Between(1, 10))).IgnoreError(); },
      std::runtime_error);
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
  EXPECT_THROW(
      { Generate(MInteger(AtLeast(10), AtMost(0))).IgnoreError(); },
      std::runtime_error);

  EXPECT_THROW(
      {
        Generate(MInteger(AtLeast(0), AtMost("3 * N + 1")),
                 Context().WithValue<MInteger>("N", -3))
            .IgnoreError();
      },
      std::runtime_error);
}

TEST(MIntegerNonBuilderDeathTest,
     AtMostAtLeastBetweenWithUnparsableExpressionsShouldFail) {
  // We should throw a nice exception here, not ABSL die.
  EXPECT_DEATH({ MInteger(AtLeast("3 + ")); }, "operation");
  EXPECT_DEATH({ MInteger(AtMost("+ X +")); }, "operation");
  EXPECT_DEATH({ MInteger(Between("N + 2", "* M + M")); }, "operation");
}

TEST(MIntegerTest, AtMostAndAtLeastWithExpressionsShouldLimitTheOutputRange) {
  // TODO(darcybest): Make GeneratedValuesAre compatible with Context instead of
  // using GenerateLots().
  EXPECT_THAT(GenerateLots(MInteger(AtLeast(0), AtMost("3 * N + 1")),
                           Context().WithValue<MInteger>("N", 10)),
              IsOkAndHolds(Each(AllOf(Ge(0), Le(31)))));
  EXPECT_THAT(GenerateLots(MInteger(AtLeast("3 * N + 1"), AtMost(50)),
                           Context().WithValue<MInteger>("N", 10)),
              IsOkAndHolds(Each(AllOf(Ge(31), Le(50)))));
}

TEST(
    MIntegerTest,
    MultipleExpressionsAndConstantsInAtLeastAtMostBetweenShouldRestrictOutput) {
  // TODO(darcybest): Make GeneratedValuesAre compatible with Context.
  EXPECT_THAT(
      GenerateLots(
          MInteger(AtLeast(0), AtMost("3 * N + 1"), Between("N + M", 100)),
          Context().WithValue<MInteger>("N", 10).WithValue<MInteger>("M", 15)),
      IsOkAndHolds(Each(AllOf(Ge(0), Le(31), Ge(25), Le(100)))));

  EXPECT_THAT(
      GenerateLots(
          MInteger(AtLeast("3 * N + 1"), AtLeast("3 * M + 3"), AtMost(50),
                   AtMost("M ^ 2")),
          Context().WithValue<MInteger>("N", 6).WithValue<MInteger>("M", 5)),
      IsOkAndHolds(Each(AllOf(Ge(19), Ge(18), Le(50), Le(25)))));
}

TEST(MIntegerTest, AllOverloadsOfExactlyAreEffective) {
  {  // No variables
    EXPECT_THAT(MInteger(Exactly(10)), GeneratedValuesAre(10));
    EXPECT_THAT(MInteger(Exactly("10")), GeneratedValuesAre(10));
    EXPECT_THAT(MInteger(ExactlyIntegerExpression("10")),
                GeneratedValuesAre(10));
  }
  {  // With variables
    EXPECT_THAT(GenerateLots(MInteger(Exactly("3 * N + 1")),
                             Context().WithValue<MInteger>("N", 10)),
                IsOkAndHolds(Each(31)));
    EXPECT_THAT(GenerateLots(MInteger(ExactlyIntegerExpression("3 * N + 1")),
                             Context().WithValue<MInteger>("N", 10)),
                IsOkAndHolds(Each(31)));
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
  EXPECT_THAT(Generate(MInteger(Exactly("3 * N + 1")),
                       Context().WithValue<MInteger>("N", 10)),
              IsOkAndHolds(31));
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
  EXPECT_THAT([&] { GetUniqueValue(MInteger(Between("N", "N"))); },
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
  // TODO: These should all throw, not die or status.
  EXPECT_DEATH(
      { Generate(MInteger(Exactly("N + "))).IgnoreError(); }, "operation");
  EXPECT_DEATH(
      { Generate(MInteger(AtMost("N + "))).IgnoreError(); }, "operation");
  EXPECT_DEATH(
      { Generate(MInteger(AtLeast("N + "))).IgnoreError(); }, "operation");
  EXPECT_DEATH(
      { Generate(MInteger(Between("& x", "N + "))).IgnoreError(); },
      "Unknown character");
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
    EXPECT_THAT(MInteger(Exactly(5), OneOf({6, 7, 8})),
                IsNotSatisfiedWith(5, "one of"));
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
