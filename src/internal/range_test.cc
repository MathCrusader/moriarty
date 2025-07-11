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

#include "src/internal/range.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/types/real.h"

namespace moriarty {
namespace {

using ::testing::Optional;

// Useful as a parameter to pass to Extremes() when you don't care about the
// variables
int64_t NoVariablesKnown(std::string_view var) {
  throw std::invalid_argument("No variables known");
}

std::function<int64_t(std::string_view)> FromMap(
    const std::unordered_map<std::string, int64_t>& map) {
  return
      [=](std::string_view var) -> int64_t { return map.at(std::string(var)); };
}

Range NewRange(int64_t min, int64_t max) {
  return Range().AtLeast(min).AtMost(max);
}

Range Intersect(Range r1, const Range& r2) {
  r1.Intersect(r2);
  return r1;
}

// Checks if two ranges are equal, taking into account any expressions.
testing::AssertionResult EqualRanges(const Range& r1, const Range& r2) {
  auto extremes1 = r1.IntegerExtremes(NoVariablesKnown);
  auto extremes2 = r2.IntegerExtremes(NoVariablesKnown);

  if (!extremes1.has_value() && !extremes2.has_value())
    return testing::AssertionSuccess() << "both ranges are empty";

  if (!extremes1.has_value())
    return testing::AssertionFailure()
           << "first range is empty, second is "
           << "[" << extremes2->min << ", " << extremes2->max << "]";

  if (!extremes2.has_value())
    return testing::AssertionFailure()
           << "first range is "
           << "[" << extremes1->min << ", " << extremes1->max << "]"
           << ", second is empty";

  if (extremes1 == extremes2)
    return testing::AssertionSuccess()
           << "both ranges are [" << extremes1->min << ", " << extremes1->max;

  return testing::AssertionFailure()
         << "are not equal "
         << "[" << extremes1->min << ", " << extremes1->max << "]"
         << " vs "
         << "[" << extremes2->min << ", " << extremes2->max << "]";
}

// Checks if a range is empty, taking into account any expressions.
testing::AssertionResult IsEmptyRange(const Range& r) {
  auto extremes = r.IntegerExtremes(NoVariablesKnown);

  if (!extremes)
    return testing::AssertionSuccess() << "is an empty range";
  else
    return testing::AssertionFailure()
           << "[" << extremes->min << ", " << extremes->min
           << "] is a non-empty range";
}

TEST(RangeTest, IntersectNonEmptyIntersectionsWork) {
  // Partial overlap
  EXPECT_TRUE(EqualRanges(Intersect(NewRange(1, 10), NewRange(5, 12)),
                          NewRange(5, 10)));
  // Subset
  EXPECT_TRUE(
      EqualRanges(Intersect(NewRange(1, 10), NewRange(5, 8)), NewRange(5, 8)));
  EXPECT_TRUE(EqualRanges(Intersect(NewRange(1, 10), NewRange(0, 18)),
                          NewRange(1, 10)));
  // Equal
  EXPECT_TRUE(EqualRanges(Intersect(NewRange(1, 10), NewRange(1, 10)),
                          NewRange(1, 10)));
  // Singleton overlap
  EXPECT_TRUE(EqualRanges(Intersect(NewRange(1, 10), NewRange(10, 100)),
                          NewRange(10, 10)));
  EXPECT_TRUE(
      EqualRanges(Intersect(NewRange(1, 10), NewRange(-5, 1)), NewRange(1, 1)));
}

TEST(RangeTest, EmptyRangeWorks) {
  EXPECT_TRUE(IsEmptyRange(NewRange(0, -1)));
  EXPECT_TRUE(IsEmptyRange(NewRange(10, 2)));

  EXPECT_FALSE(IsEmptyRange(NewRange(0, 0)));
  EXPECT_FALSE(IsEmptyRange(NewRange(5, 10)));

  EXPECT_TRUE(IsEmptyRange(EmptyRange()));
}

TEST(RangeTest, EqualityWorks) {
  // Normal cases
  EXPECT_EQ(NewRange(1, 2), NewRange(1, 2));
  EXPECT_NE(NewRange(1, 3), NewRange(1, 2));
  Range r;
  r.AtLeast(1);
  r.AtMost(2);
  EXPECT_EQ(r, NewRange(1, 2));
  EXPECT_EQ(EmptyRange(), NewRange(1, 0));  // Not guaranteed to be equal

  // Expressions are considered. This is not guaranteed to be stable over time.
  Range r1;
  r1.AtLeast(1);
  r1.AtMost(2);
  Range r2;
  r2.AtLeast(Expression("1"));
  r2.AtMost(Expression("2"));
  EXPECT_NE(r1, r2);

  Range r3 = NewRange(1, 4);
  r3.AtLeast(Expression("a"));
  r3.AtMost(Expression("b"));
  Range r4;
  r4.AtLeast(1);
  r4.AtMost(4);
  r4.AtLeast(Expression("a"));
  r4.AtMost(Expression("b"));
  EXPECT_EQ(r3, r4);
}

TEST(RangeTest, EmptyIntersectionsWork) {
  // Normal cases
  EXPECT_TRUE(IsEmptyRange(Intersect(NewRange(1, 10), NewRange(11, 100))));
  EXPECT_TRUE(IsEmptyRange(Intersect(NewRange(101, 1000), NewRange(11, 100))));

  // Input was already empty
  EXPECT_TRUE(IsEmptyRange(Intersect(NewRange(10, 1), NewRange(5, 5))));
  EXPECT_TRUE(IsEmptyRange(Intersect(NewRange(10, 1), NewRange(10, 1))));

  EXPECT_FALSE(IsEmptyRange(Intersect(NewRange(10, 10), NewRange(10, 100))));
}

TEST(RangeTest, ExtremesWork) {
  EXPECT_EQ(Range::ExtremeValues<int64_t>({1, 2}),
            Range::ExtremeValues<int64_t>({1, 2}));

  // Normal case
  EXPECT_THAT(NewRange(1, 2).IntegerExtremes(NoVariablesKnown),
              Optional(Range::ExtremeValues<int64_t>({1, 2})));

  // Default constructor gives full 64-bit range
  EXPECT_THAT(Range().IntegerExtremes(NoVariablesKnown),
              Optional(Range::ExtremeValues<int64_t>(
                  {std::numeric_limits<int64_t>::min(),
                   std::numeric_limits<int64_t>::max()})));

  // Empty range returns nullopt
  EXPECT_THAT(EmptyRange().IntegerExtremes(NoVariablesKnown), std::nullopt);
}

TEST(RangeTest, ExtremesWithFnShouldWork) {
  auto get_value = [](std::string_view var) -> int64_t {
    if (var == "x") return 1;
    if (var == "y") return 20;
    throw std::runtime_error("Unexpected variable: " + std::string(var));
  };

  EXPECT_THAT(Range()
                  .AtLeast(Expression("x"))
                  .AtMost(Expression("y + 1"))
                  .IntegerExtremes(get_value),
              Optional(Range::ExtremeValues<int64_t>({1, 21})));
}

TEST(RangeTest,
     RepeatedCallsToAtMostAndAtLeastIntegerVersionsShouldConsiderAll) {
  Range R;
  R.AtLeast(5);
  R.AtLeast(6);
  R.AtLeast(4);

  EXPECT_THAT(R.IntegerExtremes(NoVariablesKnown),
              Optional(Range::ExtremeValues<int64_t>(
                  {6, std::numeric_limits<int64_t>::max()})));

  R.AtMost(30);
  R.AtMost(20);
  R.AtMost(10);
  R.AtMost(15);
  EXPECT_THAT(R.IntegerExtremes(NoVariablesKnown),
              Optional(Range::ExtremeValues<int64_t>({6, 10})));

  R.AtMost(5);
  EXPECT_TRUE(IsEmptyRange(R));
}

TEST(RangeTest, ExpressionsWorkInAtLeastAndAtMost) {
  Range R;
  R.AtLeast(Expression("N + 5"));
  R.AtMost(Expression("3 * N + 1"));

  EXPECT_THAT(R.IntegerExtremes(FromMap({{"N", 4}})),
              Optional(Range::ExtremeValues<int64_t>({9, 13})));
  EXPECT_THAT(R.IntegerExtremes(FromMap({{"N", 2}})),
              Optional(Range::ExtremeValues<int64_t>({7, 7})));
  EXPECT_EQ(R.IntegerExtremes(FromMap({{"N", 0}})), std::nullopt);
}

TEST(RangeTest, RealsWorkInAtLeastAndAtMost) {
  EXPECT_EQ(
      Range().AtLeast(Real(5, 2)).AtMost(Real(0)).RealExtremes(FromMap({})),
      std::nullopt);
  EXPECT_THAT(
      Range().AtLeast(Real(5, 2)).AtMost(Real("1e6")).RealExtremes(FromMap({})),
      Optional(Range::ExtremeValues<Real>({Real(5, 2), Real(1000000)})));
  EXPECT_THAT(Range().AtLeast(Real(5, 2)).AtMost(100).RealExtremes(FromMap({})),
              Optional(Range::ExtremeValues<Real>({Real(5, 2), Real(100)})));
  EXPECT_THAT(Range()
                  .AtLeast(Real("2.4"))
                  .AtLeast(Real(5, 2))
                  .AtMost(100)
                  .RealExtremes(FromMap({})),
              Optional(Range::ExtremeValues<Real>({Real(5, 2), Real(100)})));
}

TEST(RangeTest,
     RepeatedCallsToAtMostAndAtLeastExpressionVersionsShouldConsiderAll) {
  Range R;  // {y>=3x+1, y>=-x+3, y>=x+5, y<=x+15, y<=-x+15}

  R.AtLeast(Expression("-N + 3"));     // Valid: (-infinity, -1]
  R.AtLeast(Expression("N + 5"));      // Valid: [-1, 1.5]
  R.AtLeast(Expression("3 * N + 1"));  // Valid: [1.5, infinity)

  R.AtMost(Expression("-N + 15"));  // Valid: [0, infinity)
  R.AtMost(Expression("N + 15"));   // Valid: (-infinity, 0]

  // Left of valid range (-infinity, -6)
  EXPECT_THAT(R.IntegerExtremes(FromMap({{"N", -10}})), std::nullopt);
  // Between -N + 3 and N + 15 [-6, -1]
  EXPECT_THAT(R.IntegerExtremes(FromMap({{"N", -6}})),
              Optional(Range::ExtremeValues<int64_t>({9, 9})));
  // Between N + 5 and N + 15 [-1, 0]
  EXPECT_THAT(R.IntegerExtremes(FromMap({{"N", 0}})),
              Optional(Range::ExtremeValues<int64_t>({5, 15})));
  // Between N + 5 and -N + 15 [0, 2]
  EXPECT_THAT(R.IntegerExtremes(FromMap({{"N", 1}})),
              Optional(Range::ExtremeValues<int64_t>({6, 14})));
  // Between 3 * N + 1 and -N + 15 [2, 3.5]
  EXPECT_THAT(R.IntegerExtremes(FromMap({{"N", 3}})),
              Optional(Range::ExtremeValues<int64_t>({10, 12})));
  // Right of valid range (3.5, infinity)
  EXPECT_THAT(R.IntegerExtremes(FromMap({{"N", 4}})), std::nullopt);
}

TEST(RangeTest, IntegersShouldConsiderAllTypes) {
  {  // At most with expression and integer
    Range R = Range().AtLeast(-100).AtMost(3).AtMost(Expression("N"));

    // "N" smaller
    EXPECT_THAT(R.IntegerExtremes(FromMap({{"N", 1}})),
                Optional(Range::ExtremeValues<int64_t>({-100, 1})));
    // "N" and 3 the same
    EXPECT_THAT(R.IntegerExtremes(FromMap({{"N", 3}})),
                Optional(Range::ExtremeValues<int64_t>({-100, 3})));
    // "N" larger
    EXPECT_THAT(R.IntegerExtremes(FromMap({{"N", 5}})),
                Optional(Range::ExtremeValues<int64_t>({-100, 3})));
  }
  {  // At most with expression and Real
    Range R = Range().AtLeast(-100).AtMost(Real(3, 2)).AtMost(Expression("N"));

    // "N" smaller
    EXPECT_THAT(R.IntegerExtremes(FromMap({{"N", 0}})),
                Optional(Range::ExtremeValues<int64_t>({-100, 0})));
    // "N" and floor(3/2) the same
    EXPECT_THAT(R.IntegerExtremes(FromMap({{"N", 1}})),
                Optional(Range::ExtremeValues<int64_t>({-100, 1})));
    // "N" larger
    EXPECT_THAT(R.IntegerExtremes(FromMap({{"N", 5}})),
                Optional(Range::ExtremeValues<int64_t>({-100, 1})));
  }
  {  // At least with expression and integer
    Range R = Range().AtMost(100).AtLeast(2).AtLeast(Expression("N"));

    // "N" smaller
    EXPECT_THAT(R.IntegerExtremes(FromMap({{"N", 0}})),
                Optional(Range::ExtremeValues<int64_t>({2, 100})));
    // "N" and 2 the same
    EXPECT_THAT(R.IntegerExtremes(FromMap({{"N", 1}})),
                Optional(Range::ExtremeValues<int64_t>({2, 100})));
    // "N" larger
    EXPECT_THAT(R.IntegerExtremes(FromMap({{"N", 5}})),
                Optional(Range::ExtremeValues<int64_t>({5, 100})));
  }
  {  // At least with expression and Real
    Range R = Range().AtMost(100).AtLeast(Real(3, 2)).AtLeast(Expression("N"));

    // "N" smaller
    EXPECT_THAT(R.IntegerExtremes(FromMap({{"N", 0}})),
                Optional(Range::ExtremeValues<int64_t>({2, 100})));
    // "N" and ceil(3/2) the same
    EXPECT_THAT(R.IntegerExtremes(FromMap({{"N", 1}})),
                Optional(Range::ExtremeValues<int64_t>({2, 100})));
    // "N" larger
    EXPECT_THAT(R.IntegerExtremes(FromMap({{"N", 5}})),
                Optional(Range::ExtremeValues<int64_t>({5, 100})));
  }
  {  // At least / at most with real and integer
    auto get = [](std::string_view var) -> int64_t {
      throw std::runtime_error("Unexpected variable: " + std::string(var));
    };
    EXPECT_THAT(
        Range().AtMost(100).AtLeast(Real(3, 2)).AtLeast(1).IntegerExtremes(get),
        Optional(Range::ExtremeValues<int64_t>({2, 100})));
    EXPECT_THAT(
        Range().AtMost(100).AtLeast(Real(3, 2)).AtLeast(3).IntegerExtremes(get),
        Optional(Range::ExtremeValues<int64_t>({3, 100})));
    EXPECT_THAT(
        Range().AtLeast(-100).AtMost(Real(3, 2)).AtMost(0).IntegerExtremes(get),
        Optional(Range::ExtremeValues<int64_t>({-100, 0})));
    EXPECT_THAT(
        Range().AtLeast(-100).AtMost(Real(3, 2)).AtMost(3).IntegerExtremes(get),
        Optional(Range::ExtremeValues<int64_t>({-100, 1})));
  }
}

TEST(RangeTest, ToStringWithEmptyOrDefaultRangeShouldWork) {
  EXPECT_EQ(Range().ToString(), "(-inf, inf)");
  EXPECT_EQ(EmptyRange().ToString(), "[1, 0]");  // Not guaranteed to be equal
}

TEST(RangeTest, TwoSidedInequalitiesToStringShouldWork) {
  Range r1;
  r1.AtLeast(1);
  r1.AtMost(5);
  EXPECT_EQ(r1.ToString(), "[1, 5]");

  Range r2;
  r2.AtLeast(Expression("N"));
  r2.AtMost(5);
  EXPECT_EQ(r2.ToString(), "[N, 5]");

  Range r3;
  r3.AtLeast(Expression("N"));
  r3.AtMost(Expression("M"));
  EXPECT_EQ(r3.ToString(), "[N, M]");
}

TEST(RangeTest, OneSidedInequalitiesToStringShouldWork) {
  Range r1;
  r1.AtLeast(1);
  EXPECT_EQ(r1.ToString(), "[1, inf)");

  Range r2;
  r2.AtMost(5);
  EXPECT_EQ(r2.ToString(), "(-inf, 5]");

  Range r3;
  r3.AtMost(Expression("M"));
  EXPECT_EQ(r3.ToString(), "(-inf, M]");

  Range r4;
  r4.AtLeast(Expression("M"));
  EXPECT_EQ(r4.ToString(), "[M, inf)");
}

TEST(RangeTest, InequalitiesWithMultipleItemsShouldWork) {
  Range r1;
  r1.AtLeast(1);
  r1.AtLeast(Expression("3 * N"));
  r1.AtLeast(Real(5, 2));
  EXPECT_EQ(r1.ToString(), "[max(1, 5/2, 3 * N), inf)");

  Range r2;
  r2.AtMost(5);
  r2.AtMost(Expression("3 * N"));
  r2.AtMost(Real(-5, 2));
  EXPECT_EQ(r2.ToString(), "(-inf, min(5, -5/2, 3 * N)]");

  Range r3;
  r3.AtLeast(Expression("a"));
  r3.AtLeast(Expression("b"));
  r3.AtMost(Expression("c"));
  r3.AtMost(Expression("d"));
  EXPECT_EQ(r3.ToString(), "[max(a, b), min(c, d)]");

  Range r4;
  r4.AtLeast(Expression("a"));
  r4.AtMost(Expression("c"));
  r4.AtMost(Expression("d"));
  EXPECT_EQ(r4.ToString(), "[a, min(c, d)]");
}

}  // namespace
}  // namespace moriarty
