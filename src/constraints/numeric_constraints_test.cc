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

#include "src/constraints/numeric_constraints.h"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string_view>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/contexts/librarian_context.h"
#include "src/internal/range.h"
#include "src/librarian/errors.h"
#include "src/librarian/testing/gtest_helpers.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace {

using ::moriarty::AtLeast;
using ::moriarty::AtMost;
using ::moriarty::Between;
using ::moriarty::librarian::ExactlyNumeric;
using ::moriarty::librarian::OneOfNumeric;
using ::moriarty_testing::Context;
using ::moriarty_testing::HasConstraintViolation;
using ::moriarty_testing::HasNoConstraintViolation;
using ::testing::HasSubstr;
using ::testing::Optional;
using ::testing::Throws;
using ::testing::UnorderedElementsAre;

int64_t NoVariablesKnown(std::string_view var) {
  throw std::invalid_argument("No variables known");
}

Range NewRange(int64_t min, int64_t max) {
  return Range().AtLeast(min).AtMost(max);
}

testing::AssertionResult EqualRanges(const Range& r1, const Range& r2) {
  auto extremes1 = r1.IntegerExtremes(NoVariablesKnown);
  auto extremes2 = r2.IntegerExtremes(NoVariablesKnown);

  if (!extremes1.has_value() && !extremes2.has_value())
    return testing::AssertionSuccess() << "both ranges are empty";

  if (!extremes1.has_value())
    return testing::AssertionFailure()
           << "first range is empty, second is " << "[" << extremes2->min
           << ", " << extremes2->max << "]";

  if (!extremes2.has_value())
    return testing::AssertionFailure()
           << "first range is " << "[" << extremes1->min << ", "
           << extremes1->max << "]" << ", second is empty";

  if (extremes1 == extremes2)
    return testing::AssertionSuccess()
           << "both ranges are [" << extremes1->min << ", " << extremes1->max;

  return testing::AssertionFailure()
         << "are not equal " << "[" << extremes1->min << ", " << extremes1->max
         << "]" << " vs " << "[" << extremes2->min << ", " << extremes2->max
         << "]";
}

TEST(NumericConstraintsTest, InvalidBetweenShouldThrow) {
  EXPECT_THAT([] { Between(0, -5); }, Throws<InvalidConstraint>());
}

TEST(NumericConstraintsTest, InvalidExpressionsShouldThrow) {
  {
    EXPECT_THAT([] { ExactlyNumeric("2 *"); }, Throws<std::invalid_argument>());
    EXPECT_THAT([] { OneOfNumeric(std::vector<std::string_view>{"3", "2 *"}); },
                Throws<std::invalid_argument>());
    EXPECT_THAT([] { Between("2 *", 5); }, Throws<std::invalid_argument>());
    EXPECT_THAT([] { Between(5, "2 *"); }, Throws<std::invalid_argument>());
    EXPECT_THAT([] { AtMost("2 *"); }, Throws<std::invalid_argument>());
    EXPECT_THAT([] { AtLeast("2 *"); }, Throws<std::invalid_argument>());
  }
  {
    EXPECT_THAT([] { ExactlyNumeric(""); }, Throws<std::invalid_argument>());
    EXPECT_THAT([] { OneOfNumeric(std::vector<std::string_view>{""}); },
                Throws<std::invalid_argument>());
    EXPECT_THAT([] { Between("", 5); }, Throws<std::invalid_argument>());
    EXPECT_THAT([] { Between(5, ""); }, Throws<std::invalid_argument>());
    EXPECT_THAT([] { AtMost(""); }, Throws<std::invalid_argument>());
    EXPECT_THAT([] { AtLeast(""); }, Throws<std::invalid_argument>());
  }
}

TEST(NumericConstraintsTest, GetRangeShouldGiveCorrectValues) {
  {
    EXPECT_TRUE(
        EqualRanges(ExactlyNumeric("3 * 10 + 1").GetRange(), NewRange(31, 31)));
  }
  {
    EXPECT_TRUE(EqualRanges(Between(10, 20).GetRange(), NewRange(10, 20)));
    EXPECT_TRUE(EqualRanges(Between("10", 20).GetRange(), NewRange(10, 20)));
    EXPECT_TRUE(EqualRanges(Between(10, "20").GetRange(), NewRange(10, 20)));
    EXPECT_TRUE(EqualRanges(Between("10", "20").GetRange(), NewRange(10, 20)));
  }
  {
    Range expected;
    expected.AtLeast(20);
    EXPECT_TRUE(EqualRanges(AtLeast(20).GetRange(), expected));
    EXPECT_TRUE(EqualRanges(AtLeast("20").GetRange(), expected));
  }
  {
    Range expected;
    expected.AtMost(23);
    EXPECT_TRUE(EqualRanges(AtMost(23).GetRange(), expected));
    EXPECT_TRUE(EqualRanges(AtMost("23").GetRange(), expected));
  }
}

