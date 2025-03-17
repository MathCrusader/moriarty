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

namespace moriarty {
namespace {

using ::moriarty::AtLeast;
using ::moriarty::AtMost;
using ::moriarty::Between;
using ::testing::ElementsAre;
using ::testing::Throws;

testing::AssertionResult EqualRanges(const Range& r1, const Range& r2) {
  auto extremes1 = r1.Extremes();
  auto extremes2 = r2.Extremes();
  if (!extremes1.ok())
    return testing::AssertionFailure()
           << "r1.Extremes() failed; " << extremes1.status();
  if (!extremes2.ok())
    return testing::AssertionFailure()
           << "r2.Extremes() failed; " << extremes2.status();

  if (!(*extremes1).has_value() && !(*extremes2).has_value())
    return testing::AssertionSuccess() << "both ranges are empty";

  if (!(*extremes1).has_value())
    return testing::AssertionFailure()
           << "first range is empty, second is " << "[" << (*extremes2)->min
           << ", " << (*extremes2)->max << "]";

  if (!(*extremes2).has_value())
    return testing::AssertionFailure()
           << "first range is " << "[" << (*extremes1)->min << ", "
           << (*extremes1)->max << "]" << ", second is empty";

  if ((*extremes1) == (*extremes2))
    return testing::AssertionSuccess()
           << "both ranges are [" << (*extremes1)->min << ", "
           << (*extremes1)->max;

  return testing::AssertionFailure()
         << "are not equal " << "[" << (*extremes1)->min << ", "
         << (*extremes1)->max << "]" << " vs " << "[" << (*extremes2)->min
         << ", " << (*extremes2)->max << "]";
}

TEST(NumericConstraintsTest, InvalidBetweenShouldThrow) {
  EXPECT_THAT([] { Between(0, -5); }, Throws<std::invalid_argument>());
}

TEST(NumericConstraintsTest, InvalidExpressionsShouldThrow) {
  {
    EXPECT_DEATH({ ExactlyIntegerExpression("2 *"); }, "INVALID_ARGUMENT");
    EXPECT_THAT([] { OneOfIntegerExpression({"3", "2 *"}); },
                Throws<std::invalid_argument>());
    EXPECT_DEATH({ Between("2 *", 5); }, "INVALID_ARGUMENT");
    EXPECT_DEATH({ Between(5, "2 *"); }, "INVALID_ARGUMENT");
    EXPECT_DEATH({ AtMost("2 *"); }, "INVALID_ARGUMENT");
    EXPECT_DEATH({ AtLeast("2 *"); }, "INVALID_ARGUMENT");
  }
  {
    EXPECT_DEATH({ ExactlyIntegerExpression(""); }, "INVALID_ARGUMENT");
    EXPECT_THAT([] { OneOfIntegerExpression({""}); },
                Throws<std::invalid_argument>());
    EXPECT_DEATH({ Between("", 5); }, "INVALID_ARGUMENT");
    EXPECT_DEATH({ Between(5, ""); }, "INVALID_ARGUMENT");
    EXPECT_DEATH({ AtMost(""); }, "INVALID_ARGUMENT");
    EXPECT_DEATH({ AtLeast(""); }, "INVALID_ARGUMENT");
  }

  // {
  //   EXPECT_THAT([] { ExactlyIntegerExpression("2 *"); },
  //               Throws<std::invalid_argument>());
  //   EXPECT_THAT([] { OneOfIntegerExpression({"3", "2 *"}); },
  //               Throws<std::invalid_argument>());
  //   EXPECT_THAT([] { Between("2 *", 5); }, Throws<std::invalid_argument>());
  //   EXPECT_THAT([] { Between(5, "2 *"); }, Throws<std::invalid_argument>());
  //   EXPECT_THAT([] { AtMost("2 *"); }, Throws<std::invalid_argument>());
  //   EXPECT_THAT([] { AtLeast("2 *"); }, Throws<std::invalid_argument>());
  // }
  // {
  //   EXPECT_THAT([] { ExactlyIntegerExpression(""); },
  //               Throws<std::invalid_argument>());
  //   EXPECT_THAT([] { OneOfIntegerExpression({""}); },
  //               Throws<std::invalid_argument>());
  //   EXPECT_THAT([] { Between("", 5); }, Throws<std::invalid_argument>());
  //   EXPECT_THAT([] { Between(5, ""); }, Throws<std::invalid_argument>());
  //   EXPECT_THAT([] { AtMost(""); }, Throws<std::invalid_argument>());
  //   EXPECT_THAT([] { AtLeast(""); }, Throws<std::invalid_argument>());
  // }
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
    EXPECT_TRUE(Between(10, 20).IsSatisfiedWith(tmp, 10));
    EXPECT_TRUE(Between(10, 20).IsSatisfiedWith(tmp, 15));
    EXPECT_TRUE(Between(10, 20).IsSatisfiedWith(tmp, 20));
    EXPECT_FALSE(Between(10, 20).IsSatisfiedWith(tmp, 9));
    EXPECT_FALSE(Between(10, 20).IsSatisfiedWith(tmp, 21));
  }
  {
    EXPECT_TRUE(AtLeast(10).IsSatisfiedWith(tmp, 10));
    EXPECT_TRUE(AtLeast(10).IsSatisfiedWith(tmp, 11));
    EXPECT_FALSE(AtLeast(10).IsSatisfiedWith(tmp, 9));
  }
  {
    EXPECT_TRUE(AtMost(10).IsSatisfiedWith(tmp, 10));
    EXPECT_TRUE(AtMost(10).IsSatisfiedWith(tmp, 9));
    EXPECT_FALSE(AtMost(10).IsSatisfiedWith(tmp, 11));
  }
}

TEST(NumericConstraintsTest, IsSatisfiedWithWithExpressionWorks) {
  auto tmp = [](std::string_view s) -> int64_t {
    if (s == "x") return 10;
    if (s == "y") return 20;
    throw std::runtime_error("Unknown variable");
  };

  {
    EXPECT_TRUE(ExactlyIntegerExpression("x").IsSatisfiedWith(tmp, 10));
    EXPECT_FALSE(ExactlyIntegerExpression("x").IsSatisfiedWith(tmp, 11));
    EXPECT_FALSE(ExactlyIntegerExpression("x").IsSatisfiedWith(tmp, 9));
  }
  {
    EXPECT_TRUE(OneOfIntegerExpression({"x", "14"}).IsSatisfiedWith(tmp, 10));
    EXPECT_TRUE(OneOfIntegerExpression({"x", "14"}).IsSatisfiedWith(tmp, 14));
    EXPECT_FALSE(OneOfIntegerExpression({"x", "14"}).IsSatisfiedWith(tmp, 9));
    EXPECT_FALSE(OneOfIntegerExpression({"x", "14"}).IsSatisfiedWith(tmp, 15));
  }
  {
    EXPECT_TRUE(Between("x", "y^2").IsSatisfiedWith(tmp, 10));
    EXPECT_TRUE(Between("x", "y^2").IsSatisfiedWith(tmp, 25));
    EXPECT_TRUE(Between("x", "y^2").IsSatisfiedWith(tmp, 400));
    EXPECT_FALSE(Between("x", "y^2").IsSatisfiedWith(tmp, 9));
    EXPECT_FALSE(Between("x", "y^2").IsSatisfiedWith(tmp, 401));
  }
  {
    EXPECT_TRUE(AtLeast("x").IsSatisfiedWith(tmp, 10));
    EXPECT_TRUE(AtLeast("x").IsSatisfiedWith(tmp, 11));
    EXPECT_FALSE(AtLeast("x").IsSatisfiedWith(tmp, 9));
  }
  {
    EXPECT_TRUE(AtMost("x + 1").IsSatisfiedWith(tmp, 11));
    EXPECT_TRUE(AtMost("x + 1").IsSatisfiedWith(tmp, 10));
    EXPECT_FALSE(AtMost("x + 1").IsSatisfiedWith(tmp, 12));
  }
}

TEST(NumericConstraintsTest, ExplanationShouldWork) {
  auto tmp = [](std::string_view s) -> int64_t {
    throw std::runtime_error("Should not be called");
  };

  {
    EXPECT_EQ(ExactlyIntegerExpression("x + 1").Explanation(tmp, 10),
              "is not exactly x + 1");
  }
  {
    EXPECT_EQ(OneOfIntegerExpression({"x", "14"}).Explanation(tmp, 10),
              "is not one of {x, 14}");
    EXPECT_EQ(OneOfIntegerExpression({"x", "14"}).Explanation(tmp, 15),
              "is not one of {x, 14}");
  }
  {
    EXPECT_EQ(Between("x", 12).Explanation(tmp, 9), "is not between x and 12");
    EXPECT_EQ(Between(4, "y^2").Explanation(tmp, 401),
              "is not between 4 and y^2");
    EXPECT_EQ(Between("x", "y^2").Explanation(tmp, 401),
              "is not between x and y^2");
    EXPECT_EQ(Between(18, 20).Explanation(tmp, 401),
              "is not between 18 and 20");
  }
  {
    EXPECT_EQ(AtLeast("x").Explanation(tmp, 10), "is not at least x");
    EXPECT_EQ(AtLeast(100).Explanation(tmp, 11), "is not at least 100");
  }
  {
    EXPECT_EQ(AtMost("x + 1").Explanation(tmp, 11), "is not at most x + 1");
    EXPECT_EQ(AtMost(5).Explanation(tmp, 10), "is not at most 5");
  }
}

}  // namespace
}  // namespace moriarty
