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

#include "src/contexts/internal/basic_istream_context.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>

#include "gtest/gtest.h"
#include "src/librarian/errors.h"
#include "src/librarian/io_config.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

TEST(BasicIStreamTest, ReadTokenShouldAdhereToWhitespaceScrictness) {
  {
    std::stringstream ss("  \n\tHello");
    auto cr = InputCursor::CreateFlexibleStrictness(ss);
    BasicIStreamContext c(cr);
    EXPECT_EQ(c.ReadToken(), "Hello");
  }
  {
    std::stringstream ss("  \n\tHello");
    InputCursor cr(ss);
    BasicIStreamContext c(cr);
    EXPECT_THROW({ (void)c.ReadToken(); }, IOError);
  }
}

TEST(BasicIStreamTest, ReadTokenShouldWorkInSimpleCases) {
  {
    std::stringstream ss("Hello");
    auto cr = InputCursor::CreateFlexibleStrictness(ss);
    BasicIStreamContext c(cr);
    EXPECT_EQ(c.ReadToken(), "Hello");
  }
  {
    std::stringstream ss("Hello");
    InputCursor cr(ss);
    BasicIStreamContext c(cr);
    EXPECT_EQ(c.ReadToken(), "Hello");
  }
  {
    std::stringstream ss("Hello World");
    auto cr = InputCursor::CreateFlexibleStrictness(ss);
    BasicIStreamContext c(cr);
    EXPECT_EQ(c.ReadToken(), "Hello");
    EXPECT_EQ(c.ReadToken(), "World");
  }
  {
    std::stringstream ss("Hello World");
    InputCursor cr(ss);
    BasicIStreamContext c(cr);
    EXPECT_EQ(c.ReadToken(), "Hello");
    EXPECT_THROW({ (void)c.ReadToken(); }, IOError);
  }
}

TEST(BasicIStreamTest, ReadEofShouldWork) {
  {
    std::stringstream ss("");
    auto cr = InputCursor::CreateFlexibleStrictness(ss);
    BasicIStreamContext c(cr);
    EXPECT_NO_THROW({ c.ReadEof(); });
  }
  {
    std::stringstream ss("     \t\n\t");
    auto cr = InputCursor::CreateFlexibleStrictness(ss);
    BasicIStreamContext c(cr);
    EXPECT_NO_THROW({ c.ReadEof(); });
  }
  {
    std::stringstream ss("");
    InputCursor cr(ss);
    BasicIStreamContext c(cr);
    EXPECT_NO_THROW({ c.ReadEof(); });
  }
  {
    std::stringstream ss("     \t\n\t");
    InputCursor cr(ss);
    BasicIStreamContext c(cr);
    EXPECT_THROW({ c.ReadEof(); }, IOError);
  }
  {
    std::stringstream ss("Hello");
    auto cr = InputCursor::CreateFlexibleStrictness(ss);
    BasicIStreamContext c(cr);
    EXPECT_THROW({ c.ReadEof(); }, IOError);
  }
  {
    std::stringstream ss("Hello");
    InputCursor cr(ss);
    BasicIStreamContext c(cr);
    EXPECT_THROW({ c.ReadEof(); }, IOError);
  }
  {
    std::stringstream ss("Hello");
    auto cr = InputCursor::CreateFlexibleStrictness(ss);
    BasicIStreamContext c(cr);
    EXPECT_EQ(c.ReadToken(), "Hello");
    EXPECT_NO_THROW({ c.ReadEof(); });
  }
  {
    std::stringstream ss("Hello");
    InputCursor cr(ss);
    BasicIStreamContext c(cr);
    EXPECT_EQ(c.ReadToken(), "Hello");
    EXPECT_NO_THROW({ c.ReadEof(); });
  }
}

TEST(BasicIStreamTest, ReadWhitespaceShouldWorkInHappyPath) {
  {
    std::stringstream ss(" \n\tHello");
    auto cr = InputCursor::CreateFlexibleStrictness(ss);
    BasicIStreamContext c(cr);
    c.ReadWhitespace(Whitespace::kSpace);
    c.ReadWhitespace(Whitespace::kNewline);
    c.ReadWhitespace(Whitespace::kTab);
    EXPECT_EQ(c.ReadToken(), "Hello");
  }
  {
    std::stringstream ss(" \n\tHello");
    InputCursor cr(ss);
    BasicIStreamContext c(cr);
    c.ReadWhitespace(Whitespace::kSpace);
    c.ReadWhitespace(Whitespace::kNewline);
    c.ReadWhitespace(Whitespace::kTab);
    EXPECT_EQ(c.ReadToken(), "Hello");
  }
}

