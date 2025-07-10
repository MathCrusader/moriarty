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

#include "src/internal/value_set.h"

#include <cstdint>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/librarian/testing/gtest_helpers.h"
#include "src/variables/minteger.h"
#include "src/variables/mstring.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

using ::moriarty_testing::ThrowsValueNotFound;
using ::moriarty_testing::ThrowsValueTypeMismatch;
using ::testing::AnyWith;

TEST(ValueSetTest, SimpleGetAndSetWorks) {
  ValueSet values;
  values.Set<MInteger>("x", 5);
  EXPECT_EQ(values.Get<MInteger>("x"), 5);
}

TEST(ValueSetTest, OverwritingTheSameVariableShouldReplaceTheValue) {
  ValueSet values;
  values.Set<MInteger>("x", 5);
  EXPECT_EQ(values.Get<MInteger>("x"), 5);
  values.Set<MInteger>("x", 10);
  EXPECT_EQ(values.Get<MInteger>("x"), 10);
}

TEST(ValueSetTest, MultipleVariablesShouldWork) {
  ValueSet values;
  values.Set<MInteger>("x", 5);
  values.Set<MInteger>("y", 10);
  EXPECT_EQ(values.Get<MInteger>("x"), 5);
  EXPECT_EQ(values.Get<MInteger>("y"), 10);
}

TEST(ValueSetTest, RequestingANonExistentVariableShouldThrow) {
  ValueSet values;
  EXPECT_THAT([&] { (void)values.Get<MInteger>("x"); },
              ThrowsValueNotFound("x"));

  values.Set<MInteger>("x", 5);
  EXPECT_THAT([&] { (void)values.Get<MInteger>("y"); },
              ThrowsValueNotFound("y"));
}

TEST(ValueSetTest, RequestingTheWrongTypeShouldThrow) {
  ValueSet values;
  values.Set<MInteger>("x", 10);
  EXPECT_THAT([&] { (void)values.Get<MString>("x"); },
              ThrowsValueTypeMismatch("x", "MString"));

  values.Set<MString>("x", "hello");
  EXPECT_THAT([&] { (void)values.Get<MInteger>("x"); },
              ThrowsValueTypeMismatch("x", "MInteger"));
  EXPECT_EQ(values.Get<MString>("x"), "hello");
}

TEST(ValueSetTest, SimpleUnsafeGetWorks) {
  ValueSet values;
  values.Set<MInteger>("x", 5);

  EXPECT_THAT(values.UnsafeGet("x"), AnyWith<int64_t>(5));
}

TEST(ValueSetTest, UnsafeGetOverwritingTheSameVariableShouldReplaceTheValue) {
  ValueSet values;

  values.Set<MInteger>("x", 5);
  EXPECT_THAT(values.UnsafeGet("x"), AnyWith<int64_t>(5));

  values.Set<MString>("x", "hi");
  EXPECT_THAT(values.UnsafeGet("x"), AnyWith<std::string>("hi"));
}

TEST(ValueSetTest, UnsafeGetRequestingANonExistentVariableFails) {
  ValueSet values;
  EXPECT_THAT([&] { (void)values.UnsafeGet("x"); }, ThrowsValueNotFound("x"));

  values.Set<MInteger>("x", 5);
  EXPECT_THAT([&] { (void)values.UnsafeGet("y"); }, ThrowsValueNotFound("y"));
}

TEST(ValueSetTest, ContainsShouldWork) {
  ValueSet values;
  EXPECT_FALSE(values.Contains("x"));
  values.Set<MInteger>("x", 5);
  EXPECT_TRUE(values.Contains("x"));
  values.Set<MInteger>("x", 10);
  EXPECT_TRUE(values.Contains("x"));

  EXPECT_FALSE(values.Contains("y"));
}

TEST(ValueSetTest, EraseRemovesTheValueFromTheSet) {
  ValueSet values;
  values.Set<MInteger>("x", 5);
  values.Erase("x");
  EXPECT_FALSE(values.Contains("x"));
}

TEST(ValueSetTest, ErasingANonExistentVariableSucceeds) {
  ValueSet values;
  values.Erase("x");
  EXPECT_FALSE(values.Contains("x"));
}

TEST(ValueSetTest, ErasingMultipleTimesSucceeds) {
  ValueSet values;
  values.Set<MInteger>("x", 5);

  values.Erase("x");
  EXPECT_FALSE(values.Contains("x"));
  values.Erase("x");
  EXPECT_FALSE(values.Contains("x"));
}

TEST(ValueSetTest, ErasingVariableLeavesOthersAlone) {
  ValueSet values;
  values.Set<MInteger>("x", 5);
  values.Set<MInteger>("y", 5);

  values.Erase("x");
  EXPECT_TRUE(values.Contains("y"));
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