TEST(NumericConstraintsTest, GetOptionsShouldGiveCorrectValues) {
  Context context;
  context.WithValue<MInteger>("x", 10)
      .WithValue<MInteger>("y", 20)
      .WithValue<MInteger>("z", 30);
  librarian::AnalyzeVariableContext ctx("N", context.Variables(),
                                        context.Values());

  EXPECT_THAT(OneOfNumeric(std::vector<std::string_view>{"1", "2", "3"})
                  .GetOptions(ctx),
              UnorderedElementsAre(1, 2, 3));
  EXPECT_THAT(OneOfNumeric(std::vector<std::string_view>{"x + 5", "y", "z"})
                  .GetOptions(ctx),
              UnorderedElementsAre(15, 20, 30));
  EXPECT_THAT(OneOfNumeric(std::vector<std::string_view>{"1", "3", "5"})
                  .GetOptions(ctx),
              UnorderedElementsAre(1, 3, 5));
  EXPECT_THAT(
      OneOfNumeric(std::vector<Real>{Real(1, 2), Real(3, 2), Real(5, 1)})
          .GetOptions(ctx),
      UnorderedElementsAre(Real(1, 2), Real(3, 2), 5));

  {
    OneOfNumeric o1(std::vector<int64_t>{15, 25, 30});
    OneOfNumeric o2(std::vector<std::string_view>{"x + 5", "y", "z"});
    EXPECT_TRUE(o1.ConstrainOptions(o2));
    EXPECT_THAT(o1.GetOptions(ctx), UnorderedElementsAre(15, 30));
  }
}

TEST(NumericConstraintsTest, OneOfGetUniqueValueShouldWork) {
  Context context;
  context.WithValue<MInteger>("x", 10)
      .WithValue<MInteger>("y", 20)
      .WithValue<MInteger>("z", 30);
  librarian::AnalyzeVariableContext ctx("N", context.Variables(),
                                        context.Values());

  {  // No unique value
    EXPECT_EQ(OneOfNumeric().GetUniqueValue(ctx), std::nullopt);
    EXPECT_EQ(OneOfNumeric(std::vector<std::string_view>{"1", "2", "3"})
                  .GetUniqueValue(ctx),
              std::nullopt);
    EXPECT_EQ(
        OneOfNumeric(std::vector<Real>{Real(1, 2), Real(3, 2), Real(5, 1)})
            .GetUniqueValue(ctx),
        std::nullopt);

    OneOfNumeric o1(std::vector<int64_t>{15, 25, 30});
    OneOfNumeric o2(std::vector<std::string_view>{"x + 5", "y", "z"});
    EXPECT_TRUE(o1.ConstrainOptions(o2));
    EXPECT_EQ(o1.GetUniqueValue(ctx), std::nullopt);

    EXPECT_FALSE(o1.ConstrainOptions(OneOfNumeric(std::vector<int64_t>{0})));
    EXPECT_EQ(o1.GetUniqueValue(ctx), std::nullopt);
  }

  {  // Has unique value
    EXPECT_THAT(OneOfNumeric(std::vector<int64_t>{3}).GetUniqueValue(ctx),
                Optional(3));
    EXPECT_THAT(
        OneOfNumeric(std::vector<std::string_view>{"x + 20", "y + 10", "z"})
            .GetUniqueValue(ctx),
        Optional(30));

    OneOfNumeric o1(std::vector<int64_t>{15, 25, 35});
    OneOfNumeric o2(std::vector<std::string_view>{"x + 5", "y", "z"});
    EXPECT_TRUE(o1.ConstrainOptions(o2));
    EXPECT_THAT(o1.GetUniqueValue(ctx), Optional(15));
  }
}

