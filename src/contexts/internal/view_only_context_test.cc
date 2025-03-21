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
#include "src/testing/gtest_helpers.h"
#include "src/variables/constraints/container_constraints.h"
#include "src/variables/marray.h"
#include "src/variables/minteger.h"
#include "src/variables/mstring.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

using moriarty_testing::Context;
using moriarty_testing::ThrowsMVariableTypeMismatch;
using moriarty_testing::ThrowsValueNotFound;
using moriarty_testing::ThrowsVariableNotFound;
using testing::SizeIs;

TEST(ViewOnlyContextTest, GetVariableShouldWork) {
  Context context = Context().WithVariable("X", MInteger());
  ViewOnlyContext ctx(context.Variables(), context.Values());

  EXPECT_NO_THROW({ (void)ctx.GetVariable<MInteger>("X"); });
  EXPECT_THAT([&] { (void)ctx.GetVariable<MInteger>("Y"); },
              ThrowsVariableNotFound("Y"));
  EXPECT_THAT([&] { (void)ctx.GetVariable<MString>("X"); },
              ThrowsMVariableTypeMismatch("MInteger", "MString"));
}

TEST(ViewOnlyContextTest, GetAnonymousVariableShouldWork) {
  Context context = Context().WithVariable("X", MInteger());
  ViewOnlyContext ctx(context.Variables(), context.Values());

  EXPECT_NO_THROW({ (void)ctx.GetAnonymousVariable("X"); });
  EXPECT_THAT([&] { (void)ctx.GetAnonymousVariable("Y"); },
              ThrowsVariableNotFound("Y"));
}

TEST(ViewOnlyContextTest, GetValueShouldWork) {
  Context context = Context()
                        .WithVariable("X", MInteger())
                        .WithVariable("Y", MInteger())
                        .WithValue<MInteger>("X", 10);
  ViewOnlyContext ctx(context.Variables(), context.Values());

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
  Context context = Context()
                        .WithVariable("X", MInteger())
                        .WithVariable("Y", MInteger())
                        .WithValue<MInteger>("X", 10);
  ViewOnlyContext ctx(context.Variables(), context.Values());

  EXPECT_EQ(ctx.GetValueIfKnown<MInteger>("X"), 10);
  EXPECT_EQ(ctx.GetValueIfKnown<MInteger>("Y"), std::nullopt);
  EXPECT_EQ(ctx.GetValueIfKnown<MInteger>("Z"), std::nullopt);
}

TEST(ViewOnlyContextTest, GetUniqueShouldWork) {
  // TODO: When we implement GetUniqueValue for MArray, we can test that here
  // with `Elements(Exactly(123))`, for example.
  Context context = Context()
                        .WithVariable("X", MInteger())
                        .WithVariable("Y", MInteger(Between(20, 20)))
                        .WithVariable("Z", MInteger(Between("X", "X")))
                        .WithVariable("W", MInteger())
                        .WithVariable("A", MArray<MInteger>(Length("X")))
                        .WithValue<MInteger>("X", 10);
  ViewOnlyContext ctx(context.Variables(), context.Values());

  EXPECT_EQ(ctx.GetUniqueValue<MInteger>("X"), 10);
  EXPECT_EQ(ctx.GetUniqueValue<MInteger>("Y"), 20);
  EXPECT_EQ(ctx.GetUniqueValue<MInteger>("Z"), 10);
  EXPECT_EQ(ctx.GetUniqueValue<MInteger>("W"), std::nullopt);
  EXPECT_EQ(ctx.GetUniqueValue<MArray<MInteger>>("A"), std::nullopt);
}

TEST(ViewOnlyContext, ValueIsKnownShouldWork) {
  Context context =
      Context().WithVariable("X", MInteger()).WithValue<MInteger>("X", 10);
  ViewOnlyContext ctx(context.Variables(), context.Values());

  EXPECT_TRUE(ctx.ValueIsKnown("X"));
  EXPECT_FALSE(ctx.ValueIsKnown("Y"));
}

TEST(ViewOnlyContext, IsSatisfiedWithShouldWork) {
  Context context =
      Context().WithVariable("X", MInteger()).WithValue<MInteger>("X", 10);
  ViewOnlyContext ctx(context.Variables(), context.Values());

  EXPECT_TRUE(ctx.IsSatisfiedWith(MInteger(Between(10, 20)), 10));
  EXPECT_FALSE(ctx.IsSatisfiedWith(MInteger(Between(10, 20)), 5));
  EXPECT_TRUE(ctx.IsSatisfiedWith(MInteger(Between("X", 10)), 10));
}

TEST(ViewOnlyContext, GetAllVariablesShouldWork) {
  Context context = Context()
                        .WithVariable("X", MInteger())
                        .WithVariable("Y", MInteger())
                        .WithValue<MInteger>("X", 10);
  ViewOnlyContext ctx(context.Variables(), context.Values());

  EXPECT_THAT(ctx.ListVariables(), SizeIs(2));
  EXPECT_EQ(ctx.ListVariables().count("X"), 1);
  EXPECT_EQ(ctx.ListVariables().count("Y"), 1);
  EXPECT_EQ(ctx.ListVariables().count("Z"), 0);
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
