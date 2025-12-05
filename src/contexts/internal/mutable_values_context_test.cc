// Copyright 2025 Darcy Best
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

#include "src/contexts/internal/mutable_values_context.h"

#include "gtest/gtest.h"
#include "src/internal/value_set.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

TEST(MutableValuesContextTest, SettingAValueShouldWork) {
  ValueSet values;
  MutableValuesContext ctx(values);
  ctx.SetValue<MInteger>("X", 10);
  EXPECT_EQ(values.Get<MInteger>("X"), 10);
}

TEST(MutableValuesContextTest, OverwritingAValueShouldWork) {
  ValueSet values;
  MutableValuesContext ctx(values);
  ctx.SetValue<MInteger>("X", 10);
  ctx.SetValue<MInteger>("X", 20);
  EXPECT_EQ(values.Get<MInteger>("X"), 20);
}

TEST(MutableValuesContextTest, ErasingAValueShouldWork) {
  ValueSet values;
  MutableValuesContext ctx(values);
  ctx.SetValue<MInteger>("X", 10);
  ctx.EraseValue("X");
  EXPECT_FALSE(values.Contains("X"));
}

TEST(MutableValuesContextTest, ErasingANonExistingValueShouldWork) {
  ValueSet values;
  MutableValuesContext ctx(values);
  ctx.EraseValue("X");
  EXPECT_FALSE(values.Contains("X"));
}

TEST(MutableValuesContextTest, ErasingAValueShouldNotImpactOtherVariables) {
  ValueSet values;
  MutableValuesContext ctx(values);
  ctx.SetValue<MInteger>("X1", 10);
  ctx.SetValue<MInteger>("X2", 20);
  ctx.EraseValue("X1");
  EXPECT_FALSE(values.Contains("X1"));
  EXPECT_EQ(values.Get<MInteger>("X2"), 20);
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
