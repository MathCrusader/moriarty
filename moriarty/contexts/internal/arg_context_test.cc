// Copyright 2026 Darcy Best
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

#include "moriarty/contexts/internal/arg_context.h"

#include <unordered_map>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "moriarty/librarian/errors.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

using ::testing::HasSubstr;
using ::testing::StartsWith;
using ::testing::ThrowsMessage;

TEST(ArgContextTest, ArgShouldReturnStringArgs) {
  std::unordered_map<std::string, std::string> args = {
      {"foo", "bar"},
      {"baz", "qux"},
  };
  ArgContext ctx(args);

  EXPECT_EQ(ctx.Arg<std::string>("foo"), "bar");
  EXPECT_EQ(ctx.Arg<std::string>("baz"), "qux");
}

TEST(ArgContextTest, ArgShouldReturnIntegralArgs) {
  std::unordered_map<std::string, std::string> args = {
      {"foo", "42"},
      {"baz", "-17"},
  };
  ArgContext ctx(args);

  EXPECT_EQ(ctx.Arg<int>("foo"), 42);
  EXPECT_EQ(ctx.Arg<int>("baz"), -17);
}

TEST(ArgContextTest, ArgShouldThrowIfArgNotFound) {
  std::unordered_map<std::string, std::string> args = {
      {"foo", "42"},
  };
  ArgContext ctx(args);

  EXPECT_THAT([&] { (void)ctx.Arg<int>("bar"); },
              ThrowsMessage<GenerationError>(
                  StartsWith("Generator argument not found: bar")));
}

TEST(ArgContextTest, ArgShouldThrowIfIntegralArgInvalid) {
  std::unordered_map<std::string, std::string> args = {
      {"foo", "not_an_int"},
      {"neg", "-1"},
      {"medium", "12345678901"},  // Too large for 32-bit, but fine for 64-bit
      {"large", "9223372036854775818"},  // >2^63, which is too large for int64
      {"massive",
       "18446744073709551616"},  // 2^64, which is too large for uint64
  };
  ArgContext ctx(args);

  EXPECT_THAT([&] { (void)ctx.Arg<int>("foo"); },
              ThrowsMessage<GenerationError>(
                  HasSubstr("does not parse to the requested type")));
  EXPECT_THAT([&] { (void)ctx.Arg<unsigned int>("neg"); },
              ThrowsMessage<GenerationError>(
                  HasSubstr("does not parse to the requested type")));
  EXPECT_EQ(ctx.Arg<int>("neg"), -1);

  EXPECT_THAT([&] { (void)ctx.Arg<int>("medium"); },
              ThrowsMessage<GenerationError>(
                  HasSubstr("does not fit in requested integer type")));
  EXPECT_EQ(ctx.Arg<long long>("medium"), 12345678901LL);

  EXPECT_THAT([&] { (void)ctx.Arg<int32_t>("large"); },
              ThrowsMessage<GenerationError>(
                  HasSubstr("does not parse to the requested type")));
  EXPECT_THAT([&] { (void)ctx.Arg<uint32_t>("large"); },
              ThrowsMessage<GenerationError>(
                  HasSubstr("does not fit in requested integer type")));
  EXPECT_THAT([&] { (void)ctx.Arg<int64_t>("large"); },
              ThrowsMessage<GenerationError>(
                  HasSubstr("does not parse to the requested type")));
  EXPECT_EQ(ctx.Arg<uint64_t>("large"), 9223372036854775818ULL);

  EXPECT_THAT([&] { (void)ctx.Arg<uint64_t>("massive"); },
              ThrowsMessage<GenerationError>(
                  HasSubstr("does not parse to the requested type")));
}

TEST(ArgContextTest, FloatingPointArgsShouldBeParsedCorrectly) {
  std::unordered_map<std::string, std::string> args = {
      {"pi", "3.14159"},
      {"e", "2.71828"},
      {"bigish", "1e8"},
      {"negative", "-0.001"},
      {"not_a_float", "156not_a_float"},
      {"not_a_float_either", "not_a_float"},
  };
  ArgContext ctx(args);

  EXPECT_DOUBLE_EQ(ctx.Arg<double>("pi"), 3.14159);
  EXPECT_DOUBLE_EQ(ctx.Arg<double>("e"), 2.71828);
  EXPECT_DOUBLE_EQ(ctx.Arg<double>("bigish"), 1e8);
  EXPECT_DOUBLE_EQ(ctx.Arg<double>("negative"), -0.001);

  EXPECT_THAT([&] { (void)ctx.Arg<double>("not_a_float"); },
              ThrowsMessage<GenerationError>(
                  HasSubstr("does not parse to the requested type")));
  EXPECT_THAT([&] { (void)ctx.Arg<double>("not_a_float_either"); },
              ThrowsMessage<GenerationError>(
                  HasSubstr("does not parse to the requested type")));
}

TEST(ArgContextTest, IntegerArgWithAPrefixShouldBeParsedCorrectly) {
  std::unordered_map<std::string, std::string> args = {
      {"x", "42hello"},
  };
  ArgContext ctx(args);

  EXPECT_EQ(ctx.Arg<std::string>("x"), "42hello");
  EXPECT_THAT([&] { (void)ctx.Arg<int>("x"); },
              ThrowsMessage<GenerationError>(
                  HasSubstr("does not parse to the requested type")));
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
