// Copyright 2025 Darcy Best
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

#include "src/variables/mreal.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/constraints/base_constraints.h"
#include "src/constraints/numeric_constraints.h"
#include "src/contexts/librarian_context.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/errors.h"
#include "src/librarian/testing/gtest_helpers.h"
#include "src/types/real.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace {

using ::moriarty_testing::Context;
using ::moriarty_testing::Generate;
using ::moriarty_testing::GeneratedValuesAre;
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
using ::testing::Ge;
using ::testing::HasSubstr;
using ::testing::Le;
using ::testing::Optional;
using ::testing::ThrowsMessage;

TEST(MRealTest, TypenameIsCorrect) { EXPECT_EQ(MReal().Typename(), "MReal"); }

TEST(MRealTest, PrintShouldSucceed) {
  EXPECT_EQ(Print(MReal(), -1), "-1.000000");
  EXPECT_EQ(Print(MReal(), 0), "0.000000");
  EXPECT_EQ(Print(MReal(), 1), "1.000000");
  EXPECT_EQ(Print(MReal(), 1.23456789), "1.234568");
  EXPECT_EQ(Print(MReal(), -11.23456789), "-11.234568");

  // Check that the precision is correct.
  EXPECT_EQ(Print(MReal().SetIODigits(1), 1.23456789), "1.2");
  EXPECT_EQ(Print(MReal().SetIODigits(2), -1.23456789), "-1.23");
  EXPECT_EQ(Print(MReal().SetIODigits(3), 21.23456789), "21.235");
  EXPECT_EQ(Print(MReal().SetIODigits(4), -1.23456789), "-1.2346");
  EXPECT_EQ(Print(MReal().SetIODigits(5), -1.23456789), "-1.23457");
  EXPECT_EQ(Print(MReal().SetIODigits(6), 21.23456789), "21.234568");
  EXPECT_EQ(Print(MReal().SetIODigits(7), -1.23456789), "-1.2345679");
  EXPECT_EQ(Print(MReal().SetIODigits(8), 1.23456789), "1.23456789");
  EXPECT_EQ(Print(MReal().SetIODigits(9), -1.23456789), "-1.234567890");
  EXPECT_EQ(Print(MReal().SetIODigits(10), 1.23456789), "1.2345678900");
  EXPECT_EQ(Print(MReal().SetIODigits(11), -1.23456789), "-1.23456789000");

  EXPECT_THROW((void)Print(MReal().SetIODigits(0), 1.23456789),
               std::invalid_argument);
  EXPECT_THROW((void)Print(MReal().SetIODigits(21), 1.23456789),
               std::invalid_argument);
  EXPECT_THROW((void)Print(MReal().SetIODigits(-2), 1.23456789),
               std::invalid_argument);
}

TEST(MRealTest, ValidReadInStrictModeShouldSucceed) {
  {  // Strict mode
    EXPECT_EQ(Read(MReal().SetIODigits(6), "123.456789"), 123.456789);
    EXPECT_EQ(Read(MReal().SetIODigits(3), "-789.789"), -789.789);
  }
  {  // Flexible mode
    Context c = Context().WithFlexibleMode();
    EXPECT_EQ(Read(MReal(), "456 ", c), 456);
    EXPECT_EQ(Read(MReal(), "-7.89e12", c), -7890000000000.0);
    EXPECT_EQ(Read(MReal(), "-7.89e-4", c), -0.000789);
  }
}

TEST(MRealTest, ReadWithTokensAfterwardsIsFine) {
  EXPECT_EQ(Read(MReal().SetIODigits(3), "-123.456 you should ignore this"),
            -123.456);
  EXPECT_EQ(Read(MReal(), "-123 you should ignore this",
                 Context().WithFlexibleMode()),
            -123);
}

TEST(MRealTest, InvalidReadShouldFail) {
  // EOF
  EXPECT_THROW({ (void)Read(MReal(), ""); }, IOError);
  // EOF with whitespace
  EXPECT_THROW({ (void)Read(MReal(), " "); }, IOError);
  // Double negative
  EXPECT_THROW({ (void)Read(MReal(), "--123"); }, IOError);
  // Invalid character start/middle/end
  EXPECT_THROW({ (void)Read(MReal(), "c123"); }, IOError);
  EXPECT_THROW({ (void)Read(MReal(), "12c3"); }, IOError);
  EXPECT_THROW({ (void)Read(MReal(), "123c"); }, IOError);
  // Multiple decimals
  EXPECT_THROW({ (void)Read(MReal(), "123.45.67"); }, IOError);
  EXPECT_THROW(
      { (void)Read(MReal(), "123.45e0.7", Context().WithFlexibleMode()); },
      IOError);
}