TEST(NumericConstraintsTest, ToStringShouldWork) {
  {
    EXPECT_EQ(ExactlyNumeric("3 * X + 1").ToString(), "is exactly 3 * X + 1");
  }
  {
    EXPECT_EQ(
        OneOfNumeric(std::vector<std::string_view>{"1", "x+3"}).ToString(),
        "is one of: [1, x+3]");
    EXPECT_EQ(OneOfNumeric(std::vector<std::string_view>{"x+3"}).ToString(),
              "is one of: [x+3]");
    OneOfNumeric o1(std::vector<int64_t>{15, 25, 30});
    OneOfNumeric o2(std::vector<std::string_view>{"x + 5", "y", "z"});
    EXPECT_TRUE(o1.ConstrainOptions(o2));
    EXPECT_EQ(
        o1.ToString(),
        "is one of the elements from each list: {[x + 5, y, z], [15, 25, 30]}");
  }
  {
    EXPECT_EQ(Between(10, 20).ToString(), "is between 10 and 20");
    EXPECT_EQ(Between("10 + X", 20).ToString(), "is between 10 + X and 20");
    EXPECT_EQ(Between(10, "20 + Y").ToString(), "is between 10 and 20 + Y");
    EXPECT_EQ(Between("10 + X", "20 + Y").ToString(),
              "is between 10 + X and 20 + Y");
  }
  {
    EXPECT_EQ(AtMost(10).ToString(), "is at most 10");
    EXPECT_EQ(AtMost("N + 3").ToString(), "is at most N + 3");
  }

  {
    EXPECT_EQ(AtLeast(10).ToString(), "is at least 10");
    EXPECT_EQ(AtLeast("N + 5").ToString(), "is at least N + 5");
  }
}

TEST(NumericConstraintsTest, IsSatisfiedWithWithIntegersWorks) {
  Context context;
  librarian::AnalyzeVariableContext ctx("N", context.Variables(),
                                        context.Values());

  {
    EXPECT_THAT(Between(10, 20).CheckValue(ctx, 10),
                HasNoConstraintViolation());
    EXPECT_THAT(Between(10, 20).CheckValue(ctx, 15),
                HasNoConstraintViolation());
    EXPECT_THAT(Between(10, 20).CheckValue(ctx, 20),
                HasNoConstraintViolation());
    EXPECT_THAT(Between(10, 20).CheckValue(ctx, 9),
                HasConstraintViolation(HasSubstr("between")));
    EXPECT_THAT(Between(10, 20).CheckValue(ctx, 21),
                HasConstraintViolation(HasSubstr("between")));
  }
  {
    EXPECT_THAT(AtLeast(10).CheckValue(ctx, 10), HasNoConstraintViolation());
    EXPECT_THAT(AtLeast(10).CheckValue(ctx, 11), HasNoConstraintViolation());
    EXPECT_THAT(AtLeast(10).CheckValue(ctx, 9),
                HasConstraintViolation(HasSubstr("at least")));
  }
  {
    EXPECT_THAT(AtMost(10).CheckValue(ctx, 10), HasNoConstraintViolation());
    EXPECT_THAT(AtMost(10).CheckValue(ctx, 9), HasNoConstraintViolation());
    EXPECT_THAT(AtMost(10).CheckValue(ctx, 11),
                HasConstraintViolation(HasSubstr("at most")));
  }
}

