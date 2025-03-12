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
#include <vector>

#include "absl/status/status.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/contexts/librarian/analysis_context.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/size_property.h"
#include "src/librarian/test_utils.h"
#include "src/testing/status_test_util.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/constraints/numeric_constraints.h"
#include "src/variables/constraints/size_constraints.h"

namespace moriarty {
namespace {

using ::moriarty::IsOkAndHolds;
using ::moriarty::StatusIs;
using ::moriarty_testing::Context;
using ::moriarty_testing::Generate;
using ::moriarty_testing::GenerateDifficultInstancesValues;
using ::moriarty_testing::GeneratedValuesAre;
using ::moriarty_testing::GenerateSameValues;
using ::moriarty_testing::GetUniqueValue;
using ::moriarty_testing::IsNotSatisfiedWith;
using ::moriarty_testing::IsSatisfiedWith;
using ::moriarty_testing::Print;
using ::moriarty_testing::Read;
using ::moriarty_testing::ThrowsVariableNotFound;
using ::testing::AllOf;
using ::testing::Each;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::HasSubstr;
using ::testing::IsSupersetOf;
using ::testing::Le;
using ::testing::Optional;
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

TEST(MIntegerTest, BetweenShouldRestrictTheRangeProperly) {
  EXPECT_THAT(MInteger().Between(5, 5), GeneratedValuesAre(5));
  EXPECT_THAT(MInteger().Between(5, 10),
              GeneratedValuesAre(AllOf(Ge(5), Le(10))));
  EXPECT_THAT(MInteger().Between(-1, 1),
              GeneratedValuesAre(AllOf(Ge(-1), Le(1))));
}

TEST(MIntegerTest, RepeatedBetweenCallsShouldBeIntersectedTogether) {
  // All possible valid intersections
  EXPECT_TRUE(
      GenerateSameValues(MInteger().Between(0, 30).Between(1, 10),
                         MInteger().Between(1, 10)));  // First is a superset
  EXPECT_TRUE(
      GenerateSameValues(MInteger().Between(1, 10).Between(0, 30),
                         MInteger().Between(1, 10)));  // Second is a superset
  EXPECT_TRUE(
      GenerateSameValues(MInteger().Between(0, 10).Between(1, 30),
                         MInteger().Between(1, 10)));  // First on the left
  EXPECT_TRUE(
      GenerateSameValues(MInteger().Between(1, 30).Between(0, 10),
                         MInteger().Between(1, 10)));  // First on the right
  EXPECT_TRUE(GenerateSameValues(MInteger().Between(1, 8).Between(8, 10),
                                 MInteger().Between(8, 8)));  // Singleton Range

  // Several chained calls to Between should work
  EXPECT_TRUE(
      GenerateSameValues(MInteger().Between(1, 20).Between(3, 21).Between(2, 5),
                         MInteger().Between(3, 5)));
}

TEST(MIntegerTest, InvalidBoundsShouldCrash) {
  // Min > Max
  EXPECT_THROW(
      { Generate(MInteger().Between(0, -1)).IgnoreError(); },
      std::runtime_error);

  // Empty intersection (First interval to the left)
  EXPECT_THROW(
      { Generate(MInteger().Between(1, 10).Between(20, 30)).IgnoreError(); },
      std::runtime_error);

  // Empty intersection (First interval to the right)
  EXPECT_THROW(
      { Generate(MInteger().Between(20, 30).Between(1, 10)).IgnoreError(); },
      std::runtime_error);
}

// TODO(darcybest): MInteger should have an equality operator instead of this.
TEST(MIntegerTest, MergeFromCorrectlyMerges) {
  // All possible valid intersections
  EXPECT_TRUE(GenerateSameValues(
      MInteger().Between(0, 30).MergeFrom(MInteger().Between(1, 10)),
      MInteger().Between(0, 30).Between(1, 10)));  // First superset
  EXPECT_TRUE(GenerateSameValues(
      MInteger().Between(1, 10).MergeFrom(MInteger().Between(0, 30)),
      MInteger().Between(1, 10).Between(0, 30)));  // Second superset
  EXPECT_TRUE(GenerateSameValues(
      MInteger().Between(0, 10).MergeFrom(MInteger().Between(1, 30)),
      MInteger().Between(0, 10).Between(1, 30)));  // First on left
  EXPECT_TRUE(GenerateSameValues(
      MInteger().Between(1, 30).MergeFrom(MInteger().Between(0, 10)),
      MInteger().Between(1, 30).Between(0, 10)));  // First on  right
  EXPECT_TRUE(GenerateSameValues(
      MInteger().Between(1, 8).MergeFrom(MInteger().Between(8, 10)),
      MInteger().Between(1, 8).Between(8, 10)));  // Singleton Range
}

TEST(MIntegerTest, SatisfiesConstraintsWorksForValid) {
  EXPECT_THAT(MInteger().Between(1, 10), IsSatisfiedWith(5));   // Middle
  EXPECT_THAT(MInteger().Between(1, 10), IsSatisfiedWith(1));   // Low
  EXPECT_THAT(MInteger().Between(1, 10), IsSatisfiedWith(10));  // High

  // Whole range
  EXPECT_THAT(MInteger(), IsSatisfiedWith(0));
  EXPECT_THAT(MInteger(), IsSatisfiedWith(std::numeric_limits<int64_t>::min()));
  EXPECT_THAT(MInteger(), IsSatisfiedWith(std::numeric_limits<int64_t>::max()));
}

TEST(MIntegerTest, SatisfiesConstraintsWorksForInvalid) {
  EXPECT_THAT(MInteger().Between(1, 10), IsNotSatisfiedWith(0, "range"));
  EXPECT_THAT(MInteger().Between(1, 10), IsNotSatisfiedWith(11, "range"));

  // Empty range
  EXPECT_THAT(MInteger().Between(1, 0), IsNotSatisfiedWith(0, "range"));
}

TEST(MIntegerTest, SatisfiesConstraintsWorksForValidExpressions) {
  EXPECT_THAT(
      MInteger().Between(1, "3 * N + 1"),
      IsSatisfiedWith(5, Context().WithValue<MInteger>("N", 10)));  // Mid
  EXPECT_THAT(
      MInteger().Between(1, "3 * N + 1"),
      IsSatisfiedWith(1, Context().WithValue<MInteger>("N", 10)));  // Lo
  EXPECT_THAT(
      MInteger().Between(1, "3 * N + 1"),
      IsSatisfiedWith(31, Context().WithValue<MInteger>("N", 10)));  // High
}

TEST(MIntegerTest, SatisfiesConstraintsWorksForInvalidExpressions) {
  EXPECT_THAT(
      MInteger().Between(1, "3 * N + 1"),
      IsNotSatisfiedWith(0, "range", Context().WithValue<MInteger>("N", 10)));

  moriarty_internal::ValueSet values;
  moriarty_internal::VariableSet variables;
  librarian::AnalysisContext ctx("_", variables, values);
  EXPECT_THAT(
      [&] {
        MInteger()
            .Between(1, "3 * N + 1")
            .IsSatisfiedWith(ctx, 1)
            .IgnoreError();
      },
      ThrowsVariableNotFound("N"));
}

TEST(MIntegerTest, AtMostAndAtLeastShouldLimitTheOutputRange) {
  EXPECT_THAT(MInteger().AtMost(10).AtLeast(-5),
              GeneratedValuesAre(AllOf(Le(10), Ge(-5))));
}

TEST(MIntegerTest, AtMostLargerThanAtLeastShouldFail) {
  EXPECT_THROW(
      { Generate(MInteger().AtLeast(10).AtMost(0)).IgnoreError(); },
      std::runtime_error);

  EXPECT_THROW(
      {
        Generate(MInteger().AtLeast(0).AtMost("3 * N + 1"),
                 Context().WithValue<MInteger>("N", -3))
            .IgnoreError();
      },
      std::runtime_error);
}

TEST(MIntegerDeathTest,
     AtMostAtLeastBetweenWithUnparsableExpressionsShouldFail) {
  // We should throw a nice exception here, not ABSL die.
  EXPECT_DEATH({ MInteger().AtLeast("3 + "); }, "operation");
  EXPECT_DEATH({ MInteger().AtMost("+ X +"); }, "operation");
  EXPECT_DEATH({ MInteger().Between("N + 2", "* M + M"); }, "operation");
}

TEST(MIntegerTest, AtMostAndAtLeastWithExpressionsShouldLimitTheOutputRange) {
  // TODO(darcybest): Make GeneratedValuesAre compatible with Context instead of
  // using GenerateLots().
  EXPECT_THAT(GenerateLots(MInteger().AtLeast(0).AtMost("3 * N + 1"),
                           Context().WithValue<MInteger>("N", 10)),
              IsOkAndHolds(Each(AllOf(Ge(0), Le(31)))));
  EXPECT_THAT(GenerateLots(MInteger().AtLeast("3 * N + 1").AtMost(50),
                           Context().WithValue<MInteger>("N", 10)),
              IsOkAndHolds(Each(AllOf(Ge(31), Le(50)))));
}

TEST(
    MIntegerTest,
    MultipleExpressionsAndConstantsInAtLeastAtMostBetweenShouldRestrictOutput) {
  // TODO(darcybest): Make GeneratedValuesAre compatible with Context.
  EXPECT_THAT(
      GenerateLots(
          MInteger().AtLeast(0).AtMost("3 * N + 1").Between("N + M", 100),
          Context().WithValue<MInteger>("N", 10).WithValue<MInteger>("M", 15)),
      IsOkAndHolds(Each(AllOf(Ge(0), Le(31), Ge(25), Le(100)))));

  EXPECT_THAT(
      GenerateLots(
          MInteger()
              .AtLeast("3 * N + 1")
              .AtLeast("3 * M + 3")
              .AtMost(50)
              .AtMost("M ^ 2"),
          Context().WithValue<MInteger>("N", 6).WithValue<MInteger>("M", 5)),
      IsOkAndHolds(Each(AllOf(Ge(19), Ge(18), Le(50), Le(25)))));
}

TEST(MIntegerTest, AllOverloadsOfBetweenAreEffective) {
  EXPECT_THAT(MInteger().Between(1, 10),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
  EXPECT_THAT(MInteger().Between(1, "10"),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
  EXPECT_THAT(MInteger().Between("1", 10),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
  EXPECT_THAT(MInteger().Between("1", "10"),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
}

TEST(MIntegerTest, IsMIntegerExpressionShouldRestrictInput) {
  EXPECT_THAT(Generate(MInteger().Is("3 * N + 1"),
                       Context().WithValue<MInteger>("N", 10)),
              IsOkAndHolds(31));
}

TEST(MIntegerTest, GetUniqueValueWorksWhenUniqueValueKnown) {
  EXPECT_THAT(GetUniqueValue(MInteger().Between("N", "N"),
                             Context().WithValue<MInteger>("N", 10)),
              Optional(10));

  EXPECT_THAT(GetUniqueValue(MInteger().Between(20, "2 * N"),
                             Context().WithValue<MInteger>("N", 10)),
              Optional(20));
}

TEST(MIntegerTest, GetUniqueValueWithNestedDependenciesShouldWork) {
  EXPECT_THAT(GetUniqueValue(
                  MInteger().Between("X", "Y"),
                  Context()
                      .WithVariable<MInteger>("X", MInteger().Is(5))
                      .WithVariable<MInteger>("Y", MInteger().Between(5, "N"))
                      .WithValue<MInteger>("N", 5)),
              Optional(5));
}

TEST(MIntegerTest, GetUniqueValueFailsWhenAVariableIsUnknown) {
  EXPECT_THAT([&] { GetUniqueValue(MInteger().Between("N", "N")); },
              ThrowsVariableNotFound("N"));
}

TEST(MIntegerTest, GetUniqueValueShouldSucceedIfTheValueIsUnique) {
  EXPECT_THAT(GetUniqueValue(MInteger().Between(123, 123)), Optional(123));
  EXPECT_THAT(GetUniqueValue(MInteger().Is(456)), Optional(456));
}

TEST(MIntegerTest, GetUniqueValueFailsWhenTheValueIsNotUnique) {
  EXPECT_EQ(GetUniqueValue(MInteger().Between(8, 10)), std::nullopt);
}

TEST(MIntegerTest, OfSizePropertyOnlyAcceptsSizeAsCategory) {
  EXPECT_THAT(
      MInteger().OfSizeProperty({.category = "wrong"}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("category")));
}

TEST(MIntegerTest, OfSizePropertyOnlyAcceptsKnownSizes) {
  EXPECT_THAT(
      MInteger().OfSizeProperty(
          {.category = "size", .descriptor = "unknown_type"}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("Unknown size")));
}

TEST(MIntegerTest, GetDifficultInstancesIncludesManyInterestingValues) {
  EXPECT_THAT(GenerateDifficultInstancesValues(MInteger()),
              IsOkAndHolds(IsSupersetOf(
                  {0LL, 1LL, 2LL, -1LL, 1LL << 32, (1LL << 62) - 1})));
}

TEST(MIntegerTest, GetDifficultInstancesIncludesIntMinAndMaxByDefault) {
  EXPECT_THAT(
      GenerateDifficultInstancesValues(MInteger()),
      IsOkAndHolds(IsSupersetOf({std::numeric_limits<int64_t>::min(),
                                 std::numeric_limits<int64_t>::max()})));
}

TEST(MIntegerTest, GetDifficultInstancesIncludesMinAndMaxValues) {
  EXPECT_THAT(GenerateDifficultInstancesValues(MInteger().Between(123, 234)),
              IsOkAndHolds(IsSupersetOf({123, 234})));
}

TEST(MIntegerTest, GetDifficultInstancesValuesAreNotRepeated) {
  EXPECT_THAT(GenerateDifficultInstancesValues(MInteger().Between(-1, 1)),
              IsOkAndHolds(UnorderedElementsAre(-1, 0, 1)));
}

TEST(MIntegerTest,
     GetDifficultInstancesForFixedNonDifficultValueFailsGeneration) {
  librarian::AnalysisContext ctx("test", {}, {});
  MORIARTY_ASSERT_OK_AND_ASSIGN(std::vector<MInteger> instances,
                                MInteger().Is(1234).GetDifficultInstances(ctx));

  for (MInteger instance : instances) {
    EXPECT_THROW({ Generate(instance).IgnoreError(); }, std::runtime_error);
  }
}

TEST(MIntegerTest, WithSizeGivesAppropriatelySizedValues) {
  // These values here are fuzzy and may need to be changed over time. "small"
  // might be changed over time. The bounds here are mostly just to check the
  // approximate sizes are considered.

  EXPECT_THAT(MInteger().Between(1, "10^9").WithSize(CommonSize::kMin),
              GeneratedValuesAre(Eq(1)));
  EXPECT_THAT(MInteger().Between(1, "10^9").WithSize(CommonSize::kTiny),
              GeneratedValuesAre(Le(30)));
  EXPECT_THAT(MInteger().Between(1, "10^9").WithSize(CommonSize::kSmall),
              GeneratedValuesAre(Le(2000)));
  EXPECT_THAT(MInteger().Between(1, "10^9").WithSize(CommonSize::kMedium),
              GeneratedValuesAre(Le(1000000)));
  EXPECT_THAT(MInteger().Between(1, "10^9").WithSize(CommonSize::kLarge),
              GeneratedValuesAre(Ge(1000000)));
  EXPECT_THAT(MInteger().Between(1, "10^9").WithSize(CommonSize::kHuge),
              GeneratedValuesAre(Ge(500000000)));
  EXPECT_THAT(MInteger().Between(1, "10^9").WithSize(CommonSize::kMax),
              GeneratedValuesAre(Eq(1000000000)));
}

TEST(MIntegerTest, WithSizeBehavesWithMergeFrom) {
  MInteger small = MInteger().Between(1, "10^9").WithSize(CommonSize::kSmall);
  MInteger tiny = MInteger().Between(1, "10^9").WithSize(CommonSize::kTiny);
  MInteger large = MInteger().Between(1, "10^9").WithSize(CommonSize::kLarge);
  MInteger any = MInteger().Between(1, "10^9").WithSize(CommonSize::kAny);

  {
    EXPECT_FALSE(GenerateSameValues(small, tiny));
    MORIARTY_EXPECT_OK(small.TryMergeFrom(tiny));
    EXPECT_TRUE(GenerateSameValues(small, tiny));
  }
  {
    EXPECT_FALSE(GenerateSameValues(any, large));
    MORIARTY_EXPECT_OK(any.TryMergeFrom(large));
    EXPECT_TRUE(GenerateSameValues(any, large));
  }

  EXPECT_THAT(tiny.TryMergeFrom(large),
              StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("size")));
}

TEST(MIntegerTest, ToStringWorks) {
  EXPECT_THAT(MInteger().ToString(), HasSubstr("MInteger"));
  EXPECT_THAT(MInteger().Between(1, 10).ToString(), HasSubstr("[1, 10]"));
  EXPECT_THAT(MInteger().WithSize(CommonSize::kSmall).ToString(),
              HasSubstr("mall"));  // [S|s]mall
  EXPECT_THAT(MInteger().Between(1, 5).WithSize(CommonSize::kSmall).ToString(),
              AllOf(HasSubstr("[1, 5]"), HasSubstr("mall")));  // [S|s]mall
}

// -----------------------------------------------------------------------------
//  Tests for the non-builder version of MInteger's API
//  These are copies of the above functions, but with the builder version of
//  MInteger replaced with the non-builder version.
// -----------------------------------------------------------------------------

TEST(MIntegerNonBuilderTest, BetweenShouldRestrictTheRangeProperly) {
  EXPECT_THAT(MInteger(Between(5, 5)), GeneratedValuesAre(5));
  EXPECT_THAT(MInteger(Between(5, 10)),
              GeneratedValuesAre(AllOf(Ge(5), Le(10))));
  EXPECT_THAT(MInteger(Between(-1, 1)),
              GeneratedValuesAre(AllOf(Ge(-1), Le(1))));
}

TEST(MIntegerNonBuilderTest, RepeatedBetweenCallsShouldBeIntersectedTogether) {
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

TEST(MIntegerNonBuilderTest, InvalidBoundsShouldCrash) {
  // Min > Max
  EXPECT_THROW(
      { Generate(MInteger(Between(0, -1))).IgnoreError(); },
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
TEST(MIntegerNonBuilderTest, MergeFromCorrectlyMerges) {
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

TEST(MIntegerNonBuilderTest, SatisfiesConstraintsWorksForValid) {
  EXPECT_THAT(MInteger(Between(1, 10)), IsSatisfiedWith(5));   // Middle
  EXPECT_THAT(MInteger(Between(1, 10)), IsSatisfiedWith(1));   // Low
  EXPECT_THAT(MInteger(Between(1, 10)), IsSatisfiedWith(10));  // High

  // Whole range
  EXPECT_THAT(MInteger(), IsSatisfiedWith(0));
  EXPECT_THAT(MInteger(), IsSatisfiedWith(std::numeric_limits<int64_t>::min()));
  EXPECT_THAT(MInteger(), IsSatisfiedWith(std::numeric_limits<int64_t>::max()));
}

TEST(MIntegerNonBuilderTest, SatisfiesConstraintsWorksForInvalid) {
  EXPECT_THAT(MInteger(Between(1, 10)), IsNotSatisfiedWith(0, "range"));
  EXPECT_THAT(MInteger(Between(1, 10)), IsNotSatisfiedWith(11, "range"));

  // Empty range
  EXPECT_THAT(MInteger(Between(1, 0)), IsNotSatisfiedWith(0, "range"));
}

TEST(MIntegerNonBuilderTest, SatisfiesConstraintsWorksForValidExpressions) {
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

TEST(MIntegerNonBuilderTest, SatisfiesConstraintsWorksForInvalidExpressions) {
  EXPECT_THAT(
      MInteger(Between(1, "3 * N + 1")),
      IsNotSatisfiedWith(0, "range", Context().WithValue<MInteger>("N", 10)));

  moriarty_internal::ValueSet values;
  moriarty_internal::VariableSet variables;
  librarian::AnalysisContext ctx("_", variables, values);
  EXPECT_THAT(
      [&] {
        MInteger(Between(1, "3 * N + 1")).IsSatisfiedWith(ctx, 2).IgnoreError();
      },
      ThrowsVariableNotFound("N"));
}

TEST(MIntegerNonBuilderTest, AtMostAndAtLeastShouldLimitTheOutputRange) {
  EXPECT_THAT(MInteger(AtMost(10), AtLeast(-5)),
              GeneratedValuesAre(AllOf(Le(10), Ge(-5))));
}

TEST(MIntegerNonBuilderTest, AtMostLargerThanAtLeastShouldFail) {
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

TEST(MIntegerNonBuilderTest,
     AtMostAndAtLeastWithExpressionsShouldLimitTheOutputRange) {
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
    MIntegerNonBuilderTest,
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

TEST(MIntegerNonBuilderTest, AllOverloadsOfBetweenAreEffective) {
  EXPECT_THAT(MInteger(Between(1, 10)),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
  EXPECT_THAT(MInteger(Between(1, "10")),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
  EXPECT_THAT(MInteger(Between("1", 10)),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
  EXPECT_THAT(MInteger(Between("1", "10")),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
}

TEST(MIntegerNonBuilderTest, IsMIntegerExpressionShouldRestrictInput) {
  EXPECT_THAT(Generate(MInteger(Exactly("3 * N + 1")),
                       Context().WithValue<MInteger>("N", 10)),
              IsOkAndHolds(31));
}

TEST(MIntegerNonBuilderTest, GetUniqueValueWorksWhenUniqueValueKnown) {
  EXPECT_THAT(GetUniqueValue(MInteger(Between("N", "N")),
                             Context().WithValue<MInteger>("N", 10)),
              Optional(10));

  EXPECT_THAT(GetUniqueValue(MInteger(Between(20, "2 * N")),
                             Context().WithValue<MInteger>("N", 10)),
              Optional(20));
}

TEST(MIntegerNonBuilderTest, GetUniqueValueWithNestedDependenciesShouldWork) {
  EXPECT_THAT(
      GetUniqueValue(MInteger(Between("X", "Y")),
                     Context()
                         .WithVariable<MInteger>("X", MInteger(Exactly(5)))
                         .WithVariable<MInteger>("Y", MInteger(Between(5, "N")))
                         .WithValue<MInteger>("N", 5)),
      Optional(5));
}

TEST(MIntegerNonBuilderTest, GetUniqueValueFailsWhenAVariableIsUnknown) {
  EXPECT_THAT([&] { GetUniqueValue(MInteger(Between("N", "N"))); },
              ThrowsVariableNotFound("N"));
}

TEST(MIntegerNonBuilderTest, GetUniqueValueShouldSucceedIfTheValueIsUnique) {
  EXPECT_THAT(GetUniqueValue(MInteger(Between(123, 123))), Optional(123));
  EXPECT_THAT(GetUniqueValue(MInteger(Exactly(456))), Optional(456));
}

TEST(MIntegerNonBuilderTest, GetUniqueValueFailsWhenTheValueIsNotUnique) {
  EXPECT_EQ(GetUniqueValue(MInteger(Between(8, 10))), std::nullopt);
}

TEST(MIntegerNonBuilderTest, OfSizePropertyOnlyAcceptsSizeAsCategory) {
  EXPECT_THAT(
      MInteger().OfSizeProperty({.category = "wrong"}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("category")));
}

TEST(MIntegerNonBuilderTest, OfSizePropertyOnlyAcceptsKnownSizes) {
  EXPECT_THAT(
      MInteger().OfSizeProperty(
          {.category = "size", .descriptor = "unknown_type"}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("Unknown size")));
}

TEST(MIntegerNonBuilderTest,
     GetDifficultInstancesIncludesManyInterestingValues) {
  EXPECT_THAT(GenerateDifficultInstancesValues(MInteger()),
              IsOkAndHolds(IsSupersetOf(
                  {0LL, 1LL, 2LL, -1LL, 1LL << 32, (1LL << 62) - 1})));
}

TEST(MIntegerNonBuilderTest,
     GetDifficultInstancesIncludesIntMinAndMaxByDefault) {
  EXPECT_THAT(
      GenerateDifficultInstancesValues(MInteger()),
      IsOkAndHolds(IsSupersetOf({std::numeric_limits<int64_t>::min(),
                                 std::numeric_limits<int64_t>::max()})));
}

TEST(MIntegerNonBuilderTest, GetDifficultInstancesIncludesMinAndMaxValues) {
  EXPECT_THAT(GenerateDifficultInstancesValues(MInteger(Between(123, 234))),
              IsOkAndHolds(IsSupersetOf({123, 234})));
}

TEST(MIntegerNonBuilderTest, GetDifficultInstancesValuesAreNotRepeated) {
  EXPECT_THAT(GenerateDifficultInstancesValues(MInteger(Between(-1, 1))),
              IsOkAndHolds(UnorderedElementsAre(-1, 0, 1)));
}

TEST(MIntegerNonBuilderTest, WithSizeGivesAppropriatelySizedValues) {
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

TEST(MIntegerNonBuilderTest, WithSizeBehavesWithMergeFrom) {
  MInteger small = MInteger(Between(1, "10^9"), SizeCategory::Small());
  MInteger tiny = MInteger(Between(1, "10^9"), SizeCategory::Tiny());
  MInteger large = MInteger(Between(1, "10^9"), SizeCategory::Large());
  MInteger any = MInteger(Between(1, "10^9"), SizeCategory::Any());

  {
    EXPECT_FALSE(GenerateSameValues(small, tiny));
    MORIARTY_EXPECT_OK(small.TryMergeFrom(tiny));
    EXPECT_TRUE(GenerateSameValues(small, tiny));
  }
  {
    EXPECT_FALSE(GenerateSameValues(any, large));
    MORIARTY_EXPECT_OK(any.TryMergeFrom(large));
    EXPECT_TRUE(GenerateSameValues(any, large));
  }

  EXPECT_THAT(tiny.TryMergeFrom(large),
              StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("size")));
}

TEST(MIntegerNonBuilderTest, ToStringWorks) {
  EXPECT_THAT(MInteger().ToString(), HasSubstr("MInteger"));
  EXPECT_THAT(MInteger(Between(1, 10)).ToString(), HasSubstr("[1, 10]"));
  EXPECT_THAT(MInteger(SizeCategory::Small()).ToString(),
              HasSubstr("mall"));  // [S|s]mall
  EXPECT_THAT(MInteger(Between(1, 5), SizeCategory::Small()).ToString(),
              AllOf(HasSubstr("[1, 5]"), HasSubstr("mall")));  // [S|s]mall
}

TEST(MIntegerNonBuilderTest, InvalidSizeCombinationsFailGeneration) {
  EXPECT_THAT(
      Generate(MInteger(SizeCategory::Small(), SizeCategory::Large())),
      StatusIs(absl::StatusCode::kFailedPrecondition, HasSubstr("size")));

  EXPECT_THAT(MInteger(SizeCategory::Small(), SizeCategory::Large()),
              IsNotSatisfiedWith(123, "size"));

  EXPECT_THAT(
      MInteger(SizeCategory::Small(), SizeCategory::Large())
          .TryMergeFrom(MInteger(SizeCategory::Tiny())),
      StatusIs(absl::StatusCode::kFailedPrecondition, HasSubstr("size")));

  EXPECT_THAT(
      Print(MInteger(SizeCategory::Small(), SizeCategory::Large()), -1),
      StatusIs(absl::StatusCode::kFailedPrecondition, HasSubstr("size")));

  EXPECT_THAT(
      Read(MInteger(SizeCategory::Small(), SizeCategory::Large()), "-1"),
      StatusIs(absl::StatusCode::kFailedPrecondition, HasSubstr("size")));

  EXPECT_THAT(MInteger(SizeCategory::Small(), SizeCategory::Large()).ToString(),
              HasSubstr("Invalid size"));
}

TEST(MIntegerNonBuilderTest, InvalidExpressionsShouldFail) {
  // TODO: These should all throw, not die or status.
  EXPECT_THAT(Generate(MInteger(Exactly("N + "))),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("invalid expression")));

  EXPECT_DEATH(
      { Generate(MInteger(AtMost("N + "))).IgnoreError(); }, "operation");
  EXPECT_DEATH(
      { Generate(MInteger(AtLeast("N + "))).IgnoreError(); }, "operation");
  EXPECT_DEATH(
      { Generate(MInteger(Between("& x", "N + "))).IgnoreError(); },
      "Unknown character");
}

}  // namespace
}  // namespace moriarty