TEST(BasicIStreamTest, ReadWhitespaceShouldCareWhichWhitespaceInStrictMode) {
  {  // Happy
    std::stringstream ss(" \n\t");
    InputCursor cr(ss);
    BasicIStreamContext c(cr);
    c.ReadWhitespace(Whitespace::kSpace);
    c.ReadWhitespace(Whitespace::kNewline);
    c.ReadWhitespace(Whitespace::kTab);
  }
  {  // Not space
    std::stringstream ss("\n");
    InputCursor cr(ss);
    BasicIStreamContext c(cr);
    EXPECT_THROW({ c.ReadWhitespace(Whitespace::kSpace); }, IOError);
  }
  {  // Not newline
    std::stringstream ss(" ");
    InputCursor cr(ss);
    BasicIStreamContext c(cr);
    EXPECT_THROW({ c.ReadWhitespace(Whitespace::kNewline); }, IOError);
  }
  {  // Not tab
    std::stringstream ss(" ");
    InputCursor cr(ss);
    BasicIStreamContext c(cr);
    EXPECT_THROW({ c.ReadWhitespace(Whitespace::kTab); }, IOError);
  }
  {  // At EOF
    std::stringstream ss("");
    InputCursor cr(ss);
    BasicIStreamContext c(cr);
    EXPECT_THROW({ c.ReadWhitespace(Whitespace::kSpace); }, IOError);
  }
}

TEST(BasicIStreamTest,
     ReadWhitespaceShouldNotCareWhichWhitespaceInFlexibleMode) {
  std::stringstream ss("\t  ");
  auto cr = InputCursor::CreateFlexibleStrictness(ss);
  BasicIStreamContext c(cr);
  c.ReadWhitespace(Whitespace::kSpace);
  c.ReadWhitespace(Whitespace::kNewline);
  c.ReadWhitespace(Whitespace::kTab);
  EXPECT_THROW({ c.ReadWhitespace(Whitespace::kSpace); }, IOError);
}

TEST(BasicIStreamTest, ReadFunctionBehaveEndToEnd) {
  {
    std::stringstream ss("1 2 3");
    auto cr = InputCursor::CreateFlexibleStrictness(ss);
    BasicIStreamContext c(cr);
    EXPECT_EQ(c.ReadToken(), "1");
    EXPECT_EQ(c.ReadToken(), "2");
    EXPECT_EQ(c.ReadToken(), "3");
    EXPECT_NO_THROW({ c.ReadEof(); });
  }
  {
    std::stringstream ss("1 2 3");
    InputCursor cr(ss);
    BasicIStreamContext c(cr);
    EXPECT_EQ(c.ReadToken(), "1");
    c.ReadWhitespace(Whitespace::kSpace);
    EXPECT_EQ(c.ReadToken(), "2");
    c.ReadWhitespace(Whitespace::kSpace);
    EXPECT_EQ(c.ReadToken(), "3");
    EXPECT_NO_THROW({ c.ReadEof(); });
  }
  {
    std::stringstream ss("1 2 3");
    InputCursor cr(ss);
    BasicIStreamContext c(cr);
    EXPECT_EQ(c.ReadToken(), "1");
    EXPECT_THROW({ (void)c.ReadToken(); }, IOError);
  }
}

void TestReadInteger(std::string input,
                     std::optional<int64_t> strict_mode_expected,
                     std::optional<int64_t> flexible_mode_expected) {
  SCOPED_TRACE(input);
  {
    SCOPED_TRACE("strict mode");
    std::stringstream ss(input);
    InputCursor cr(ss);
    BasicIStreamContext c(cr);
    if (strict_mode_expected.has_value())
      EXPECT_EQ(c.ReadInteger(), *strict_mode_expected);
    else
      EXPECT_THROW({ (void)c.ReadInteger(); }, IOError);
  }
  {
    SCOPED_TRACE("flexible mode");
    std::stringstream ss(input);
    auto cr = InputCursor::CreateFlexibleStrictness(ss);
    BasicIStreamContext c(cr);
    if (flexible_mode_expected.has_value())
      EXPECT_EQ(c.ReadInteger(), *flexible_mode_expected);
    else
      EXPECT_THROW({ (void)c.ReadInteger(); }, IOError);
  }
}

