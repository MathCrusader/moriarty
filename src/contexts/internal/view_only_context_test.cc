/*
 * Copyright 2025 Darcy Best
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

#include "src/contexts/internal/view_only_context.h"

#include "gtest/gtest.h"
#include "src/internal/value_set.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/constraints/container_constraints.h"
#include "src/variables/marray.h"
#include "src/variables/minteger.h"
#include "src/variables/mstring.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

using moriarty_testing::ThrowsMVariableTypeMismatch;
using moriarty_testing::ThrowsValueNotFound;
using moriarty_testing::ThrowsVariableNotFound;
using testing::Not;
using testing::SizeIs;

TEST(ViewOnlyContextTest, GetVariableShouldWork) {
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("X", MInteger()));
  ValueSet values;
  ViewOnlyContext ctx(variables, values);

  EXPECT_NO_THROW({ (void)ctx.GetVariable<MInteger>("X"); });
  EXPECT_THAT([&] { (void)ctx.GetVariable<MInteger>("Y"); },
              ThrowsVariableNotFound("Y"));
  EXPECT_THAT([&] { (void)ctx.GetVariable<MString>("X"); },
              ThrowsMVariableTypeMismatch("MInteger", "MString"));
}

TEST(ViewOnlyContextTest, GetAnonymousVariableShouldWork) {
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("X", MInteger()));
  ValueSet values;
  ViewOnlyContext ctx(variables, values);

  EXPECT_NO_THROW({ (void)ctx.GetAnonymousVariable("X"); });
  EXPECT_THAT([&] { (void)ctx.GetAnonymousVariable("Y"); },
              ThrowsVariableNotFound("Y"));
}

TEST(ViewOnlyContextTest, GetValueShouldWork) {
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("X", MInteger()));
  MORIARTY_ASSERT_OK(variables.AddVariable("Y", MInteger()));
  ValueSet values;
  values.Set<MInteger>("X", 10);
  ViewOnlyContext ctx(variables, values);

  EXPECT_EQ(ctx.GetValue<MInteger>("X"), 10);
  EXPECT_THAT([&] { (void)ctx.GetValue<MInteger>("Y"); },
              ThrowsValueNotFound("Y"));  // No value

  // Note that in this case, we should really be checking for a missing variable
  // first... TBD if we will check for that in the future. It's technically
  // better, but also an extra hashmap lookup on every happy path for slightly
  // more consistent error messages.
  EXPECT_THAT([&] { (void)ctx.GetValue<MInteger>("Z"); },
              ThrowsValueNotFound("Z"));
}

TEST(ViewOnlyContextTest, GetValueIfKnownShouldWork) {
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("X", MInteger()));
  MORIARTY_ASSERT_OK(variables.AddVariable("Y", MInteger()));
  ValueSet values;
  values.Set<MInteger>("X", 10);
  ViewOnlyContext ctx(variables, values);

  EXPECT_EQ(ctx.GetValueIfKnown<MInteger>("X"), 10);
  EXPECT_EQ(ctx.GetValueIfKnown<MInteger>("Y"), std::nullopt);
  EXPECT_EQ(ctx.GetValueIfKnown<MInteger>("Z"), std::nullopt);
}

TEST(ViewOnlyContextTest, GetUniqueShouldWork) {
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("X", MInteger()));
  MORIARTY_ASSERT_OK(variables.AddVariable("Y", MInteger(Between(20, 20))));
  MORIARTY_ASSERT_OK(variables.AddVariable("Z", MInteger(Between("X", "X"))));
  MORIARTY_ASSERT_OK(variables.AddVariable("W", MInteger()));

  // TODO: When we implement GetUniqueValue for MArray, we can test that here
  // with `Elements(Exactly(123))`, for example.
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MArray<MInteger>(Length("X"))));

  ValueSet values;
  values.Set<MInteger>("X", 10);
  ViewOnlyContext ctx(variables, values);

  EXPECT_EQ(ctx.GetUniqueValue<MInteger>("X"), 10);
  EXPECT_EQ(ctx.GetUniqueValue<MInteger>("Y"), 20);
  EXPECT_EQ(ctx.GetUniqueValue<MInteger>("Z"), 10);
  EXPECT_EQ(ctx.GetUniqueValue<MInteger>("W"), std::nullopt);
  EXPECT_EQ(ctx.GetUniqueValue<MArray<MInteger>>("A"), std::nullopt);
}

TEST(ViewOnlyContext, ValueIsKnownShouldWork) {
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("X", MInteger()));
  ValueSet values;
  values.Set<MInteger>("X", 10);
  ViewOnlyContext ctx(variables, values);

  EXPECT_TRUE(ctx.ValueIsKnown("X"));
  EXPECT_FALSE(ctx.ValueIsKnown("Y"));
}

TEST(ViewOnlyContext, SatisfiesConstraintsShouldWork) {
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("X", MInteger()));
  ValueSet values;
  values.Set<MInteger>("X", 10);
  ViewOnlyContext ctx(variables, values);

  EXPECT_THAT(ctx.SatisfiesConstraints(MInteger(Between(10, 20)), 10), IsOk());
  EXPECT_THAT(ctx.SatisfiesConstraints(MInteger(Between(10, 20)), 5),
              Not(IsOk()));
  EXPECT_THAT(ctx.SatisfiesConstraints(MInteger(Between("X", 10)), 10), IsOk());
}

TEST(ViewOnlyContext, GetAllVariablesShouldWork) {
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("X", MInteger()));
  MORIARTY_ASSERT_OK(variables.AddVariable("Y", MInteger()));
  ValueSet values;
  values.Set<MInteger>("X", 10);
  ViewOnlyContext ctx(variables, values);

  EXPECT_THAT(ctx.GetAllVariables(), SizeIs(2));
  EXPECT_EQ(ctx.GetAllVariables().count("X"), 1);
  EXPECT_EQ(ctx.GetAllVariables().count("Y"), 1);
  EXPECT_EQ(ctx.GetAllVariables().count("Z"), 0);
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