TEST(MRealTest, BetweenShouldRestrictTheRangeProperly) {
  EXPECT_THAT(MReal(Between(5, 5)), GeneratedValuesAre(5));
  EXPECT_THAT(MReal(Between(5, 10)), GeneratedValuesAre(AllOf(Ge(5), Le(10))));
  EXPECT_THAT(MReal(Between(-1, 1)), GeneratedValuesAre(AllOf(Ge(-1), Le(1))));
  EXPECT_THAT(MReal(Between(Real("-0.5"), Real("10.2"))),
              GeneratedValuesAre(AllOf(Ge(-0.5), Le(10.2))));
}

TEST(MRealTest, RepeatedBetweenCallsShouldBeIntersectedTogether) {
  // All possible valid intersections
  EXPECT_TRUE(GenerateSameValues(
      MReal(Between(0, 30), Between(1, Real("10.4"))),
      MReal(Between(1, Real("10.4")))));  // First is a superset
  EXPECT_TRUE(GenerateSameValues(
      MReal(Between(1, Real("10.4")), Between(0, 30)),
      MReal(Between(1, Real("10.4")))));  // Second is a superset
  EXPECT_TRUE(GenerateSameValues(
      MReal(Between(0, Real("10.4")), Between(1, 30)),
      MReal(Between(1, Real("10.4")))));  // First on the left
  EXPECT_TRUE(GenerateSameValues(
      MReal(Between(1, 30), Between(0, Real("10.4"))),
      MReal(Between(1, Real("10.4")))));  // First on the right
  EXPECT_TRUE(GenerateSameValues(MReal(Between(1, 8), Between(8, Real("10.4"))),
                                 MReal(Between(8, 8))));  // Singleton Range

  // Several chained calls to Between should work
  EXPECT_TRUE(GenerateSameValues(
      MReal(Between(1, Real("20.2")), Between(Real("3.14"), 21), Between(2, 5)),
      MReal(Between(Real("3.14"), 5))));
}

TEST(MRealTest, InvalidBoundsShouldCrash) {
  // Need to use AtMost/AtLeast here since Between will crash on its own.
  // Min > Max
  EXPECT_THAT(MReal(AtLeast(0), AtMost(-1)),
              GenerateThrowsGenerationError("", Context()));

  // Empty intersection (First interval to the left)
  EXPECT_THAT(MReal(Between(1, 10), Between(20, 30)),
              GenerateThrowsGenerationError("", Context()));

  // Empty intersection (First interval to the right)
  EXPECT_THAT(MReal(Between(20, 30), Between(1, 10)),
              GenerateThrowsGenerationError("", Context()));
}

// TODO(darcybest): MInteger should have an equality operator instead of this.
TEST(MRealTest, MergeFromCorrectlyMerges) {
  // All possible valid intersections
  EXPECT_TRUE(GenerateSameValues(
      MReal(Between(0, 30)).MergeFrom(MReal(Between(1, Real("10.4")))),
      MReal(Between(0, 30), Between(1, Real("10.4")))));  // First superset
  EXPECT_TRUE(GenerateSameValues(
      MReal(Between(1, Real("10.4"))).MergeFrom(MReal(Between(0, 30))),
      MReal(Between(1, Real("10.4")), Between(0, 30))));  // Second superset
  EXPECT_TRUE(GenerateSameValues(
      MReal(Between(0, Real("10.4"))).MergeFrom(MReal(Between(1, 30))),
      MReal(Between(0, Real("10.4")), Between(1, 30))));  // First on left
  EXPECT_TRUE(GenerateSameValues(
      MReal(Between(1, 30)).MergeFrom(MReal(Between(0, Real("10.4")))),
      MReal(Between(1, 30), Between(0, Real("10.4")))));  // First on  right
  EXPECT_TRUE(GenerateSameValues(
      MReal(Between(1, 8)).MergeFrom(MReal(Between(8, Real("10.4")))),
      MReal(Between(1, 8), Between(8, Real("10.4")))));  // Singleton Range
}

