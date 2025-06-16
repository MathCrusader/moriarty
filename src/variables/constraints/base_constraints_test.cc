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

#include "src/variables/constraints/base_constraints.h"

#include <cstdint>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/testing/gtest_helpers.h"

namespace moriarty {
namespace {

using moriarty_testing::HasConstraintViolation;
using moriarty_testing::HasNoConstraintViolation;
using testing::ElementsAre;
using testing::HasSubstr;
using testing::Throws;

TEST(BaseConstraintsTest, ExactlyForVariousIntegerTypesWorks) {
  static_assert(std::is_same_v<decltype(Exactly(123)), Exactly<int64_t>>);
  static_assert(std::is_same_v<decltype(Exactly(static_cast<int32_t>(123))),
                               Exactly<int64_t>>);
  static_assert(std::is_same_v<decltype(Exactly(static_cast<int64_t>(123))),
                               Exactly<int64_t>>);
  static_assert(std::is_same_v<decltype(Exactly(static_cast<uint32_t>(123))),
                               Exactly<int64_t>>);
  static_assert(std::is_same_v<decltype(Exactly(static_cast<uint64_t>(123))),
                               Exactly<int64_t>>);

  EXPECT_EQ(Exactly(123).GetValue(), int64_t{123});
  EXPECT_EQ(Exactly(static_cast<int32_t>(123)).GetValue(), int64_t{123});
  EXPECT_EQ(Exactly(static_cast<int64_t>(123)).GetValue(), int64_t{123});
  EXPECT_EQ(Exactly(static_cast<uint32_t>(123)).GetValue(), int64_t{123});
  EXPECT_EQ(Exactly(static_cast<uint64_t>(123)).GetValue(), int64_t{123});
}

TEST(BaseConstraintsTest, ExactlyForVariousStringLikeTypesWorks) {
  static_assert(std::is_same_v<decltype(Exactly("abc")), Exactly<std::string>>);
  static_assert(std::is_same_v<decltype(Exactly(std::string("abc"))),
                               Exactly<std::string>>);
  static_assert(std::is_same_v<decltype(Exactly(std::string_view("abc"))),
                               Exactly<std::string>>);
  static_assert(std::is_same_v<decltype(Exactly(std::string_view("abc"))),
                               Exactly<std::string>>);

  EXPECT_EQ(Exactly("abc").GetValue(), std::string("abc"));
  EXPECT_EQ(Exactly(std::string("abc")).GetValue(), std::string("abc"));
  EXPECT_EQ(Exactly(std::string_view("abc")).GetValue(), std::string("abc"));
  EXPECT_EQ(Exactly(std::string_view("abc")).GetValue(), std::string("abc"));
}

TEST(BaseConstraintsTest, ExactlyForVariousArrayLikeTypesWorks) {
  // TODO: The internals of vector are not fully being deduced the way we want
  // them to (e.g., std::vector<int> instead of std::vector<int64_t>). We should
  // fix that.

  {  // initializer_list<int>
    static_assert(std::is_same_v<decltype(Exactly({1, 2, 3})),
                                 Exactly<std::vector<int>>>);
  }
  {  // span<const int>
    std::vector<int> v = {1, 2, 3};
    static_assert(std::is_same_v<decltype(Exactly(std::span<const int>{v})),
                                 Exactly<std::vector<int>>>);
  }
  {
    // span<int>
    std::vector<int> v = {1, 2, 3};
    static_assert(std::is_same_v<decltype(Exactly(std::span<int>{v})),
                                 Exactly<std::vector<int>>>);
  }
  {  // initializer_list<std::string>
    static_assert(std::is_same_v<decltype(Exactly(
                                     {std::string("abc"), std::string("def")})),
                                 Exactly<std::vector<std::string>>>);
  }
  {  // vector<std::string>
    std::vector<std::string> v = {"abc", "def"};
    static_assert(std::is_same_v<decltype(Exactly(v)),
                                 Exactly<std::vector<std::string>>>);
  }
  {  // span<const std::string>
    std::vector<std::string> v = {"abc", "def"};
    static_assert(
        std::is_same_v<decltype(Exactly(std::span<const std::string>{v})),
                       Exactly<std::vector<std::string>>>);
  }
  {  // span<std::string>
    std::vector<std::string> v = {"abc", "def"};
    static_assert(std::is_same_v<decltype(Exactly(std::span<std::string>{v})),
                                 Exactly<std::vector<std::string>>>);
  }
}

TEST(BaseConstraintsTest, ExactlyForOtherTypesWorks) {
  EXPECT_EQ(Exactly(std::set<int>({1, 2, 3})).GetValue(),
            std::set<int>({1, 2, 3}));
}

TEST(BaseConstraintsTest, ExactlyToStringWorks) {
  EXPECT_EQ(Exactly(123).ToString(), "is exactly `123`");
  EXPECT_EQ(Exactly("abc").ToString(), "is exactly `abc`");
}

TEST(BaseConstraintsTest, ExactlyWithTooLargeOfIntegersShouldThrow) {
  EXPECT_THAT([=]() { (void)Exactly(std::numeric_limits<uint64_t>::max()); },
              Throws<std::invalid_argument>());
}

TEST(BaseConstraintsTest, ExactlyCheckValueShouldWork) {
  {
    EXPECT_THAT(Exactly(123).CheckValue(123), HasNoConstraintViolation());
    EXPECT_THAT(Exactly("abc").CheckValue("abc"), HasNoConstraintViolation());
  }
  {
    EXPECT_THAT(Exactly(123).CheckValue(124),
                HasConstraintViolation(HasSubstr("exactly")));
    EXPECT_THAT(Exactly("abc").CheckValue("def"),
                HasConstraintViolation(HasSubstr("exactly")));
  }
}

// ---------------------------------------------------------------------------
//  OneOf

TEST(BaseConstraintsTest, OneOfForVariousIntegerTypesWorks) {
  {  // initializer_list
    static_assert(std::is_same_v<decltype(OneOf({1, 2, 3})), OneOf<int64_t>>);
    static_assert(std::is_same_v<decltype(OneOf({static_cast<int32_t>(1),
                                                 static_cast<int32_t>(2),
                                                 static_cast<int32_t>(3)})),
                                 OneOf<int64_t>>);
    static_assert(std::is_same_v<decltype(OneOf({static_cast<int64_t>(1),
                                                 static_cast<int64_t>(2),
                                                 static_cast<int64_t>(3)})),
                                 OneOf<int64_t>>);
    static_assert(std::is_same_v<decltype(OneOf({static_cast<uint32_t>(1),
                                                 static_cast<uint32_t>(2),
                                                 static_cast<uint32_t>(3)})),
                                 OneOf<int64_t>>);
    static_assert(std::is_same_v<decltype(OneOf({static_cast<uint64_t>(1),
                                                 static_cast<uint64_t>(2),
                                                 static_cast<uint64_t>(3)})),
                                 OneOf<int64_t>>);
  }
  {  // vector
    std::vector<int> v = {1, 2, 3};
    static_assert(std::is_same_v<decltype(OneOf(v)), OneOf<int64_t>>);
  }
  {  // span
    std::vector<int> v = {1, 2, 3};
    static_assert(
        std::is_same_v<decltype(OneOf(std::span<int>{v})), OneOf<int64_t>>);
    static_assert(std::is_same_v<decltype(OneOf(std::span<const int>{v})),
                                 OneOf<int64_t>>);
  }

  EXPECT_EQ(OneOf({1, 2, 3}).GetOptions(), std::vector<int64_t>({1, 2, 3}));
  EXPECT_EQ(OneOf({static_cast<int32_t>(1), static_cast<int32_t>(2),
                   static_cast<int32_t>(3)})
                .GetOptions(),
            std::vector<int64_t>({1, 2, 3}));
  EXPECT_EQ(OneOf({static_cast<int64_t>(1), static_cast<int64_t>(2),
                   static_cast<int64_t>(3)})
                .GetOptions(),
            std::vector<int64_t>({1, 2, 3}));
  EXPECT_EQ(OneOf({static_cast<uint32_t>(1), static_cast<uint32_t>(2),
                   static_cast<uint32_t>(3)})
                .GetOptions(),
            std::vector<int64_t>({1, 2, 3}));
  EXPECT_EQ(OneOf({static_cast<uint64_t>(1), static_cast<uint64_t>(2),
                   static_cast<uint64_t>(3)})
                .GetOptions(),
            std::vector<int64_t>({1, 2, 3}));
}

TEST(BaseConstraintsTest, OneOfForVariousStringTypesWorks) {
  {  // initializer_list
    static_assert(
        std::is_same_v<decltype(OneOf({"abc", "def"})), OneOf<std::string>>);
    static_assert(std::is_same_v<decltype(OneOf(
                                     {std::string("abc"), std::string("def")})),
                                 OneOf<std::string>>);
    static_assert(std::is_same_v<decltype(OneOf({std::string_view("abc"),
                                                 std::string_view("def")})),
                                 OneOf<std::string>>);
  }
  {  // vector
    std::vector<std::string> v = {"abc", "def"};
    static_assert(std::is_same_v<decltype(OneOf(v)), OneOf<std::string>>);
  }
  {  // span
    std::vector<std::string> v = {"abc", "def"};
    static_assert(std::is_same_v<decltype(OneOf(std::span<std::string>{v})),
                                 OneOf<std::string>>);
    static_assert(
        std::is_same_v<decltype(OneOf(std::span<const std::string>{v})),
                       OneOf<std::string>>);
  }

  EXPECT_EQ(OneOf({"abc", "def"}).GetOptions(),
            std::vector<std::string>({"abc", "def"}));
  EXPECT_EQ(OneOf({std::string("abc"), std::string("def")}).GetOptions(),
            std::vector<std::string>({"abc", "def"}));
  EXPECT_EQ(
      OneOf({std::string_view("abc"), std::string_view("def")}).GetOptions(),
      std::vector<std::string>({"abc", "def"}));
}

TEST(BaseConstraintsTest, OneOfForOtherTypesWorks) {
  EXPECT_THAT(
      OneOf({std::set<int>({1, 2, 3}), std::set<int>({4, 5, 6})}).GetOptions(),
      ElementsAre(std::set<int>({1, 2, 3}), std::set<int>({4, 5, 6})));
}

TEST(BaseConstraintsTest, OneOfToStringWorks) {
  EXPECT_EQ(OneOf({123, 456}).ToString(), "is one of {`123`, `456`}");
  EXPECT_EQ(OneOf({"abc", "def"}).ToString(), "is one of {`abc`, `def`}");
}

TEST(BaseConstraintsTest, OneOfWithTooLargeOfIntegersShouldThrow) {
  // One is okay, one isn't. (ensuring we test multiple)
  EXPECT_THAT(
      [=]() {
        (void)OneOf({uint64_t{1}, std::numeric_limits<uint64_t>::max()});
      },
      Throws<std::invalid_argument>());
}

TEST(BaseConstraintsTest, OneOfCheckValueShouldWork) {
  {
    EXPECT_THAT(OneOf({123, 456}).CheckValue(123), HasNoConstraintViolation());
    EXPECT_THAT(OneOf({456}).CheckValue(456), HasNoConstraintViolation());
    EXPECT_THAT(OneOf({"abc", "def", "hello"}).CheckValue("abc"),
                HasNoConstraintViolation());
    EXPECT_THAT(OneOf({"abc", "def", "hello"}).CheckValue("hello"),
                HasNoConstraintViolation());
  }
  {
    EXPECT_THAT(OneOf({123, 456}).CheckValue(123567),
                HasConstraintViolation(HasSubstr("one of")));
    EXPECT_THAT(OneOf({456}).CheckValue(123),
                HasConstraintViolation(HasSubstr("one of")));
    EXPECT_THAT(OneOf({"abc", "def", "hello"}).CheckValue("ABC"),
                HasConstraintViolation(HasSubstr("one of")));
    EXPECT_THAT(OneOf({"abc", "def", "hello"}).CheckValue("ertert"),
                HasConstraintViolation(HasSubstr("one of")));
  }
}

}  // namespace
}  // namespace moriarty
