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

#include "src/util/locked_optional.h"

#include "gtest/gtest.h"

namespace moriarty {
namespace librarian {
namespace {

TEST(LockedOptionalTest, SetReturnsTrueForFirstCall) {
  LockedOptional<int> locked(0);
  EXPECT_TRUE(locked.Set(1));
}

TEST(LockedOptionalTest, SetReturnsFalseForDifferentValue) {
  LockedOptional<int> locked(0);
  EXPECT_TRUE(locked.Set(1));
  EXPECT_FALSE(locked.Set(2));
}

TEST(LockedOptionalTest, SetReturnsTrueForSameValue) {
  LockedOptional<int> locked(0);
  EXPECT_TRUE(locked.Set(1));
  EXPECT_TRUE(locked.Set(1));
}

TEST(LockedOptionalTest, GetReturnsDefaultValueInitially) {
  LockedOptional<int> locked(42);
  EXPECT_EQ(locked.Get(), 42);
}

TEST(LockedOptionalTest, GetReturnsSetValue) {
  LockedOptional<int> locked(42);
  locked.Set(1);
  EXPECT_EQ(locked.Get(), 1);
  EXPECT_FALSE(locked.Set(2));
  EXPECT_EQ(locked.Get(), 1);
}

}  // namespace
}  // namespace librarian
}  // namespace moriarty