TEST(MRealTest, IsSatisfiedWithWorksForGoodData) {
  EXPECT_THAT(MReal(Between(1, Real("10.4"))), IsSatisfiedWith(5));  // Middle
  EXPECT_THAT(MReal(Between(1, Real("10.4"))), IsSatisfiedWith(1));  // Low
  EXPECT_THAT(MReal(Between(1, Real("10.4"))),
              IsSatisfiedWith(std::nexttoward(10.4, 10)));  // High

  // Whole range
  EXPECT_THAT(MReal(), IsSatisfiedWith(0));
  EXPECT_THAT(MReal(), IsSatisfiedWith(std::numeric_limits<int64_t>::min()));
  EXPECT_THAT(MReal(), IsSatisfiedWith(std::numeric_limits<int64_t>::max()));
}

TEST(MRealTest, IsSatisfiedWithWorksForBadData) {
  EXPECT_THAT(MReal(Between(1, 10)), IsNotSatisfiedWith(0, "between"));
  EXPECT_THAT(MReal(Between(1, 10)), IsNotSatisfiedWith(11, "between"));

  // Empty range
  EXPECT_THAT(MReal(AtLeast(1), AtMost(-1)),
              AnyOf(IsNotSatisfiedWith(0, "at least"),
                    IsNotSatisfiedWith(0, "at most")));
}

TEST(MRealTest, IsSatisfiedWithWithExpressionsShouldWorkForGoodData) {
  EXPECT_THAT(
      MReal(Between(Real("1.1"), "3 * N + 1")),
      IsSatisfiedWith(5, Context().WithValue<MInteger>("N", 10)));  // Mid
  EXPECT_THAT(
      MReal(Between(Real("1.1"), "3 * N + 1")),
      IsSatisfiedWith(1.1, Context().WithValue<MInteger>("N", 10)));  // Lo
  EXPECT_THAT(
      MReal(Between(Real("1.1"), "3 * N + 1")),
      IsSatisfiedWith(31, Context().WithValue<MInteger>("N", 10)));  // High
}

TEST(MRealTest, IsSatisfiedWithWithExpressionsShouldWorkForBadData) {
  EXPECT_THAT(
      MReal(Between(Real("1.1"), "3 * N + 1")),
      IsNotSatisfiedWith(1, "between", Context().WithValue<MInteger>("N", 10)));

  EXPECT_THAT(MReal(AtLeast(Real("1.1")), AtMost(Real("-1.1"))),
              AnyOf(IsNotSatisfiedWith(0, "at least"),
                    IsNotSatisfiedWith(0, "at most")));

  moriarty_internal::ValueSet values;
  moriarty_internal::VariableSet variables;
  librarian::AnalysisContext ctx("_", variables, values);
  // Could be VariableNotFound as well (impl detail)
  EXPECT_THAT([&] { (void)MReal(Between(1, "3 * N + 1")).CheckValue(ctx, 2); },
              ThrowsVariableNotFound("N"));
}

TEST(MRealTest, AtMostAndAtLeastShouldLimitTheOutputRange) {
  EXPECT_THAT(MReal(AtMost(Real("10.4")), AtLeast(Real("-5.2"))),
              GeneratedValuesAre(AllOf(Le(10.4), Ge(-5.2))));
}

TEST(MRealTest, AtMostLargerThanAtLeastShouldFail) {
  EXPECT_THAT(MReal(AtLeast(Real("10.4")), AtMost(0)),
              GenerateThrowsGenerationError("", Context()));
  EXPECT_THAT(MReal(AtLeast(Real("10.4")), AtMost("3 * N + 1")),
              GenerateThrowsGenerationError(
                  "", Context().WithValue<MInteger>("N", -3)));
}

TEST(MRealTest, AtMostAtLeastBetweenWithUnparsableExpressionsShouldFail) {
  EXPECT_THAT([] { MReal(AtLeast("3 + ")); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("operation")));
  EXPECT_THAT([] { MReal(AtMost("+ X +")); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("operation")));
  EXPECT_THAT([] { MReal(Between("N + 2", "* M + M")); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("operation")));
}