TEST(NumericConstraintsTest, IsSatisfiedWithRealsWorks) {
  Context context;
  context.WithValue<MInteger>("x", 10)
      .WithValue<MInteger>("y", 20)
      .WithValue<MInteger>("z", 30);
  librarian::AnalyzeVariableContext ctx("N", context.Variables(),
                                        context.Values());

  {
    EXPECT_THAT(Between(10, 20).CheckValue(ctx, 10.0),
                HasNoConstraintViolation());
    EXPECT_THAT(Between(10, 20).CheckValue(ctx, 15.5),
                HasNoConstraintViolation());
    EXPECT_THAT(Between(10, 20).CheckValue(ctx, 20.0),
                HasNoConstraintViolation());
    EXPECT_THAT(Between(10, 20).CheckValue(ctx, 9.9),
                HasConstraintViolation(HasSubstr("between")));
    EXPECT_THAT(Between(10, 20).CheckValue(ctx, 20.1),
                HasConstraintViolation(HasSubstr("between")));
  }
  {
    EXPECT_THAT(AtLeast(10).CheckValue(ctx, 10.0), HasNoConstraintViolation());
    EXPECT_THAT(AtLeast(10).CheckValue(ctx, 10.1), HasNoConstraintViolation());
    EXPECT_THAT(AtLeast(10).CheckValue(ctx, 9.9),
                HasConstraintViolation(HasSubstr("at least")));
  }
  {
    EXPECT_THAT(AtMost(10).CheckValue(ctx, 10.0), HasNoConstraintViolation());
    EXPECT_THAT(AtMost(10).CheckValue(ctx, 9.9), HasNoConstraintViolation());
    EXPECT_THAT(AtMost(10).CheckValue(ctx, 10.1),
                HasConstraintViolation(HasSubstr("at most")));
  }
  {
    EXPECT_THAT(Between("x", "y").CheckValue(ctx, 15.0),
                HasNoConstraintViolation());
    EXPECT_THAT(Between("x", "y").CheckValue(ctx, 9.0),
                HasConstraintViolation(HasSubstr("between")));
    EXPECT_THAT(Between("x", "y").CheckValue(ctx, 21.0),
                HasConstraintViolation(HasSubstr("between")));
  }
  {
    EXPECT_THAT(AtLeast("x").CheckValue(ctx, 10.0), HasNoConstraintViolation());
    EXPECT_THAT(AtLeast("x").CheckValue(ctx, 11.0), HasNoConstraintViolation());
    EXPECT_THAT(AtLeast("x").CheckValue(ctx, 9.0),
                HasConstraintViolation(HasSubstr("at least")));
  }
  {
    EXPECT_THAT(AtMost("y").CheckValue(ctx, 20.0), HasNoConstraintViolation());
    EXPECT_THAT(AtMost("y").CheckValue(ctx, 19.0), HasNoConstraintViolation());
    EXPECT_THAT(AtMost("y").CheckValue(ctx, 21.0),
                HasConstraintViolation(HasSubstr("at most")));
  }
  {
    EXPECT_THAT(AtMost(Real("20.0")).CheckValue(ctx, 20.0),
                HasNoConstraintViolation());
    EXPECT_THAT(AtMost(Real("1e6")).CheckValue(ctx, 1000000.0),
                HasNoConstraintViolation());
    EXPECT_THAT(AtMost(Real("-10e-2")).CheckValue(ctx, -0.1),
                HasNoConstraintViolation());
    EXPECT_THAT(AtMost(Real("-10e-2")).CheckValue(ctx, -0.01),
                HasConstraintViolation(HasSubstr("at most")));
  }
}

TEST(NumericConstraintsTest, IsSatisfiedWithWithExpressionWorks) {
  Context context;
  context.WithValue<MInteger>("x", 10)
      .WithValue<MInteger>("y", 20)
      .WithValue<MInteger>("z", 30);
  librarian::AnalyzeVariableContext ctx("N", context.Variables(),
                                        context.Values());

  {
    EXPECT_THAT(ExactlyNumeric("x").CheckValue(ctx, 10),
                HasNoConstraintViolation());
    EXPECT_THAT(ExactlyNumeric("x").CheckValue(ctx, 11),
                HasConstraintViolation(HasSubstr("exactly")));
    EXPECT_THAT(ExactlyNumeric("x").CheckValue(ctx, 9),
                HasConstraintViolation(HasSubstr("exactly")));
  }
  {
    EXPECT_THAT(OneOfNumeric(std::vector<std::string_view>{"x", "14"})
                    .CheckValue(ctx, 10),
                HasNoConstraintViolation());
    EXPECT_THAT(OneOfNumeric(std::vector<std::string_view>{"x", "14"})
                    .CheckValue(ctx, 14),
                HasNoConstraintViolation());
    EXPECT_THAT(OneOfNumeric(std::vector<std::string_view>{"x", "14"})
                    .CheckValue(ctx, 9),
                HasConstraintViolation(HasSubstr("one of")));
    EXPECT_THAT(OneOfNumeric(std::vector<std::string_view>{"x", "14"})
                    .CheckValue(ctx, 15),
                HasConstraintViolation(HasSubstr("one of")));
  }
  {
    EXPECT_THAT(Between("x", "y^2").CheckValue(ctx, 10),
                HasNoConstraintViolation());
    EXPECT_THAT(Between("x", "y^2").CheckValue(ctx, 25),
                HasNoConstraintViolation());
    EXPECT_THAT(Between("x", "y^2").CheckValue(ctx, 400),
                HasNoConstraintViolation());
    EXPECT_THAT(Between("x", "y^2").CheckValue(ctx, 9),
                HasConstraintViolation(HasSubstr("between")));
    EXPECT_THAT(Between("x", "y^2").CheckValue(ctx, 401),
                HasConstraintViolation(HasSubstr("between")));
  }
  {
    EXPECT_THAT(AtLeast("x").CheckValue(ctx, 10), HasNoConstraintViolation());
    EXPECT_THAT(AtLeast("x").CheckValue(ctx, 11), HasNoConstraintViolation());
    EXPECT_THAT(AtLeast("x").CheckValue(ctx, 9),
                HasConstraintViolation(HasSubstr("at least")));
  }
  {
    EXPECT_THAT(AtMost("x + 1").CheckValue(ctx, 11),
                HasNoConstraintViolation());
    EXPECT_THAT(AtMost("x + 1").CheckValue(ctx, 10),
                HasNoConstraintViolation());
    EXPECT_THAT(AtMost("x + 1").CheckValue(ctx, 12),
                HasConstraintViolation(HasSubstr("at most")));
  }
}

