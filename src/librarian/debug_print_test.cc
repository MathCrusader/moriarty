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

#include "src/librarian/debug_print.h"

#include "gtest/gtest.h"

namespace moriarty {
namespace librarian {
namespace {

TEST(ShortenDebugStringTest, BasicCases) {
  EXPECT_EQ(ShortenDebugString("hello world", 11), "hello world");
  EXPECT_EQ(ShortenDebugString("hello world", 10), "hell...rld");
  EXPECT_EQ(ShortenDebugString("hello world", 15), "hello world");
  EXPECT_EQ(ShortenDebugString("short", 10), "short");
  EXPECT_EQ(ShortenDebugString("", 10), "");
}

TEST(ShortenDebugStringTest, BoundaryCases) {
  EXPECT_EQ(ShortenDebugString("12345678901", 10), "1234...901");
  EXPECT_EQ(ShortenDebugString("abcdefghijk", 10), "abcd...ijk");
  EXPECT_EQ(ShortenDebugString("a very long string", 10), "a ve...ing");
}

TEST(DebugStringTest, IntegerShouldWork) {
  EXPECT_EQ(DebugString(42), "42");
  EXPECT_EQ(DebugString(-12345), "-12345");
  EXPECT_EQ(DebugString(123456789012345LL, 13), "12345...12345");
  EXPECT_EQ(DebugString(0), "0");
}

TEST(DebugStringTest, StringShouldWork) {
  EXPECT_EQ(DebugString("hello"), "hello");
  EXPECT_EQ(DebugString("this is a very long string", 10), "this...ing");
  EXPECT_EQ(DebugString("", 5), "");
  EXPECT_EQ(DebugString("short", 10), "short");
}

TEST(DebugStringTest, ArrayShouldWork) {
  std::vector<int> vec = {1, 2, 3, 4, 5};
  EXPECT_EQ(DebugString(vec, 10), "[1,2...,5]");
  EXPECT_EQ(DebugString(std::vector<int>{}, 10), "[]");
  EXPECT_EQ(DebugString(std::vector<int>{1}, 10), "[1]");

  std::vector<std::string> str_vec = {"apple", "banana", "cherry"};
  EXPECT_EQ(DebugString(str_vec, 15), "[apple...herry]");
  EXPECT_EQ(DebugString(str_vec, 30), "[apple,banana,cherry]");
  EXPECT_EQ(DebugString(std::vector<std::string>{}, 10), "[]");
  EXPECT_EQ(DebugString(std::vector<std::string>{"single"}, 10), "[single]");
}

TEST(DebugStringTest, TupleShouldWork) {
  auto tup = std::make_tuple(1, "hello", 42);
  EXPECT_EQ(DebugString(tup, 20), "<1,hello,42>");
  EXPECT_EQ(DebugString(std::make_tuple(), 10), "<>");
  EXPECT_EQ(DebugString(std::make_tuple(42), 10), "<42>");
  EXPECT_EQ(DebugString(std::make_tuple(std::string(30, 'a'), 1), 15),
            "<aaaaa...aaa,1>");
}

}  // namespace
}  // namespace librarian
}  // namespace moriarty