TEST(MRealTest, AtMostAndAtLeastWithExpressionsShouldLimitTheOutputRange) {
  EXPECT_THAT(MReal(AtMost("3 * N + 1"), AtLeast(Real("0.0"))),
              GeneratedValuesAre(AllOf(Ge(0), Le(31)),
                                 Context().WithValue<MInteger>("N", 10)));
  EXPECT_THAT(MReal(AtLeast("3 * N + 1"), AtMost(Real("50.2"))),
              GeneratedValuesAre(AllOf(Ge(31), Le(50)),
                                 Context().WithValue<MInteger>("N", 10)));
}

TEST(
    MRealTest,
    MultipleExpressionsAndConstantsInAtLeastAtMostBetweenShouldRestrictOutput) {
  EXPECT_THAT(
      MReal(AtLeast(Real("0.5")), AtMost("3 * N + 1"), Between("N + M", 100)),
      GeneratedValuesAre(
          AllOf(Ge(0.5), Le(31), Ge(25), Le(100)),
          Context().WithValue<MInteger>("N", 10).WithValue<MInteger>("M", 15)));

  EXPECT_THAT(
      MReal(AtLeast("3 * N + 1"), AtLeast("3 * M + 3"), AtMost(Real("50.2")),
            AtMost("M ^ 2")),
      GeneratedValuesAre(
          AllOf(Ge(19), Ge(18), Le(50), Le(25)),
          Context().WithValue<MInteger>("N", 6).WithValue<MInteger>("M", 5)));
}

TEST(MRealTest, AllOverloadsOfExactlyAreEffective) {
  {  // No variables
    EXPECT_THAT(MReal(Exactly(10)), GeneratedValuesAre(10));
    EXPECT_THAT(MReal(Exactly("10")), GeneratedValuesAre(10));
    EXPECT_THAT(MReal(Exactly(Real("10"))), GeneratedValuesAre(10));
  }
  {  // With variables
    EXPECT_THAT(MReal(Exactly("3 * N + 1")),
                GeneratedValuesAre(31, Context().WithValue<MInteger>("N", 10)));
  }
}