TEST(NumericConstraintsTest, MergeFromWorks) {
  {
    OneOfNumeric o1(std::vector<std::string_view>{"x", "y"});
    OneOfNumeric o2(std::vector<std::string_view>{"y", "z"});
    EXPECT_TRUE(o1.ConstrainOptions(o2));
    EXPECT_EQ(o1.ToString(),
              "is one of the elements from each list: {[x, y], [y, z]}");
  }
  {
    OneOfNumeric o1(std::vector<int64_t>{1, 2, 3});
    OneOfNumeric o2(std::vector<std::string_view>{"y", "z"});
    EXPECT_TRUE(o1.ConstrainOptions(o2));
    EXPECT_EQ(o1.ToString(),
              "is one of the elements from each list: {[y, z], [1, 2, 3]}");
  }
  {
    OneOfNumeric o1(std::vector<int64_t>{1, 2, 3});
    OneOfNumeric o2(std::vector<int64_t>{3, 4, 2});
    EXPECT_TRUE(o1.ConstrainOptions(o2));
    EXPECT_EQ(o1.ToString(), "is one of: [2, 3]");
  }
  {
    OneOfNumeric o1(std::vector<int64_t>{1, 2, 3});
    OneOfNumeric o2(std::vector<Real>{Real(3), Real(4, 2), Real(-22)});
    EXPECT_TRUE(o1.ConstrainOptions(o2));
    EXPECT_EQ(o1.ToString(), "is one of: [2, 3]");
  }
  {
    OneOfNumeric o1(std::vector<int64_t>{1, 2, 3});
    OneOfNumeric o2(std::vector<std::string_view>{"y", "z"});
    EXPECT_TRUE(o1.ConstrainOptions(o2));
    OneOfNumeric o3(std::vector<Real>{Real(3), Real(4, 2), Real(-22)});
    OneOfNumeric o4(std::vector<std::string_view>{"z", "x"});
    EXPECT_TRUE(o3.ConstrainOptions(o4));
    EXPECT_TRUE(o1.ConstrainOptions(o3));
    EXPECT_EQ(
        o1.ToString(),
        "is one of the elements from each list: {[y, z], [z, x], [2, 3]}");
  }
  {
    OneOfNumeric o1(std::vector<int64_t>{1, 2, 3});
    OneOfNumeric o2(std::vector<Real>{Real(1, 2)});
    EXPECT_FALSE(o1.ConstrainOptions(o2));

    OneOfNumeric o3(std::vector<std::string_view>{"a", "b"});
    OneOfNumeric o4(std::vector<std::string_view>{"z", "x"});
    EXPECT_TRUE(o3.ConstrainOptions(o4));  // It's possible a == x, etc.
  }
}

TEST(NumericConstraintsTest, OneOfHasBeenConstrainedWorks) {
  OneOfNumeric o1(std::vector<int64_t>{1, 2, 3});
  EXPECT_TRUE(o1.HasBeenConstrained());
  EXPECT_TRUE(o1.ConstrainOptions(OneOfNumeric(std::vector<int64_t>{3, 4, 2})));
  EXPECT_TRUE(o1.HasBeenConstrained());
  EXPECT_TRUE(o1.ConstrainOptions(
      OneOfNumeric(std::vector<std::string_view>{"x", "y"})));
  EXPECT_TRUE(o1.HasBeenConstrained());

  EXPECT_FALSE(OneOfNumeric().HasBeenConstrained());
}

}  // namespace
}  // namespace moriarty
