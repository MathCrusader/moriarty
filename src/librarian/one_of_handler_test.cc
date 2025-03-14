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

#include "src/librarian/one_of_handler.h"

#include <stdexcept>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace moriarty {
namespace librarian {
namespace {

using ::testing::AnyOf;
using ::testing::UnorderedElementsAre;

TEST(OneOfHandler, HasBeenConstrainedReturnsFalseInitially) {
  OneOfHandler<int> handler;
  EXPECT_FALSE(handler.HasBeenConstrained());
}

TEST(OneOfHandler, HasBeenConstrainedReturnsTrueAfterAnyCall) {
  OneOfHandler<int> handler;
  handler.ConstrainOptions(std::vector{1, 2, 3});
  EXPECT_TRUE(handler.HasBeenConstrained());
}

TEST(OneOfHandler, IsSatisfiedWithReturnsIsSatisfiedWithAnythingInitially) {
  OneOfHandler<int> handler;
  EXPECT_TRUE(handler.IsSatisfiedWith(1));
  EXPECT_TRUE(handler.IsSatisfiedWith(-2));
  EXPECT_TRUE(handler.IsSatisfiedWith(3321578));
}

TEST(OneOfHandler, IsSatisfiedWithReturnsAsExpectedForConstrainedValue) {
  OneOfHandler<int> handler;
  handler.ConstrainOptions(std::vector{1, 2, 3});
  EXPECT_TRUE(handler.IsSatisfiedWith(1));
  EXPECT_TRUE(handler.IsSatisfiedWith(2));
  EXPECT_TRUE(handler.IsSatisfiedWith(3));
  EXPECT_FALSE(handler.IsSatisfiedWith(4));

  handler.ConstrainOptions(std::vector{1, 2});
  EXPECT_TRUE(handler.IsSatisfiedWith(1));
  EXPECT_TRUE(handler.IsSatisfiedWith(2));
  EXPECT_FALSE(handler.IsSatisfiedWith(3));
  EXPECT_FALSE(handler.IsSatisfiedWith(4));
}

TEST(OneOfHandler, SelectOneOfReturnsOneOfTheConstrainedValues) {
  OneOfHandler<int> handler;
  handler.ConstrainOptions(std::vector{1, 2, 3});
  EXPECT_THAT(handler.SelectOneOf([](int n) { return n - 1; }), AnyOf(1, 2, 3));
}

TEST(OneOfHandler, GetUniqueValueReturnsTheUniqueValueIfThereIsOne) {
  {  // No constraints
    OneOfHandler<int> handler;
    EXPECT_EQ(handler.GetUniqueValue(), std::nullopt);
  }
  {  // One value
    OneOfHandler<int> handler;
    handler.ConstrainOptions(std::vector{1});
    EXPECT_EQ(handler.GetUniqueValue(), 1);
  }
  {  // One value after intersection
    OneOfHandler<int> handler;
    handler.ConstrainOptions(std::vector{1, 3});
    handler.ConstrainOptions(std::vector{2, 3});
    EXPECT_EQ(handler.GetUniqueValue(), 3);
  }
  {  // Multiple values

    OneOfHandler<int> handler;
    handler.ConstrainOptions(std::vector{1, 4, 7});
    EXPECT_EQ(handler.GetUniqueValue(), std::nullopt);
    handler.ConstrainOptions(std::vector{4, 7, 10});
    EXPECT_EQ(handler.GetUniqueValue(), std::nullopt);
  }
}

TEST(OneOfHandler, GetOptionsReturnsAllValidOptions) {
  {
    OneOfHandler<int> handler;
    handler.ConstrainOptions(std::vector{1, 2, 3});
    EXPECT_THAT(handler.GetOptions(), UnorderedElementsAre(1, 2, 3));
    handler.ConstrainOptions(std::vector{1, 2});
    EXPECT_THAT(handler.GetOptions(), UnorderedElementsAre(1, 2));
    handler.ConstrainOptions(std::vector{2});
    EXPECT_THAT(handler.GetOptions(), UnorderedElementsAre(2));
  }
  {
    OneOfHandler<int> handler;
    handler.ConstrainOptions(std::vector{1, 2, 3});
    handler.ConstrainOptions(std::vector{2, 3, 4});
    EXPECT_THAT(handler.GetOptions(), UnorderedElementsAre(2, 3));
  }
}

TEST(OneOfHandler, ConstrainOptionsLeftWithNoValidOptionsShouldThrow) {
  {  // No options initially
    OneOfHandler<int> handler;
    EXPECT_THROW(
        { handler.ConstrainOptions(std::vector<int>()); }, std::runtime_error);
  }
  {  // No options after intersection
    OneOfHandler<int> handler;
    handler.ConstrainOptions(std::vector{1, 2, 3});
    EXPECT_THROW(
        { handler.ConstrainOptions(std::vector{4, 5, 6}); },
        std::runtime_error);
  }
  {  // No options after multiple intersections
    OneOfHandler<int> handler;
    handler.ConstrainOptions(std::vector{1, 2, 3});
    handler.ConstrainOptions(std::vector{2, 3, 4});
    EXPECT_THROW(
        { handler.ConstrainOptions(std::vector{1, 4}); }, std::runtime_error);
  }
}

}  // namespace
}  // namespace librarian
}  // namespace moriarty
