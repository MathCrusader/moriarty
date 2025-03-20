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

#include "src/internal/expressions.h"

#include <cstdint>
#include <exception>
#include <format>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace moriarty {
namespace {

using ::testing::HasSubstr;
using ::testing::UnorderedElementsAre;

/* -------------------------------------------------------------------------- */
/*  STRING PARSING                                                            */
/* -------------------------------------------------------------------------- */

MATCHER_P(DoesNotParse, reason, "expression should not be parseable") {
  std::string str(arg);
  try {
    auto x = Expression(str);
  } catch (const std::invalid_argument& e) {
    if (testing::Matches(HasSubstr(reason))(e.what())) return true;
    *result_listener
        << "Expression failed to parse, but didn't have the right reason; "
        << e.what();
    return false;
  }
  *result_listener << "Expression parsed successfully";
  return false;
}

MATCHER_P(EvaluatesTo, value,
          std::format("expression should be parsable and evaluate to {}",
                      value)) {
  std::string str(arg);
  try {
    auto x = Expression(str);  // Only to check for parse issues
  } catch (std::exception& e) {
    *result_listener << "Expression failed to parse; " << e.what();
    return false;
  }

  Expression expr(str);
  int64_t result = expr.Evaluate();
  return testing::ExplainMatchResult(testing::Eq(value), result,
                                     result_listener);
}

MATCHER_P2(EvaluatesToWithVars, value, variables,
           std::format("expression should be parsable and evaluate to {}",
                       value)) {
  std::string str(arg);
  try {
    auto x = Expression(str);  // Only to check for parse issues
  } catch (std::exception& e) {
    *result_listener << "Expression failed to parse; " << e.what();
    return false;
  }

  // Re-casting here to get a nicer compile error
  std::unordered_map<std::string, int64_t> vars(variables);
  auto get_value = [&vars](std::string_view var) -> int64_t {
    return vars.at(std::string(var));
  };
  Expression expr(str);
  int64_t result = expr.Evaluate(get_value);
  return testing::ExplainMatchResult(testing::Eq(value), result,
                                     result_listener);
}

MATCHER_P(EvaluateThrowsInvalid, value,
          "expression should be parsable, but throws an std::invalid_argument "
          "during evaluation") {
  std::string str(arg);
  try {
    auto x = Expression(str);  // Only to check for parse issues
  } catch (std::exception& e) {
    *result_listener << "Expression failed to parse; " << e.what();
    return false;
  }

  auto run = [&]() {
    Expression expr(str);
    (void)expr.Evaluate();
  };
  // .at() throws std::out_of_range if the key is not found.
  return testing::ExplainMatchResult(testing::Throws<std::invalid_argument>(),
                                     run, result_listener);
}

MATCHER_P2(EvaluateThrowsMissingVariable, value, variables,
           "expression should be parsable, but throw an exception due to "
           "missing variable") {
  std::string str(arg);
  try {
    auto x = Expression(str);  // Only to check for parse issues
  } catch (std::exception& e) {
    *result_listener << "Expression failed to parse; " << e.what();
    return false;
  }

  // Re-casting here to get a nicer compile error
  std::unordered_map<std::string, int64_t> vars(variables);
  auto get_value = [&vars](std::string_view var) -> int64_t {
    return vars.at(std::string(var));
  };
  auto run = [&]() {
    Expression expr(str);
    (void)expr.Evaluate(get_value);
  };
  // .at() throws std::out_of_range if the key is not found.
  return testing::ExplainMatchResult(testing::Throws<std::out_of_range>(), run,
                                     result_listener);
}

TEST(ExpressionTest, ToStringShouldWork) {
  EXPECT_EQ(Expression("1 + 2").ToString(), "1 + 2");
  EXPECT_EQ(Expression("1 + 2 + 3").ToString(), "1 + 2 + 3");
  EXPECT_EQ(Expression("1 + 2 * 3").ToString(), "1 + 2 * 3");
  EXPECT_EQ(Expression(" X  ^2 + 5").ToString(), " X  ^2 + 5");
}

TEST(ExpressionsTest, SingleNonnegativeIntegersWork) {
  EXPECT_THAT("0", EvaluatesTo(0));
  EXPECT_THAT("3", EvaluatesTo(3));
  EXPECT_THAT("123456", EvaluatesTo(123456));
  EXPECT_THAT(std::to_string(std::numeric_limits<int64_t>::max()),
              EvaluatesTo(std::numeric_limits<int64_t>::max()));
  EXPECT_THAT("12345678901234567890", DoesNotParse("overflow"));
}

TEST(ExpressionsTest, SingleNegativeIntegersWork) {
  EXPECT_THAT("-0", EvaluatesTo(0));
  EXPECT_THAT("-3", EvaluatesTo(-3));
  EXPECT_THAT("-123456", EvaluatesTo(-123456));
  EXPECT_THAT(std::to_string(-std::numeric_limits<int64_t>::max()),
              EvaluatesTo(-std::numeric_limits<int64_t>::max()));
  EXPECT_THAT("-12345678901234567890", DoesNotParse("overflow"));
  EXPECT_THAT("-12345678901234567890", DoesNotParse("overflow"));
  // Values larger than 2^128 fail slighly differently.
  EXPECT_THAT("-1234567890123456789012345678901234567890",
              DoesNotParse("parse"));

  // For now, we cannot support -2^63 as an exact value. This is due to -x being
  // parsed as -(x), which is definitely needed if we want items like -10^9 to
  // mean -(10^9) and not (-10)^9. Users can type "(-2^62)*2" if they want
  // this value, so we'll leave it for now. This may or may not change
  // in the future.
  EXPECT_THAT("-9223372036854775808", DoesNotParse("overflow"));
  EXPECT_THAT("(-2^62)*2", EvaluatesTo(std::numeric_limits<int64_t>::min()));
}

TEST(ExpressionsTest, ScopeIssuesShouldBeCaught) {
  // Each of these have broken either this implementation or other parsers that
  // I've seen before.
  EXPECT_THAT("", DoesNotParse(""));
  EXPECT_THAT("()", DoesNotParse(""));
  EXPECT_THAT("(,)", DoesNotParse(""));
  EXPECT_THAT("(,,4)", DoesNotParse(""));
  EXPECT_THAT("(4,)", DoesNotParse(""));
  EXPECT_THAT("(())", DoesNotParse(""));
  EXPECT_THAT("((,))", DoesNotParse(""));
  EXPECT_THAT("((,,4))", DoesNotParse(""));
  EXPECT_THAT("((4,))", DoesNotParse(""));
  EXPECT_THAT("1()", DoesNotParse(""));
  EXPECT_THAT("3(*5)", DoesNotParse(""));
  EXPECT_THAT("3(/5)", DoesNotParse(""));
  EXPECT_THAT("3(%5)", DoesNotParse(""));
  EXPECT_THAT("3(+5)", DoesNotParse(""));
  EXPECT_THAT("3(-5)", DoesNotParse(""));
  EXPECT_THAT("3 max(,5)", DoesNotParse(""));
}

TEST(ExpressionsTest, AdditionWorks) {
  EXPECT_THAT("0 + 0", EvaluatesTo(0 + 0));    // 0, 0
  EXPECT_THAT("1 + 0", EvaluatesTo(1 + 0));    // Pos, 0
  EXPECT_THAT("0 + 2", EvaluatesTo(0 + 2));    // 0, Pos
  EXPECT_THAT("-1 + 0", EvaluatesTo(-1 + 0));  // Neg, 0
  EXPECT_THAT("0 + -2", EvaluatesTo(0 + -2));  // 0, Neg

  EXPECT_THAT("5 + 5", EvaluatesTo(5 + 5));  // PosEqual, PosEqual
  EXPECT_THAT("3 + 2", EvaluatesTo(3 + 2));  // PosLarge, PosSmall
  EXPECT_THAT("2 + 4", EvaluatesTo(2 + 4));  // PosSmall, PosLarge

  EXPECT_THAT("-5 + 5", EvaluatesTo(-5 + 5));  // NegEqual, PosEqual
  EXPECT_THAT("-3 + 2", EvaluatesTo(-3 + 2));  // NegLarge, PosSmall
  EXPECT_THAT("-2 + 4", EvaluatesTo(-2 + 4));  // NegSmall, PosLarge

  EXPECT_THAT("5 + -5", EvaluatesTo(5 + -5));  // PosEqual, NegEqual
  EXPECT_THAT("3 + -2", EvaluatesTo(3 + -2));  // PosLarge, NegSmall
  EXPECT_THAT("2 + -4", EvaluatesTo(2 + -4));  // PosSmall, NegLarge

  EXPECT_THAT("-5 + -5", EvaluatesTo(-5 + -5));  // NegEqual, NegEqual
  EXPECT_THAT("-3 + -2", EvaluatesTo(-3 + -2));  // NegLarge, NegSmall
  EXPECT_THAT("-2 + -4", EvaluatesTo(-2 + -4));  // NegSmall, NegLarge
}

TEST(ExpressionsTest, SubtractionWorks) {
  EXPECT_THAT("0 - 0", EvaluatesTo(0 - 0));    // 0, 0
  EXPECT_THAT("1 - 0", EvaluatesTo(1 - 0));    // Pos, 0
  EXPECT_THAT("0 - 2", EvaluatesTo(0 - 2));    // 0, Pos
  EXPECT_THAT("-1 - 0", EvaluatesTo(-1 - 0));  // Neg, 0
  EXPECT_THAT("0 - -2", EvaluatesTo(0 - -2));  // 0, Neg

  EXPECT_THAT("5 - 5", EvaluatesTo(5 - 5));  // PosEqual, PosEqual
  EXPECT_THAT("3 - 2", EvaluatesTo(3 - 2));  // PosLarge, PosSmall
  EXPECT_THAT("2 - 4", EvaluatesTo(2 - 4));  // PosSmall, PosLarge

  EXPECT_THAT("-5 - 5", EvaluatesTo(-5 - 5));  // NegEqual, PosEqual
  EXPECT_THAT("-3 - 2", EvaluatesTo(-3 - 2));  // NegLarge, PosSmall
  EXPECT_THAT("-2 - 4", EvaluatesTo(-2 - 4));  // NegSmall, PosLarge

  EXPECT_THAT("5 - -5", EvaluatesTo(5 - -5));  // PosEqual, NegEqual
  EXPECT_THAT("3 - -2", EvaluatesTo(3 - -2));  // PosLarge, NegSmall
  EXPECT_THAT("2 - -4", EvaluatesTo(2 - -4));  // PosSmall, NegLarge

  EXPECT_THAT("-5 - -5", EvaluatesTo(-5 - -5));  // NegEqual, NegEqual
  EXPECT_THAT("-3 - -2", EvaluatesTo(-3 - -2));  // NegLarge, NegSmall
  EXPECT_THAT("-2 - -4", EvaluatesTo(-2 - -4));  // NegSmall, NegLarge
}

TEST(ExpressionsTest, MultiplicationWorks) {
  EXPECT_THAT("0 * 0", EvaluatesTo(0 * 0));    // 0, 0
  EXPECT_THAT("1 * 0", EvaluatesTo(1 * 0));    // Pos, 0
  EXPECT_THAT("0 * 2", EvaluatesTo(0 * 2));    // 0, Pos
  EXPECT_THAT("-1 * 0", EvaluatesTo(-1 * 0));  // Neg, 0
  EXPECT_THAT("0 * -2", EvaluatesTo(0 * -2));  // 0, Neg

  EXPECT_THAT("5 * 5", EvaluatesTo(5 * 5));  // PosEqual, PosEqual
  EXPECT_THAT("3 * 2", EvaluatesTo(3 * 2));  // PosLarge, PosSmall
  EXPECT_THAT("2 * 4", EvaluatesTo(2 * 4));  // PosSmall, PosLarge

  EXPECT_THAT("-5 * 5", EvaluatesTo(-5 * 5));  // NegEqual, PosEqual
  EXPECT_THAT("-3 * 2", EvaluatesTo(-3 * 2));  // NegLarge, PosSmall
  EXPECT_THAT("-2 * 4", EvaluatesTo(-2 * 4));  // NegSmall, PosLarge

  EXPECT_THAT("5 * -5", EvaluatesTo(5 * -5));  // PosEqual, NegEqual
  EXPECT_THAT("3 * -2", EvaluatesTo(3 * -2));  // PosLarge, NegSmall
  EXPECT_THAT("2 * -4", EvaluatesTo(2 * -4));  // PosSmall, NegLarge

  EXPECT_THAT("-5 * -5", EvaluatesTo(-5 * -5));  // NegEqual, NegEqual
  EXPECT_THAT("-3 * -2", EvaluatesTo(-3 * -2));  // NegLarge, NegSmall
  EXPECT_THAT("-2 * -4", EvaluatesTo(-2 * -4));  // NegSmall, NegLarge
}

TEST(ExpressionsTest, DivisionWorks) {
  EXPECT_THAT("0 / 2", EvaluatesTo(0 / 2));    // 0, Pos
  EXPECT_THAT("0 / -2", EvaluatesTo(0 / -2));  // 0, Neg

  EXPECT_THAT("5 / 5", EvaluatesTo(5 / 5));  // PosEqual, PosEqual
  EXPECT_THAT("3 / 2", EvaluatesTo(3 / 2));  // PosLarge, PosSmall
  EXPECT_THAT("2 / 4", EvaluatesTo(2 / 4));  // PosSmall, PosLarge

  EXPECT_THAT("-5 / 5", EvaluatesTo(-5 / 5));  // NegEqual, PosEqual
  EXPECT_THAT("-3 / 2", EvaluatesTo(-3 / 2));  // NegLarge, PosSmall
  EXPECT_THAT("-2 / 4", EvaluatesTo(-2 / 4));  // NegSmall, PosLarge

  EXPECT_THAT("5 / -5", EvaluatesTo(5 / -5));  // PosEqual, NegEqual
  EXPECT_THAT("3 / -2", EvaluatesTo(3 / -2));  // PosLarge, NegSmall
  EXPECT_THAT("2 / -4", EvaluatesTo(2 / -4));  // PosSmall, NegLarge

  EXPECT_THAT("-5 / -5", EvaluatesTo(-5 / -5));  // NegEqual, NegEqual
  EXPECT_THAT("-3 / -2", EvaluatesTo(-3 / -2));  // NegLarge, NegSmall
  EXPECT_THAT("-2 / -4", EvaluatesTo(-2 / -4));  // NegSmall, NegLarge
}

TEST(ExpressionsTest, ModulusWorks) {
  EXPECT_THAT("0 % 2", EvaluatesTo(0 % 2));    // 0, Pos
  EXPECT_THAT("0 % -2", EvaluatesTo(0 % -2));  // 0, Neg

  EXPECT_THAT("5 % 5", EvaluatesTo(5 % 5));  // PosEqual, PosEqual
  EXPECT_THAT("3 % 2", EvaluatesTo(3 % 2));  // PosLarge, PosSmall
  EXPECT_THAT("2 % 4", EvaluatesTo(2 % 4));  // PosSmall, PosLarge

  EXPECT_THAT("-5 % 5", EvaluatesTo(-5 % 5));  // NegEqual, PosEqual
  EXPECT_THAT("-3 % 2", EvaluatesTo(-3 % 2));  // NegLarge, PosSmall
  EXPECT_THAT("-2 % 4", EvaluatesTo(-2 % 4));  // NegSmall, PosLarge

  EXPECT_THAT("5 % -5", EvaluatesTo(5 % -5));  // PosEqual, NegEqual
  EXPECT_THAT("3 % -2", EvaluatesTo(3 % -2));  // PosLarge, NegSmall
  EXPECT_THAT("2 % -4", EvaluatesTo(2 % -4));  // PosSmall, NegLarge

  EXPECT_THAT("-5 % -5", EvaluatesTo(-5 % -5));  // NegEqual, NegEqual
  EXPECT_THAT("-3 % -2", EvaluatesTo(-3 % -2));  // NegLarge, NegSmall
  EXPECT_THAT("-2 % -4", EvaluatesTo(-2 % -4));  // NegSmall, NegLarge
}

// Basic exponentiation in O(exp)
int64_t pow(int64_t base, int64_t exp) {
  int64_t val = 1;
  while (exp--) val *= base;
  return val;
}

TEST(ExpressionsTest, ExponentiationWorks) {
  EXPECT_THAT("1 ^ 0", EvaluatesTo(pow(1, 0)));      // Pos, 0
  EXPECT_THAT("0 ^ 2", EvaluatesTo(pow(0, 2)));      // 0, Pos
  EXPECT_THAT("-1 ^ 0", EvaluatesTo(-pow(1, 0)));    // Neg, 0
  EXPECT_THAT("(-1) ^ 0", EvaluatesTo(pow(-1, 0)));  // Neg, 0

  // Bracketted vs not. -1 ^ 2 == -(1 ^ 2) to maintain parity with Python.
  EXPECT_THAT("(-1) ^ 2", EvaluatesTo(pow(-1, 2)));  // Neg, even
  EXPECT_THAT("-1 ^ 2", EvaluatesTo(-pow(1, 2)));    // Neg, even

  EXPECT_THAT("5 ^ 5", EvaluatesTo(pow(5, 5)));  // PosEqual, PosEqual
  EXPECT_THAT("3 ^ 2", EvaluatesTo(pow(3, 2)));  // PosLarge, PosSmall
  EXPECT_THAT("2 ^ 4", EvaluatesTo(pow(2, 4)));  // PosSmall, PosLarge

  EXPECT_THAT("-5 ^ 5", EvaluatesTo(-pow(5, 5)));  // NegEqual, PosEqual
  EXPECT_THAT("-3 ^ 2", EvaluatesTo(-pow(3, 2)));  // NegLarge, PosSmall
  EXPECT_THAT("-2 ^ 4", EvaluatesTo(-pow(2, 4)));  // NegSmall, PosLarge

  EXPECT_THAT("(-5) ^ 5", EvaluatesTo(pow(-5, 5)));  // NegEqual, PosEqual
  EXPECT_THAT("(-3) ^ 2", EvaluatesTo(pow(-3, 2)));  // NegLarge, PosSmall
  EXPECT_THAT("(-2) ^ 4", EvaluatesTo(pow(-2, 4)));  // NegSmall, PosLarge

  // Huge exponentiation for 0, 1, -1 is okay and fast.
  EXPECT_THAT("0 ^ 123456789012345678", EvaluatesTo(0));
  EXPECT_THAT("1 ^ 123456789012345678", EvaluatesTo(1));
  EXPECT_THAT("(-1) ^ 123456789012345678", EvaluatesTo(1));
  EXPECT_THAT("(-1) ^ 123456789012345677", EvaluatesTo(-1));

  // Bad cases
  EXPECT_THAT("10 ^ (-5)", EvaluateThrowsInvalid("non-negative"));
  EXPECT_THAT("0^0", EvaluateThrowsInvalid("undefined"));
}

TEST(ExpressionsTest, OrderOfOperationsAndAssociativityBehaves) {
  EXPECT_THAT("3 + 7 + 5", EvaluatesTo(3 + 7 + 5));
  EXPECT_THAT("3 + 7 - 5", EvaluatesTo(3 + 7 - 5));
  EXPECT_THAT("3 + 7 * 5", EvaluatesTo(3 + 7 * 5));
  EXPECT_THAT("3 + 7 / 5", EvaluatesTo(3 + 7 / 5));
  EXPECT_THAT("3 + 7 % 5", EvaluatesTo(3 + 7 % 5));
  EXPECT_THAT("3 + 7 ^ 5", EvaluatesTo(3 + pow(7, 5)));

  EXPECT_THAT("3 - 7 + 5", EvaluatesTo(3 - 7 + 5));
  EXPECT_THAT("3 - 7 - 5", EvaluatesTo(3 - 7 - 5));
  EXPECT_THAT("3 - 7 * 5", EvaluatesTo(3 - 7 * 5));
  EXPECT_THAT("3 - 7 / 5", EvaluatesTo(3 - 7 / 5));
  EXPECT_THAT("3 - 7 % 5", EvaluatesTo(3 - 7 % 5));
  EXPECT_THAT("3 - 7 ^ 5", EvaluatesTo(3 - pow(7, 5)));

  EXPECT_THAT("3 * 7 + 5", EvaluatesTo(3 * 7 + 5));
  EXPECT_THAT("3 * 7 - 5", EvaluatesTo(3 * 7 - 5));
  EXPECT_THAT("3 * 7 * 5", EvaluatesTo(3 * 7 * 5));
  EXPECT_THAT("3 * 7 / 5", EvaluatesTo(3 * 7 / 5));
  EXPECT_THAT("3 * 7 % 5", EvaluatesTo(3 * 7 % 5));
  EXPECT_THAT("3 * 7 ^ 5", EvaluatesTo(3 * pow(7, 5)));

  EXPECT_THAT("123456789 / 7 + 5", EvaluatesTo(123456789 / 7 + 5));
  EXPECT_THAT("123456789 / 7 - 5", EvaluatesTo(123456789 / 7 - 5));
  EXPECT_THAT("123456789 / 7 * 5", EvaluatesTo(123456789 / 7 * 5));
  EXPECT_THAT("123456789 / 7 / 5", EvaluatesTo(123456789 / 7 / 5));
  EXPECT_THAT("123456789 / 7 % 5", EvaluatesTo(123456789 / 7 % 5));
  EXPECT_THAT("123456789 / 7 ^ 5", EvaluatesTo(123456789 / pow(7, 5)));

  EXPECT_THAT("123456789 % 7 + 5", EvaluatesTo(123456789 % 7 + 5));
  EXPECT_THAT("123456789 % 7 - 5", EvaluatesTo(123456789 % 7 - 5));
  EXPECT_THAT("123456789 % 7 * 5", EvaluatesTo(123456789 % 7 * 5));
  EXPECT_THAT("123456789 % 7 / 5", EvaluatesTo(123456789 % 7 / 5));
  EXPECT_THAT("123456789 % 7 % 5", EvaluatesTo(123456789 % 7 % 5));
  EXPECT_THAT("123456789 % 7 ^ 5", EvaluatesTo(123456789 % pow(7, 5)));

  EXPECT_THAT("3 ^ 7 + 5", EvaluatesTo(pow(3, 7) + 5));
  EXPECT_THAT("3 ^ 7 - 5", EvaluatesTo(pow(3, 7) - 5));
  EXPECT_THAT("3 ^ 7 * 5", EvaluatesTo(pow(3, 7) * 5));
  EXPECT_THAT("3 ^ 7 / 5", EvaluatesTo(pow(3, 7) / 5));
  EXPECT_THAT("3 ^ 7 % 5", EvaluatesTo(pow(3, 7) % 5));
  EXPECT_THAT("4 ^ 3 ^ 2", EvaluatesTo(pow(4, pow(3, 2))));
}

TEST(ExpressionsTest, ParenthesesOverrideOrderOfOperations) {
  EXPECT_THAT("(3 + 7) + 5", EvaluatesTo((3 + 7) + 5));
  EXPECT_THAT("(3 + 7) - 5", EvaluatesTo((3 + 7) - 5));
  EXPECT_THAT("(3 + 7) * 5", EvaluatesTo((3 + 7) * 5));
  EXPECT_THAT("(3 + 7) / 5", EvaluatesTo((3 + 7) / 5));
  EXPECT_THAT("(3 + 7) % 5", EvaluatesTo((3 + 7) % 5));
  EXPECT_THAT("(3 + 7) ^ 5", EvaluatesTo(pow(3 + 7, 5)));
  EXPECT_THAT("((3 + 7)) + 5", EvaluatesTo((3 + 7) + 5));

  EXPECT_THAT("(3 - 7) + 5", EvaluatesTo((3 - 7) + 5));
  EXPECT_THAT("(3 - 7) - 5", EvaluatesTo((3 - 7) - 5));
  EXPECT_THAT("(3 - 7) * 5", EvaluatesTo((3 - 7) * 5));
  EXPECT_THAT("(3 - 7) / 5", EvaluatesTo((3 - 7) / 5));
  EXPECT_THAT("(3 - 7) % 5", EvaluatesTo((3 - 7) % 5));
  EXPECT_THAT("(3 - 7) ^ 5", EvaluatesTo(pow(3 - 7, 5)));

  EXPECT_THAT("(3 * 7) + 5", EvaluatesTo((3 * 7) + 5));
  EXPECT_THAT("(3 * 7) - 5", EvaluatesTo((3 * 7) - 5));
  EXPECT_THAT("(3 * 7) * 5", EvaluatesTo((3 * 7) * 5));
  EXPECT_THAT("(3 * 7) / 5", EvaluatesTo((3 * 7) / 5));
  EXPECT_THAT("(3 * 7) % 5", EvaluatesTo((3 * 7) % 5));
  EXPECT_THAT("(3 * 7) ^ 5", EvaluatesTo(pow(3 * 7, 5)));

  EXPECT_THAT("(123456789 / 7) + 5", EvaluatesTo((123456789 / 7) + 5));
  EXPECT_THAT("(123456789 / 7) - 5", EvaluatesTo((123456789 / 7) - 5));
  EXPECT_THAT("(123456789 / 7) * 5", EvaluatesTo((123456789 / 7) * 5));
  EXPECT_THAT("(123456789 / 7) / 5", EvaluatesTo((123456789 / 7) / 5));
  EXPECT_THAT("(123456789 / 7) % 5", EvaluatesTo((123456789 / 7) % 5));
  EXPECT_THAT("(123 / 7) ^ 5", EvaluatesTo(pow(123 / 7, 5)));

  EXPECT_THAT("(123456789 % 7) + 5", EvaluatesTo((123456789 % 7) + 5));
  EXPECT_THAT("(123456789 % 7) - 5", EvaluatesTo((123456789 % 7) - 5));
  EXPECT_THAT("(123456789 % 7) * 5", EvaluatesTo((123456789 % 7) * 5));
  EXPECT_THAT("(123456789 % 7) / 5", EvaluatesTo((123456789 % 7) / 5));
  EXPECT_THAT("(123456789 % 7) % 5", EvaluatesTo((123456789 % 7) % 5));
  EXPECT_THAT("(123456789 % 7) ^ 5", EvaluatesTo(pow(123456789 % 7, 5)));

  EXPECT_THAT("(3 ^ 7) + 5", EvaluatesTo(pow(3, 7) + 5));
  EXPECT_THAT("(3 ^ 7) - 5", EvaluatesTo(pow(3, 7) - 5));
  EXPECT_THAT("(3 ^ 7) * 5", EvaluatesTo(pow(3, 7) * 5));
  EXPECT_THAT("(3 ^ 7) / 5", EvaluatesTo(pow(3, 7) / 5));
  EXPECT_THAT("(3 ^ 7) % 5", EvaluatesTo(pow(3, 7) % 5));
  EXPECT_THAT("((4 ^ 3) ^ 2)", EvaluatesTo(pow(pow(4, 3), 2)));

  EXPECT_THAT("3 + (7 + 5)", EvaluatesTo(3 + (7 + 5)));
  EXPECT_THAT("3 + (7 - 5)", EvaluatesTo(3 + (7 - 5)));
  EXPECT_THAT("3 + (7 * 5)", EvaluatesTo(3 + (7 * 5)));
  EXPECT_THAT("3 + (7 / 5)", EvaluatesTo(3 + (7 / 5)));
  EXPECT_THAT("3 + (7 % 5)", EvaluatesTo(3 + (7 % 5)));
  EXPECT_THAT("3 + (7 ^ 5)", EvaluatesTo(3 + pow(7, 5)));

  EXPECT_THAT("3 - (7 + 5)", EvaluatesTo(3 - (7 + 5)));
  EXPECT_THAT("3 - (7 - 5)", EvaluatesTo(3 - (7 - 5)));
  EXPECT_THAT("3 - (7 * 5)", EvaluatesTo(3 - (7 * 5)));
  EXPECT_THAT("3 - (7 / 5)", EvaluatesTo(3 - (7 / 5)));
  EXPECT_THAT("3 - (7 % 5)", EvaluatesTo(3 - (7 % 5)));
  EXPECT_THAT("3 - (7 ^ 5)", EvaluatesTo(3 - pow(7, 5)));

  EXPECT_THAT("3 * (7 + 5)", EvaluatesTo(3 * (7 + 5)));
  EXPECT_THAT("3 * (7 - 5)", EvaluatesTo(3 * (7 - 5)));
  EXPECT_THAT("3 * (7 * 5)", EvaluatesTo(3 * (7 * 5)));
  EXPECT_THAT("3 * (7 / 5)", EvaluatesTo(3 * (7 / 5)));
  EXPECT_THAT("3 * (7 % 5)", EvaluatesTo(3 * (7 % 5)));
  EXPECT_THAT("3 * (7 ^ 5)", EvaluatesTo(3 * pow(7, 5)));

  EXPECT_THAT("123456789 / (7 + 5)", EvaluatesTo(123456789 / (7 + 5)));
  EXPECT_THAT("123456789 / (7 - 5)", EvaluatesTo(123456789 / (7 - 5)));
  EXPECT_THAT("123456789 / (7 * 5)", EvaluatesTo(123456789 / (7 * 5)));
  EXPECT_THAT("123456789 / (7 / 5)", EvaluatesTo(123456789 / (7 / 5)));
  EXPECT_THAT("123456789 / (7 % 5)", EvaluatesTo(123456789 / (7 % 5)));
  EXPECT_THAT("123456789 / (7 ^ 5)", EvaluatesTo(123456789 / pow(7, 5)));

  EXPECT_THAT("123456789 % (7 + 5)", EvaluatesTo(123456789 % (7 + 5)));
  EXPECT_THAT("123456789 % (7 - 5)", EvaluatesTo(123456789 % (7 - 5)));
  EXPECT_THAT("123456789 % (7 * 5)", EvaluatesTo(123456789 % (7 * 5)));
  EXPECT_THAT("123456789 % (7 / 5)", EvaluatesTo(123456789 % (7 / 5)));
  EXPECT_THAT("123456789 % (7 % 5)", EvaluatesTo(123456789 % (7 % 5)));
  EXPECT_THAT("123456789 % (7 ^ 5)", EvaluatesTo(123456789 % pow(7, 5)));

  EXPECT_THAT("3 ^ (7 + 5)", EvaluatesTo(pow(3, 7 + 5)));
  EXPECT_THAT("3 ^ (7 - 5)", EvaluatesTo(pow(3, 7 - 5)));
  EXPECT_THAT("3 ^ (7 * 5)", EvaluatesTo(pow(3, 7 * 5)));
  EXPECT_THAT("3 ^ (7 / 5)", EvaluatesTo(pow(3, 7 / 5)));
  EXPECT_THAT("3 ^ (7 % 5)", EvaluatesTo(pow(3, 7 % 5)));
  EXPECT_THAT("(4 ^ 3) ^ 2", EvaluatesTo(pow(pow(4, 3), 2)));
}

TEST(ExpressionsTest, NestedAndSideBySideParenthesesWork) {
  EXPECT_THAT("(2 + 3) * (4 + 5)", EvaluatesTo((2 + 3) * (4 + 5)));
  EXPECT_THAT("((2 + 3) * (4 + 5))", EvaluatesTo((2 + 3) * (4 + 5)));
  EXPECT_THAT("((((((2 + 3))))))", EvaluatesTo(2 + 3));

  EXPECT_THAT("(+2 + 3) * (4 + -5)", EvaluatesTo((+2 + 3) * (4 + -5)));
  EXPECT_THAT("((+2 + 3) * (-4 + 5))", EvaluatesTo((+2 + 3) * (-4 + 5)));
  EXPECT_THAT("((((((+2 + +3))))))", EvaluatesTo(+2 + +3));
}

TEST(ExpressionsTest, ImproperNesting) {
  EXPECT_THAT("(1", DoesNotParse("'('"));
  EXPECT_THAT("((1)", DoesNotParse("'("));
  EXPECT_THAT("1)", DoesNotParse("')'"));
  EXPECT_THAT("(1))", DoesNotParse("')'"));
}

TEST(ExpressionsTest, BackToBackItems) {
  // No good messages to check for here
  EXPECT_THAT("1 1", DoesNotParse(""));
  EXPECT_THAT("N N", DoesNotParse(""));
  EXPECT_THAT("1 + N 1 + 3", DoesNotParse(""));
}

TEST(ExpressionsTest, BinaryOperatorsWithoutTwoArguments) {
  EXPECT_THAT("1 *", DoesNotParse("binary"));
  EXPECT_THAT("1 * ) * 1", DoesNotParse("binary"));
  EXPECT_THAT("* 3 + 4", DoesNotParse("binary"));
}

TEST(ExpressionsTest, UnaryOperatorsWithoutAnArgument) {
  EXPECT_THAT("-", DoesNotParse("unary"));
  EXPECT_THAT("4 + (-)", DoesNotParse(""));  // No good message
}

TEST(ExpressionsTest, UnaryPlusAndMinusAreIdentified) {
  EXPECT_THAT("3  + (-10 + 1)", EvaluatesTo(3 + (-10 + 1)));
  EXPECT_THAT("-(4 * -4)", EvaluatesTo(-(4 * -4)));
  EXPECT_THAT("--42",
              DoesNotParse("unary operator after another unary operator"));
  EXPECT_THAT("-+42",
              DoesNotParse("unary operator after another unary operator"));
}

TEST(ExpressionsTest, WhitespaceShouldBeIgnored) {
  EXPECT_THAT(" 3 + 1", EvaluatesTo(3 + 1));
  EXPECT_THAT("3 + 1 ", EvaluatesTo(3 + 1));
  EXPECT_THAT(" 3 + 1 ", EvaluatesTo(3 + 1));
  EXPECT_THAT(" (  3   + - 1 - 1 ) * 4 ", EvaluatesTo((3 + -1 - 1) * 4));
}

TEST(ExpressionsTest, InvalidCharacter) {
  EXPECT_THAT("~", DoesNotParse("Unknown character"));
  EXPECT_THAT("3 + &", DoesNotParse("Unknown character"));
}

/* -------------------------------------------------------------------------- */
/*  STRING PARSING WITH VARIABLES                                             */
/* -------------------------------------------------------------------------- */

TEST(ExpressionsTest, SingleVariableParsesProperly) {
  using Map = std::unordered_map<std::string, int64_t>;

  EXPECT_THAT("N", EvaluatesToWithVars(2, Map{{"N", 2}}));
  EXPECT_THAT("multiple_letters_4",
              EvaluatesToWithVars(2, Map{{"multiple_letters_4", 2}}));
  EXPECT_THAT("3 * N + 1", EvaluatesToWithVars(7, Map{{"N", 2}}));
  EXPECT_THAT("1 + 3 * N", EvaluatesToWithVars(7, Map{{"N", 2}}));
  EXPECT_THAT("N ^ N", EvaluatesToWithVars(pow(4, 4), Map{{"N", 4}}));
}

TEST(ExpressionsTest, MultipleVariableParsesProperly) {
  using Map = std::unordered_map<std::string, int64_t>;

  EXPECT_THAT("X + Y", EvaluatesToWithVars(6, Map{{"X", 2}, {"Y", 4}}));
  EXPECT_THAT("X * Y + 1", EvaluatesToWithVars(9, Map{{"X", 2}, {"Y", 4}}));
  EXPECT_THAT("Y + Y / X", EvaluatesToWithVars(6, Map{{"X", 2}, {"Y", 4}}));
  EXPECT_THAT("X ^ Y", EvaluatesToWithVars(pow(3, 4), Map{{"X", 3}, {"Y", 4}}));
  EXPECT_THAT(
      "X + X + Y * Y + X * Y",
      EvaluatesToWithVars(3 + 3 + 4 * 4 + 3 * 4, Map{{"X", 3}, {"Y", 4}}));
}

TEST(ExpressionsTest, MissingVariablesFails) {
  using Map = std::unordered_map<std::string, int64_t>;

  EXPECT_THAT("X", EvaluateThrowsMissingVariable("", Map{}));
  EXPECT_THAT("X + -Y", EvaluateThrowsMissingVariable("", Map{}));
  EXPECT_THAT("X + Y", EvaluateThrowsMissingVariable("", Map{{"Y", 33}}));
  EXPECT_THAT("-X + X + Y", EvaluateThrowsMissingVariable("", Map{{"Y", 33}}));
}

TEST(ExpressionsTest, DependenciesShouldWork) {
  auto needed_vars =
      [](std::string_view expression) -> std::vector<std::string> {
    Expression expr(expression);
    return expr.GetDependencies();
  };

  EXPECT_THAT(needed_vars("3 + 1"), UnorderedElementsAre());
  EXPECT_THAT(needed_vars("N"), UnorderedElementsAre("N"));
  EXPECT_THAT(needed_vars("3 * N + 1"), UnorderedElementsAre("N"));
  EXPECT_THAT(needed_vars("X * Y + 5"), UnorderedElementsAre("X", "Y"));
  EXPECT_THAT(needed_vars("multiple_letters_4 * N + 1"),
              UnorderedElementsAre("multiple_letters_4", "N"));
  EXPECT_THAT(needed_vars("multiple_letters_4 * N + 1"),
              UnorderedElementsAre("multiple_letters_4", "N"));
  EXPECT_THAT(needed_vars("X * Y + X"), UnorderedElementsAre("X", "Y"));
}

/* -------------------------------------------------------------------------- */
/*  STRING PARSING WITH FUNCTIONS                                             */
/* -------------------------------------------------------------------------- */

TEST(ExpressionsTest, FunctionsWork) {
  EXPECT_THAT("min(3, 5)", EvaluatesTo(3));
  EXPECT_THAT("min(1, 2, -3, 0)", EvaluatesTo(-3));
  EXPECT_THAT("min(max(5, 20, 10), max(10, 11, 12))", EvaluatesTo(12));
  EXPECT_THAT("min(3)", EvaluatesTo(3));
  EXPECT_THAT("abs(-3)", EvaluatesTo(3));
  EXPECT_THAT("min(max(5, abs(-20), 10), max(10, 11, abs(-12)))",
              EvaluatesTo(12));
  EXPECT_THAT("min(abs(-5), 10)", EvaluatesTo(5));
}

TEST(ExpressionsTest, InvalidFunctionArgumentsShouldFail) {
  EXPECT_THAT("(1, 2, 3)", DoesNotParse("paren"));      // No name
  EXPECT_THAT("f()", DoesNotParse("tokens to parse"));  // Unknown name
  EXPECT_THAT("fake(3, 4)", DoesNotParse("Unknown function"));

  EXPECT_THAT("abs()", DoesNotParse("tokens to parse"));
  EXPECT_THAT("abs(3, 4)", DoesNotParse("argument"));
}

TEST(ExpressionsTest, InvalidCommasInArgumentShouldFail) {
  EXPECT_THAT("f((1, 2))", DoesNotParse(""));
  EXPECT_THAT("f(1, (2, 3))", DoesNotParse(""));
  EXPECT_THAT("f(1, , 2)", DoesNotParse(""));
  EXPECT_THAT("f(", DoesNotParse(""));
  EXPECT_THAT("f( , 1)", DoesNotParse(""));
  EXPECT_THAT("f(1, )", DoesNotParse(""));
  EXPECT_THAT("(1, 2)", DoesNotParse(""));
  EXPECT_THAT("1 + (1, 2)", DoesNotParse(""));
  EXPECT_THAT("(1, 2) + 3", DoesNotParse(""));

  // This may see that there are 3 arguments to g, then assume they are "g",
  // "x", "y", then pop "f" off the stack instead of "g".
  EXPECT_THAT("f(, g(x,y,)", DoesNotParse(""));
  EXPECT_THAT("1(g(,2,3)", DoesNotParse(""));
}

TEST(ExpressionsTest, FunctionsAndVariablesMixWell) {
  using Map = std::unordered_map<std::string, int64_t>;
  EXPECT_THAT("min(3, N)", EvaluatesToWithVars(2, Map{{"N", 2}}));
  EXPECT_THAT("min(3, N)", EvaluatesToWithVars(3, Map{{"N", 3}}));
  EXPECT_THAT("min(3, N)", EvaluatesToWithVars(3, Map{{"N", 4}}));
  EXPECT_THAT("min(M, N)", EvaluatesToWithVars(2, Map{{"M", 3}, {"N", 2}}));

  // FunctionsAndVariablesMayHaveTheSameName
  EXPECT_THAT("min(min, 3)", EvaluatesToWithVars(2, Map{{"min", 2}}));
  EXPECT_THAT("min(min, max)",
              EvaluatesToWithVars(3, Map{{"min", 3}, {"max", 5}}));
}

}  // namespace
}  // namespace moriarty