TEST(MRealTest, AllOverloadsOfBetweenAreEffective) {
  EXPECT_THAT(MReal(Between(1, 10)), GeneratedValuesAre(AllOf(Ge(1), Le(10))));
  EXPECT_THAT(MReal(Between(1, "10")),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
  EXPECT_THAT(MReal(Between(1, Real("10.4"))),
              GeneratedValuesAre(AllOf(Ge(1), Le(10.4))));
  EXPECT_THAT(MReal(Between("1", 10)),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
  EXPECT_THAT(MReal(Between("1", "10")),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
  EXPECT_THAT(MReal(Between(Real("1.8"), "10")),
              GeneratedValuesAre(AllOf(Ge(1.8), Le(10))));
}

TEST(MRealTest, IntegerExpressionShouldRestrictInput) {
  EXPECT_THAT(MReal(Exactly("3 * N + 1")),
              GeneratedValuesAre(31, Context().WithValue<MInteger>("N", 10)));
}

TEST(MRealTest, GetUniqueValueWorksWhenUniqueValueKnown) {
  EXPECT_THAT(GetUniqueValue(MReal(Between("N", "N")),
                             Context().WithValue<MInteger>("N", 10)),
              Optional(10));

  EXPECT_THAT(GetUniqueValue(MReal(Between(20, "2 * N")),
                             Context().WithValue<MInteger>("N", 10)),
              Optional(20));
}

TEST(MRealTest, GetUniqueValueWithNestedDependenciesShouldWork) {
  EXPECT_THAT(
      GetUniqueValue(MReal(Between("X", "Y")),
                     Context()
                         .WithVariable<MInteger>("X", MInteger(Exactly(5)))
                         .WithVariable<MInteger>("Y", MInteger(Between(5, "N")))
                         .WithValue<MInteger>("N", 5)),
      Optional(5));
}

TEST(MRealTest, GetUniqueValueFailsWhenAVariableIsUnknown) {
  EXPECT_THAT([&] { (void)GetUniqueValue(MReal(Between("N", "N"))); },
              ThrowsVariableNotFound("N"));
}

TEST(MRealTest, GetUniqueValueShouldSucceedIfTheValueIsUnique) {
  EXPECT_THAT(GetUniqueValue(MReal(Between(123, Real("123")))), Optional(123));
  EXPECT_THAT(GetUniqueValue(MReal(Between(Real("123.45"), Real("123.45")))),
              Optional(123.45));
  EXPECT_THAT(GetUniqueValue(MReal(Exactly(Real("4.56")))), Optional(4.56));
}

TEST(MRealTest, GetUniqueValueFailsWhenTheValueIsNotUnique) {
  EXPECT_EQ(GetUniqueValue(MReal(Between(8, Real("8.2")))), std::nullopt);
}

TEST(MRealTest, InvalidExpressionsShouldFail) {
  EXPECT_THAT([] { (void)Generate(MReal(Exactly("N + "))); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("operation")));
  EXPECT_THAT([] { (void)Generate(MReal(AtMost("N + "))); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("operation")));
  EXPECT_THAT([] { (void)Generate(MReal(AtLeast("N + "))); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("operation")));
  EXPECT_THAT(
      [] { (void)Generate(MReal(Between("& x", "N + "))); },
      ThrowsMessage<std::invalid_argument>(HasSubstr("Unknown character")));
}

TEST(MRealTest, ExactlyAndOneOfConstraintsWithNoVariablesShouldWork) {
  {  // IsSatisfiedWith
    EXPECT_THAT(MReal(Exactly(Real(10, 2))), IsSatisfiedWith(5));
    EXPECT_THAT(MReal(Exactly(5)), IsNotSatisfiedWith(6, "exactly"));

    EXPECT_THAT(
        MReal(OneOf({Real("5"), Real("6.4"), Real("7.3")})),
        AllOf(IsSatisfiedWith(5), IsSatisfiedWith(6.4), IsSatisfiedWith(7.3)));
    EXPECT_THAT(MReal(OneOf({Real("5"), Real("6.4"), Real("7.3")})),
                IsNotSatisfiedWith(7, "one of"));

    EXPECT_THAT(MReal(Exactly(Real("5.5")), OneOf({Real(11, 2), Real(6)})),
                IsSatisfiedWith(5.5));
    EXPECT_THAT(MReal(Exactly(Real("5.5")), OneOf({Real(11, 2), Real(6)}),
                      Between(Real("5.5"), 1000000)),
                IsSatisfiedWith(5.5));
    EXPECT_THAT(MReal(Exactly(Real(5)), OneOf({Real(5), Real(6, 5), Real(7)})),
                IsNotSatisfiedWith(6.0 / 5, "exactly"));
    EXPECT_THAT([] { MReal(Exactly(Real(5, 2)), OneOf({Real(6), Real(5)})); },
                ThrowsImpossibleToSatisfy("one of"));
  }
  {  // Generate
    EXPECT_THAT(MReal(Exactly(5)), GeneratedValuesAre(5));
    EXPECT_THAT(MReal(Exactly(Real("5.5"))), GeneratedValuesAre(5.5));

    EXPECT_THAT(MReal(OneOf({Real("5.1"), Real("5.2"), Real("5.3")})),
                GeneratedValuesAre(AnyOf(5.1, 5.2, 5.3)));
    EXPECT_THAT(MReal(Exactly(5), OneOf({5, 6, 7})), GeneratedValuesAre(5));
    EXPECT_THAT(MReal(Exactly(5), Between(1, 10)), GeneratedValuesAre(5));
    EXPECT_THAT(MReal(OneOf({Real("5.1"), Real("5.2"), Real("5.3")}),
                      Between(Real(1), Real("5.2"))),
                GeneratedValuesAre(AnyOf(5.1, 5.2)));
  }
  {  // GetUniqueValue
    EXPECT_THAT(GetUniqueValue(MReal(Exactly(5))), Optional(5));
    EXPECT_THAT(GetUniqueValue(MReal(Exactly(Real("5.1")))), Optional(5.1));
    EXPECT_THAT(GetUniqueValue(MReal(OneOf({5, 6, 7}))), std::nullopt);
    EXPECT_THAT(
        GetUniqueValue(MReal(OneOf({Real("5.1"), Real("5.2"), Real("5.3")}))),
        std::nullopt);
    EXPECT_THAT(
        GetUniqueValue(MReal(OneOf({Real("5.1"), Real("5.2"), Real("5.3")}),
                             Exactly(Real("5.2")))),
        Optional(5.2));
    EXPECT_THAT(GetUniqueValue(MReal(OneOf({5, 6, 7, 8}), OneOf({9, 7, 4}))),
                Optional(7));
  }
}

TEST(MRealTest, ExactlyAndOneOfConstraintsWithVariablesShouldWork) {
  {  // IsSatisfiedWith
    EXPECT_THAT(MReal(Exactly("N")),
                IsSatisfiedWith(10, Context().WithValue<MInteger>("N", 10)));
    EXPECT_THAT(MReal(Exactly("N")),
                IsNotSatisfiedWith(11, "exactly",
                                   Context().WithValue<MInteger>("N", 10)));

    EXPECT_THAT(
        MReal(OneOf({"N", "N+1", "N+2", "123"})),
        AllOf(IsSatisfiedWith(5, Context().WithValue<MInteger>("N", 5)),
              IsSatisfiedWith(6, Context().WithValue<MInteger>("N", 5)),
              IsSatisfiedWith(7, Context().WithValue<MInteger>("N", 5)),
              IsSatisfiedWith(123, Context().WithValue<MInteger>("N", 5))));
    EXPECT_THAT(
        MReal(OneOf({"N", "N+1", "N+2"})),
        IsNotSatisfiedWith(8, "one of", Context().WithValue<MInteger>("N", 5)));

    EXPECT_THAT(MReal(Exactly("N"), OneOf({"N-1", "N+1", "N*1", "N/1"})),
                IsSatisfiedWith(5, Context().WithValue<MInteger>("N", 5)));
    EXPECT_THAT(MReal(Exactly("N + 1"), OneOf({"N-1", "N+1", "N*1", "N/1"}),
                      Between("N-1", 1000)),
                IsSatisfiedWith(6, Context().WithValue<MInteger>("N", 5)));
    EXPECT_THAT(MReal(Exactly("N"), OneOf({"N", "N+1", "N+2"})),
                IsNotSatisfiedWith(6, "exactly",
                                   Context().WithValue<MInteger>("N", 5)));
    EXPECT_THAT(
        MReal(Exactly("N"), OneOf({6, 7, 8})),
        IsNotSatisfiedWith(5, "one of", Context().WithValue<MInteger>("N", 5)));
    EXPECT_THAT(
        MReal(Exactly("N"), OneOf({"N+1", "N+2"})),
        IsNotSatisfiedWith(5, "one of", Context().WithValue<MInteger>("N", 5)));
  }
  {  // Generate
    EXPECT_THAT(MReal(Exactly(5)), GeneratedValuesAre(5));
    EXPECT_THAT(MReal(OneOf({5, 6, 7})), GeneratedValuesAre(AnyOf(5, 6, 7)));
    EXPECT_THAT(MReal(Exactly(5), OneOf({5, 6, 7})), GeneratedValuesAre(5));
    EXPECT_THAT(MReal(Exactly(5), OneOf({4, 5, 6}), Between(5, 10)),
                GeneratedValuesAre(5));
    EXPECT_THAT(MReal(Exactly(5), Between(1, 10)), GeneratedValuesAre(5));
    EXPECT_THAT(MReal(OneOf({5, 6, 7}), Between(1, 6)),
                GeneratedValuesAre(AnyOf(5, 6)));
  }
  {  // GetUniqueValue
    EXPECT_THAT(GetUniqueValue(MReal(Exactly("N")),
                               Context().WithValue<MInteger>("N", 10)),
                Optional(10));
    EXPECT_THAT(GetUniqueValue(MReal(OneOf({"N", "N+1", "N+4"})),
                               Context().WithValue<MInteger>("N", 10)),
                std::nullopt);

    EXPECT_THAT(GetUniqueValue(MReal(OneOf({"N", "N+1"}), Exactly("N")),
                               Context().WithValue<MInteger>("N", 10)),
                Optional(10));

    EXPECT_THAT(GetUniqueValue(MReal(OneOf({"N", "N+1"}), OneOf({9, 10})),
                               Context().WithValue<MInteger>("N", 10)),
                Optional(10));
  }
}

}  // namespace
}  // namespace moriarty
