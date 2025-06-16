/*
 * Copyright 2024 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "src/variables/constraints/numeric_constraints.h"

#include <string_view>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/internal/range.h"
#include "src/testing/gtest_helpers.h"

namespace moriarty {
namespace {

using ::moriarty::AtLeast;
using ::moriarty::AtMost;
using ::moriarty::Between;
using ::moriarty_testing::HasConstraintViolation;
using ::moriarty_testing::HasNoConstraintViolation;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::Throws;

int64_t NoVariablesKnown(std::string_view var) {
  throw std::invalid_argument("No variables known");
}

testing::AssertionResult EqualRanges(const Range& r1, const Range& r2) {
  auto extremes1 = r1.Extremes(NoVariablesKnown);
  auto extremes2 = r2.Extremes(NoVariablesKnown);

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
  EXPECT_THAT([] { Between(0, -5); }, Throws<std::invalid_argument>());
}

TEST(NumericConstraintsTest, InvalidExpressionsShouldThrow) {
  {
    EXPECT_THAT([] { ExactlyIntegerExpression("2 *"); },
                Throws<std::invalid_argument>());
    EXPECT_THAT([] { OneOfIntegerExpression({"3", "2 *"}); },
                Throws<std::invalid_argument>());
    EXPECT_THAT([] { Between("2 *", 5); }, Throws<std::invalid_argument>());
    EXPECT_THAT([] { Between(5, "2 *"); }, Throws<std::invalid_argument>());
    EXPECT_THAT([] { AtMost("2 *"); }, Throws<std::invalid_argument>());
    EXPECT_THAT([] { AtLeast("2 *"); }, Throws<std::invalid_argument>());
  }
  {
    EXPECT_THAT([] { ExactlyIntegerExpression(""); },
                Throws<std::invalid_argument>());
    EXPECT_THAT([] { OneOfIntegerExpression({""}); },
                Throws<std::invalid_argument>());
    EXPECT_THAT([] { Between("", 5); }, Throws<std::invalid_argument>());
    EXPECT_THAT([] { Between(5, ""); }, Throws<std::invalid_argument>());
    EXPECT_THAT([] { AtMost(""); }, Throws<std::invalid_argument>());
    EXPECT_THAT([] { AtLeast(""); }, Throws<std::invalid_argument>());
  }
}

TEST(NumericConstraintsTest, GetRangeShouldGiveCorrectValues) {
  {
    EXPECT_TRUE(EqualRanges(ExactlyIntegerExpression("3 * 10 + 1").GetRange(),
                            Range(31, 31)));
  }
  {
    EXPECT_TRUE(EqualRanges(Between(10, 20).GetRange(), Range(10, 20)));
    EXPECT_TRUE(EqualRanges(Between("10", 20).GetRange(), Range(10, 20)));
    EXPECT_TRUE(EqualRanges(Between(10, "20").GetRange(), Range(10, 20)));
    EXPECT_TRUE(EqualRanges(Between("10", "20").GetRange(), Range(10, 20)));
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
  {
    OneOfIntegerExpression one_of({"1", "2", "3"});
    EXPECT_THAT(one_of.GetOptions(), ElementsAre("1", "2", "3"));
  }
  {
    OneOfIntegerExpression one_of({"x", "y", "z"});
    EXPECT_THAT(one_of.GetOptions(), ElementsAre("x", "y", "z"));
  }
}

TEST(NumericConstraintsTest, ToStringShouldWork) {
  {
    EXPECT_EQ(ExactlyIntegerExpression("3 * X + 1").ToString(),
              "is exactly 3 * X + 1");
  }
  {
    EXPECT_EQ(OneOfIntegerExpression({"1", "x+3"}).ToString(),
              "is one of {1, x+3}");
    EXPECT_EQ(OneOfIntegerExpression({"x+3"}).ToString(), "is one of {x+3}");
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

TEST(NumericConstraintsTest, IsSatisfiedWithWithNonExpressionWorks) {
  auto tmp = [](std::string_view s) -> int64_t {
    throw std::runtime_error("Should not be called");
  };

  {
    EXPECT_THAT(Between(10, 20).CheckValue(tmp, 10),
                HasNoConstraintViolation());
    EXPECT_THAT(Between(10, 20).CheckValue(tmp, 15),
                HasNoConstraintViolation());
    EXPECT_THAT(Between(10, 20).CheckValue(tmp, 20),
                HasNoConstraintViolation());
    EXPECT_THAT(Between(10, 20).CheckValue(tmp, 9),
                HasConstraintViolation(HasSubstr("between")));
    EXPECT_THAT(Between(10, 20).CheckValue(tmp, 21),
                HasConstraintViolation(HasSubstr("between")));
  }
  {
    EXPECT_THAT(AtLeast(10).CheckValue(tmp, 10), HasNoConstraintViolation());
    EXPECT_THAT(AtLeast(10).CheckValue(tmp, 11), HasNoConstraintViolation());
    EXPECT_THAT(AtLeast(10).CheckValue(tmp, 9),
                HasConstraintViolation(HasSubstr("at least")));
  }
  {
    EXPECT_THAT(AtMost(10).CheckValue(tmp, 10), HasNoConstraintViolation());
    EXPECT_THAT(AtMost(10).CheckValue(tmp, 9), HasNoConstraintViolation());
    EXPECT_THAT(AtMost(10).CheckValue(tmp, 11),
                HasConstraintViolation(HasSubstr("at most")));
  }
}

TEST(NumericConstraintsTest, IsSatisfiedWithWithExpressionWorks) {
  auto tmp = [](std::string_view s) -> int64_t {
    if (s == "x") return 10;
    if (s == "y") return 20;
    throw std::runtime_error("Unknown variable");
  };

  {
    EXPECT_THAT(ExactlyIntegerExpression("x").CheckValue(tmp, 10),
                HasNoConstraintViolation());
    EXPECT_THAT(ExactlyIntegerExpression("x").CheckValue(tmp, 11),
                HasConstraintViolation(HasSubstr("exactly")));
    EXPECT_THAT(ExactlyIntegerExpression("x").CheckValue(tmp, 9),
                HasConstraintViolation(HasSubstr("exactly")));
  }
  {
    EXPECT_THAT(OneOfIntegerExpression({"x", "14"}).CheckValue(tmp, 10),
                HasNoConstraintViolation());
    EXPECT_THAT(OneOfIntegerExpression({"x", "14"}).CheckValue(tmp, 14),
                HasNoConstraintViolation());
    EXPECT_THAT(OneOfIntegerExpression({"x", "14"}).CheckValue(tmp, 9),
                HasConstraintViolation(HasSubstr("one of")));
    EXPECT_THAT(OneOfIntegerExpression({"x", "14"}).CheckValue(tmp, 15),
                HasConstraintViolation(HasSubstr("one of")));
  }
  {
    EXPECT_THAT(Between("x", "y^2").CheckValue(tmp, 10),
                HasNoConstraintViolation());
    EXPECT_THAT(Between("x", "y^2").CheckValue(tmp, 25),
                HasNoConstraintViolation());
    EXPECT_THAT(Between("x", "y^2").CheckValue(tmp, 400),
                HasNoConstraintViolation());
    EXPECT_THAT(Between("x", "y^2").CheckValue(tmp, 9),
                HasConstraintViolation(HasSubstr("between")));
    EXPECT_THAT(Between("x", "y^2").CheckValue(tmp, 401),
                HasConstraintViolation(HasSubstr("between")));
  }
  {
    EXPECT_THAT(AtLeast("x").CheckValue(tmp, 10), HasNoConstraintViolation());
    EXPECT_THAT(AtLeast("x").CheckValue(tmp, 11), HasNoConstraintViolation());
    EXPECT_THAT(AtLeast("x").CheckValue(tmp, 9),
                HasConstraintViolation(HasSubstr("at least")));
  }
  {
    EXPECT_THAT(AtMost("x + 1").CheckValue(tmp, 11),
                HasNoConstraintViolation());
    EXPECT_THAT(AtMost("x + 1").CheckValue(tmp, 10),
                HasNoConstraintViolation());
    EXPECT_THAT(AtMost("x + 1").CheckValue(tmp, 12),
                HasConstraintViolation(HasSubstr("at most")));
  }
}

}  // namespace
}  // namespace moriarty