TEST(BasicIStreamContextTest, ValidIntegerReadShouldWork) {
  {
    SCOPED_TRACE("positive integer");
    TestReadInteger("123", 123, 123);
  }
  {
    SCOPED_TRACE("negative integer");
    TestReadInteger("-123", -123, -123);
  }
  {
    SCOPED_TRACE("extremes");
    TestReadInteger("0", 0, 0);
    TestReadInteger(std::format("{}", std::numeric_limits<int64_t>::max()),
                    std::numeric_limits<int64_t>::max(),
                    std::numeric_limits<int64_t>::max());
    TestReadInteger(std::format("{}", std::numeric_limits<int64_t>::min()),
                    std::numeric_limits<int64_t>::min(),
                    std::numeric_limits<int64_t>::min());
  }
  {
    SCOPED_TRACE("negative zero");
    TestReadInteger("-0", std::nullopt, 0);
  }
  {
    SCOPED_TRACE("leading zeroes");
    TestReadInteger("000123", std::nullopt, 123);
    TestReadInteger("000", std::nullopt, 0);
    TestReadInteger("-000", std::nullopt, 0);
    TestReadInteger("-000123", std::nullopt, -123);
  }
  {
    SCOPED_TRACE("positive integer with leading whitespace");
    TestReadInteger("   123", std::nullopt, 123);
  }
  {
    SCOPED_TRACE("negative integer with leading whitespace");
    TestReadInteger("   -123", std::nullopt, -123);
  }
  {
    SCOPED_TRACE("positive integer with leading '+'");
    TestReadInteger("+123", std::nullopt, 123);
  }
}

TEST(BasicIStreamContextTest, InvalidIntegerReadShouldFail) {
  {
    SCOPED_TRACE("EOF");
    TestReadInteger("", std::nullopt, std::nullopt);
  }
  {
    SCOPED_TRACE("EOF with whitespace");
    TestReadInteger(" ", std::nullopt, std::nullopt);
  }
  {
    SCOPED_TRACE("double negative");
    TestReadInteger("--123", std::nullopt, std::nullopt);
  }
  {
    SCOPED_TRACE("positive, then negative");
    TestReadInteger("+-123", std::nullopt, std::nullopt);
  }
  {
    SCOPED_TRACE("negative, then positive");
    TestReadInteger("-+123", std::nullopt, std::nullopt);
  }
  {
    SCOPED_TRACE("invalid character start");
    TestReadInteger("c123", std::nullopt, std::nullopt);
  }
  {
    SCOPED_TRACE("invalid character middle");
    TestReadInteger("12c3", std::nullopt, std::nullopt);
  }
  {
    SCOPED_TRACE("invalid character end");
    TestReadInteger("123c", std::nullopt, std::nullopt);
  }
  {
    SCOPED_TRACE("too large positive integer");
    TestReadInteger("9223372036854775808", std::nullopt, std::nullopt);
  }
  {
    SCOPED_TRACE("too small negative integer");
    TestReadInteger("-9223372036854775809", std::nullopt, std::nullopt);
  }
  {
    SCOPED_TRACE("massive integer");
    TestReadInteger(std::string(1000, '3'), std::nullopt, std::nullopt);
    TestReadInteger(std::format("-{}", std::string(1000, '3')), std::nullopt,
                    std::nullopt);
  }
}

void TestReadReal(std::string input, int precision,
                  std::optional<double> strict_mode_expected,
                  std::optional<double> flexible_mode_expected) {
  SCOPED_TRACE(input);
  {
    SCOPED_TRACE("strict mode");
    std::stringstream ss(input);
    InputCursor cr(ss);
    BasicIStreamContext c(cr);
    if (strict_mode_expected.has_value())
      EXPECT_EQ(c.ReadReal(precision), *strict_mode_expected);
    else
      EXPECT_THROW({ (void)c.ReadReal(precision); }, IOError);
  }
  {
    SCOPED_TRACE("flexible mode");
    std::stringstream ss(input);
    auto cr = InputCursor::CreateFlexibleStrictness(ss);
    BasicIStreamContext c(cr);
    if (flexible_mode_expected.has_value())
      EXPECT_EQ(c.ReadReal(precision), *flexible_mode_expected);
    else
      EXPECT_THROW({ (void)c.ReadReal(precision); }, IOError);
  }
}

