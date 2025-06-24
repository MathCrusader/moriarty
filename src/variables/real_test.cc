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

#include "src/variables/real.h"

#include <compare>
#include <limits>
#include <ostream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace std {

std::ostream& operator<<(std::ostream& os, std::partial_ordering p) {
  if (p == std::partial_ordering::less) {
    return os << "<";
  } else if (p == std::partial_ordering::greater) {
    return os << ">";
  } else if (p == std::partial_ordering::equivalent) {
    return os << "=";
  } else {
    return os << "unordered";
  }
}

}  // namespace std

namespace moriarty {
namespace {

using ::testing::FieldsAre;

TEST(RealTest, ConstructorFromInt64) {
  EXPECT_THAT(Real(42).GetValue(), FieldsAre(42, 1));
  EXPECT_THAT(Real(-42).GetValue(), FieldsAre(-42, 1));
  EXPECT_THAT(Real(0).GetValue(), FieldsAre(0, 1));

  EXPECT_THAT(Real(int64_t{42}).GetValue(), FieldsAre(42, 1));
  EXPECT_THAT(Real(int64_t{-42}).GetValue(), FieldsAre(-42, 1));
  EXPECT_THAT(Real(int64_t{0}).GetValue(), FieldsAre(0, 1));

  EXPECT_THAT(Real(std::numeric_limits<int64_t>::max()).GetValue(),
              FieldsAre(std::numeric_limits<int64_t>::max(), 1));
  EXPECT_THAT(Real(std::numeric_limits<int64_t>::min()).GetValue(),
              FieldsAre(std::numeric_limits<int64_t>::min(), 1));
}

TEST(RealTest, ConstructorFromFraction) {
  EXPECT_THAT(Real(42, 2).GetValue(), FieldsAre(21, 1));
  EXPECT_THAT(Real(42, -1).GetValue(), FieldsAre(-42, 1));
  EXPECT_THAT(Real(0, -10).GetValue(), FieldsAre(0, 1));

  const int64_t kMax = std::numeric_limits<int64_t>::max();
  const int64_t kMin = std::numeric_limits<int64_t>::min();
  EXPECT_THAT(Real(kMax, 1).GetValue(), FieldsAre(kMax, 1));
  EXPECT_THAT(Real(1, kMax).GetValue(), FieldsAre(1, kMax));
  EXPECT_THAT(Real(kMax, kMax).GetValue(), FieldsAre(1, 1));
  EXPECT_THAT(Real(kMin, kMax).GetValue(), FieldsAre(kMin, kMax));
  EXPECT_THAT(Real(4, kMin).GetValue(), FieldsAre(-1, int64_t{1} << 61));
  EXPECT_THAT(Real(-4, kMin).GetValue(), FieldsAre(1, int64_t{1} << 61));
  EXPECT_THAT(Real(4, kMax).GetValue(), FieldsAre(4, kMax));
}

TEST(RealTest, BadConstructorFromFraction) {
  EXPECT_THROW(Real(1, 0), std::invalid_argument);
  EXPECT_THROW(Real(0, 0), std::invalid_argument);
  EXPECT_THROW(Real(std::numeric_limits<int64_t>::min(), -1),
               std::invalid_argument);
  EXPECT_THROW(Real(-1, std::numeric_limits<int64_t>::min()),
               std::invalid_argument);
}

TEST(RealTest, ConstructorFromString) {
  // Simple integers
  EXPECT_THAT(Real("42").GetValue(), FieldsAre(42, 1));
  EXPECT_THAT(Real("-42").GetValue(), FieldsAre(-42, 1));
  EXPECT_THAT(Real("0").GetValue(), FieldsAre(0, 1));
  EXPECT_THAT(Real("000000").GetValue(), FieldsAre(0, 1));
  EXPECT_THAT(Real("-000000").GetValue(), FieldsAre(0, 1));

  // Decimal without exponent
  EXPECT_THAT(Real("3.14").GetValue(), FieldsAre(157, 50));
  EXPECT_THAT(Real("-3.14").GetValue(), FieldsAre(-157, 50));
  EXPECT_THAT(Real("0.001").GetValue(), FieldsAre(1, 1000));
  EXPECT_THAT(Real("1.23000").GetValue(), FieldsAre(123, 100));

  // Decimal with exponent
  EXPECT_THAT(Real("3.14e2").GetValue(), FieldsAre(314, 1));
  EXPECT_THAT(Real("3.14e+2").GetValue(), FieldsAre(314, 1));
  EXPECT_THAT(Real("3.14e-2").GetValue(), FieldsAre(157, 5000));
  EXPECT_THAT(Real("0.1e1").GetValue(), FieldsAre(1, 1));
  EXPECT_THAT(Real("0.1e-1").GetValue(), FieldsAre(1, 100));

  // Decimal edge forms
  EXPECT_THAT(Real(".5").GetValue(), FieldsAre(1, 2));
  EXPECT_THAT(Real("5.").GetValue(), FieldsAre(5, 1));
  EXPECT_THAT(Real("000123.45000").GetValue(), FieldsAre(2469, 20));

  // Exponent normalization
  EXPECT_THAT(Real("1.0e0").GetValue(), FieldsAre(1, 1));
  EXPECT_THAT(Real("1e0").GetValue(), FieldsAre(1, 1));
  EXPECT_THAT(Real("1e-0").GetValue(), FieldsAre(1, 1));
  EXPECT_THAT(Real("1e+0").GetValue(), FieldsAre(1, 1));

  // Leading/trailing zeros
  EXPECT_THAT(Real("0000123").GetValue(), FieldsAre(123, 1));
  EXPECT_THAT(Real("0000.0000").GetValue(), FieldsAre(0, 1));

  // INT64_MIN edge case
  EXPECT_THAT(Real("-9223372036854775808").GetValue(),
              FieldsAre(std::numeric_limits<int64_t>::min(), 1));

  // Small exponent shifts
  EXPECT_THAT(Real("123e-2").GetValue(), FieldsAre(123, 100));
  EXPECT_THAT(Real("123.0e-2").GetValue(), FieldsAre(123, 100));
  EXPECT_THAT(Real("0.000000000000000001").GetValue(),
              FieldsAre(1, 1000000000000000000ll));

  // 20 digits, but the trailing zeros are not counted
  EXPECT_THAT(Real("1.2345678901234567890").GetValue(),
              FieldsAre(1234567890123456789ll, 1000000000000000000ll));

  // Maximum safe 18-digit value
  EXPECT_THAT(Real("999999999999999999").GetValue(),
              FieldsAre(999999999999999999LL, 1));
}

TEST(RealTest, BadConstructorFromString) {
  // Invalid formats
  EXPECT_THROW(Real(""), std::invalid_argument);
  EXPECT_THROW(Real("abc"), std::invalid_argument);
  EXPECT_THROW(Real("1.2.3"), std::invalid_argument);
  EXPECT_THROW(Real("1e2e3"), std::invalid_argument);
  EXPECT_THROW(Real("1e"), std::invalid_argument);
  EXPECT_THROW(Real("e1"), std::invalid_argument);
  EXPECT_THROW(Real("1e+2e3"), std::invalid_argument);
  EXPECT_THROW(Real("1e-2e3"), std::invalid_argument);

  // Overflow cases
  EXPECT_THROW(Real("9223372036854775808"), std::invalid_argument);
  EXPECT_THROW(Real("-9223372036854775809"), std::invalid_argument);

  EXPECT_THROW(Real("3.14e100"), std::invalid_argument);  // exponent too large
  EXPECT_THROW(Real("1.2345678901234567891"),
               std::invalid_argument);               // 20 digits
  EXPECT_THROW(Real("1e"), std::invalid_argument);   // missing exponent digits
  EXPECT_THROW(Real(".e2"), std::invalid_argument);  // missing mantissa digits
  EXPECT_THROW(Real("e10"), std::invalid_argument);  // missing base
  EXPECT_THROW(Real("abc"), std::invalid_argument);  // invalid characters
  EXPECT_THROW(Real("3.14.15"), std::invalid_argument);  // double decimal
  EXPECT_THROW(Real("1e1000"), std::invalid_argument);   // way too large

  // Bad exponent use
  EXPECT_THROW(Real("3.14e+"), std::invalid_argument);
  EXPECT_THROW(Real("+e5"), std::invalid_argument);

  // Only sign
  EXPECT_THROW(Real("+"), std::invalid_argument);
  EXPECT_THROW(Real("-"), std::invalid_argument);

  // Digit count or overflow
  EXPECT_THROW(Real("100000000000000000000"),
               std::invalid_argument);                  // 21 digits
  EXPECT_THROW(Real("1e1000"), std::invalid_argument);  // absurd exponent
  EXPECT_THROW(Real("1e19"), std::invalid_argument);    // overflows int64_t
  EXPECT_THROW(Real("1e-19"),
               std::invalid_argument);  // 10^19 denominator (overflow)
}

// ----------------------------------------------------------------------------
TEST(RealComparisonTest, NaNDouble) {
  EXPECT_EQ((Real(1) <=> std::numeric_limits<double>::quiet_NaN()),
            std::partial_ordering::unordered);
}

TEST(RealComparisonTest, Infinity) {
  EXPECT_EQ((Real(1) <=> std::numeric_limits<double>::infinity()),
            std::partial_ordering::less);
  EXPECT_EQ((Real(1) <=> -std::numeric_limits<double>::infinity()),
            std::partial_ordering::greater);
}

TEST(RealComparisonTest, ZeroVsZero) {
  EXPECT_EQ((Real(0) <=> 0.0), std::partial_ordering::equivalent);
  EXPECT_EQ((Real(0) <=> -0.0), std::partial_ordering::equivalent);
}

TEST(RealComparisonTest, ZeroVsPositiveNegative) {
  EXPECT_EQ((Real(0) <=> 1.0), std::partial_ordering::less);
  EXPECT_EQ((Real(0) <=> -1.0), std::partial_ordering::greater);
}

TEST(RealComparisonTest, PositiveVsZero) {
  EXPECT_EQ((Real(1) <=> 0.0), std::partial_ordering::greater);
}

TEST(RealComparisonTest, NegativeVsZero) {
  EXPECT_EQ((Real(-1) <=> 0.0), std::partial_ordering::less);
}

TEST(RealComparisonTest, RealNegativeVsPositiveDouble) {
  EXPECT_EQ((Real(-1) <=> 1000.0), std::partial_ordering::less);
}

TEST(RealComparisonTest, RealPositiveVsNegativeDouble) {
  EXPECT_EQ((Real(1) <=> -1000.0), std::partial_ordering::greater);
}

TEST(RealComparisonTest, EarlyOverflowLeftShift) {
  EXPECT_EQ((Real(1LL << 62) <=> 0.00000000000001),
            std::partial_ordering::greater);
}

TEST(RealComparisonTest, EarlyOverflowRightShift) {
  Real r((1LL << 62), 1);                                // Big numerator
  double d = std::pow(2, -100);                          // Very small double
  EXPECT_EQ((r <=> d), std::partial_ordering::greater);  // Early exit
}

TEST(RealComparisonTest, ExactEquality) {
  EXPECT_EQ((Real(1, 2) <=> 0.5), std::partial_ordering::equivalent);
}

TEST(RealComparisonTest, LessThan) {
  EXPECT_EQ((Real(1, 3) <=> 0.5), std::partial_ordering::less);
}

TEST(RealComparisonTest, GreaterThan) {
  EXPECT_EQ((Real(2, 3) <=> 0.5), std::partial_ordering::greater);
}

TEST(RealComparisonTest, SubnormalDouble) {
  double tiny = std::numeric_limits<double>::denorm_min();
  EXPECT_EQ((Real(1) <=> tiny), std::partial_ordering::greater);
  EXPECT_EQ((Real(0) <=> tiny), std::partial_ordering::less);
}

TEST(RealComparisonTest, SubnormalAlwaysLessThanSmallestReal) {
  Real smallest_positive(1, std::numeric_limits<int64_t>::max());
  Real zero(0);
  Real smallest_negative(-1, std::numeric_limits<int64_t>::max());

  // Subnormal double: ~5e-324
  double subnormal = std::ldexp(1.0, -1074);
  EXPECT_EQ((smallest_positive <=> subnormal), std::partial_ordering::greater);
  EXPECT_EQ((smallest_negative <=> subnormal), std::partial_ordering::less);
  EXPECT_EQ((zero <=> subnormal), std::partial_ordering::less);

  // Even smaller
  double zeroish = std::nextafter(0.0, 1.0);  // Same as above
  EXPECT_EQ((smallest_positive <=> zeroish), std::partial_ordering::greater);
  EXPECT_EQ((smallest_negative <=> zeroish), std::partial_ordering::less);
  EXPECT_EQ((zero <=> zeroish), std::partial_ordering::less);
}

}  // namespace
}  // namespace moriarty