TEST(BasicIStreamContextTest, ValidRealReadShouldWork) {
  {
    SCOPED_TRACE("positive real");
    TestReadReal("123.456", 3, 123.456, 123.456);
  }
  {
    SCOPED_TRACE("negative real");
    TestReadReal("-123.456", 3, -123.456, -123.456);
  }
  {
    SCOPED_TRACE("zeros");
    TestReadReal("0.0", 1, 0.0, 0.0);
    TestReadReal("0.000", 3, 0.0, 0.0);
    TestReadReal("-0.0", 1, std::nullopt, -0.0);
    TestReadReal("-0.000", 3, std::nullopt, -0.0);
  }
  {
    SCOPED_TRACE("leading zeroes");
    TestReadReal("000123.456", 3, std::nullopt, 123.456);
    TestReadReal("000.000", 3, std::nullopt, 0.0);
    TestReadReal("-000.000", 3, std::nullopt, -0.0);
    TestReadReal("-000123.456", 3, std::nullopt, -123.456);
  }
  {
    SCOPED_TRACE("missing digit on one side of decimal point");
    TestReadReal("123.", 3, std::nullopt, 123.0);
    TestReadReal("-123.", 3, std::nullopt, -123.0);
    TestReadReal(".456", 3, std::nullopt, 0.456);
    TestReadReal(".000", 3, std::nullopt, 0.0);
    TestReadReal("-.000", 3, std::nullopt, -0.0);
    TestReadReal("-.456", 3, std::nullopt, -0.456);
  }
  {
    SCOPED_TRACE("positive real with leading whitespace");
    TestReadReal("   123.456", 3, std::nullopt, 123.456);
  }
  {
    SCOPED_TRACE("negative real with leading whitespace");
    TestReadReal("   -123.456", 3, std::nullopt, -123.456);
  }
  {
    SCOPED_TRACE("positive real with leading '+'");
    TestReadReal("+123.456", 3, std::nullopt, 123.456);
  }
  {
    SCOPED_TRACE("exponential form");
    TestReadReal("1.23e2", 3, std::nullopt, 123.0);
    TestReadReal("-1.23e2", 3, std::nullopt, -123.0);
    TestReadReal("1.23e-2", 3, std::nullopt, 0.0123);
    TestReadReal("-1.23e-2", 3, std::nullopt, -0.0123);
    TestReadReal("+1.23e2", 3, std::nullopt, 123.0);
    TestReadReal("1.23e+2", 3, std::nullopt, 123.0);
    TestReadReal("+1.23e+2", 3, std::nullopt, 123.0);
    TestReadReal("1.23E2", 3, std::nullopt, 123.0);
    TestReadReal("-1.23E2", 3, std::nullopt, -123.0);
    TestReadReal("1.23E-2", 3, std::nullopt, 0.0123);
    TestReadReal("-1.23E-2", 3, std::nullopt, -0.0123);
  }
}

TEST(BasicIStreamContextTest, InvalidRealReadShouldFail) {
  {
    SCOPED_TRACE("EOF");
    TestReadReal("", 3, std::nullopt, std::nullopt);
  }
  {
    SCOPED_TRACE("EOF with whitespace");
    TestReadReal(" ", 3, std::nullopt, std::nullopt);
  }
  {
    SCOPED_TRACE("double negative");
    TestReadReal("--123", 3, std::nullopt, std::nullopt);
  }
  {
    SCOPED_TRACE("positive, then negative");
    TestReadReal("+-123", 3, std::nullopt, std::nullopt);
  }
  {
    SCOPED_TRACE("negative, then positive");
    TestReadReal("-+123", 3, std::nullopt, std::nullopt);
  }
  {
    SCOPED_TRACE("invalid character start");
    TestReadReal("c123", 3, std::nullopt, std::nullopt);
  }
  {
    SCOPED_TRACE("invalid character middle");
    TestReadReal("12c3", 3, std::nullopt, std::nullopt);
  }
  {
    SCOPED_TRACE("invalid character end");
    TestReadReal("123c", 3, std::nullopt, std::nullopt);
  }
  {
    SCOPED_TRACE("massive string");
    TestReadReal(std::format("{}.123", std::string(1000, '3')), 3, std::nullopt,
                 std::nullopt);
    TestReadReal(std::format("-{}.123", std::string(1000, '3')), 3,
                 std::nullopt, std::nullopt);
  }
}

TEST(BasicIStreamContextTest, ReadRealWithBadPrecisionShouldThrow) {
  std::stringstream ss("123.");
  auto cr = InputCursor::CreatePreciseStrictness(ss);
  BasicIStreamContext c(cr);
  EXPECT_THROW({ (void)c.ReadReal(0); }, std::invalid_argument);
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
