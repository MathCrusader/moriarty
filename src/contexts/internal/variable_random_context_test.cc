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

#include "src/contexts/internal/variable_random_context.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/internal/random_engine.h"
#include "src/librarian/testing/gtest_helpers.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace moriarty_internal {
namespace {
using moriarty_testing::Context;
using testing::AllOf;
using testing::Ge;
using testing::Le;

TEST(VariableRandomContextTest, RandomUnnamedWithEmptyContextShouldWork) {
  Context context;
  RandomEngine engine({1, 2, 3}, "v0.1");
  VariableRandomContext ctx(context.Variables(), context.Values(), engine);
  EXPECT_THAT(ctx.Random(MInteger(Between(1, 10))), AllOf(Ge(1), Le(10)));
}

TEST(VariableRandomContextTest, RandomUnnamedWithContextShouldWork) {
  Context context =
      Context().WithVariable("N", MInteger()).WithValue<MInteger>("N", 1123);

  RandomEngine engine({1, 2, 3}, "v0.1");
  VariableRandomContext ctx(context.Variables(), context.Values(), engine);
  EXPECT_THAT(ctx.Random(MInteger(Between("N", "N+1"))),
              AllOf(Ge(1123), Le(1124)));
}

TEST(VariableRandomContextTest, RandomNamedVariableWithoutContextShouldWork) {
  Context context = Context().WithVariable("N", MInteger(Between(1, 10)));
  RandomEngine engine({1, 2, 3}, "v0.1");
  VariableRandomContext ctx(context.Variables(), context.Values(), engine);
  EXPECT_THAT(ctx.RandomValue<MInteger>("N"), AllOf(Ge(1), Le(10)));
}

TEST(VariableRandomContextTest, RandomNamedVariableWithContextShouldWork) {
  Context context = Context()
                        .WithVariable("N", MInteger())
                        .WithVariable("X", MInteger(Between("N", "N+1")))
                        .WithValue<MInteger>("N", 1123);

  RandomEngine engine({1, 2, 3}, "v0.1");
  VariableRandomContext ctx(context.Variables(), context.Values(), engine);
  EXPECT_THAT(ctx.RandomValue<MInteger>("X"), AllOf(Ge(1123), Le(1124)));
  EXPECT_EQ(ctx.RandomValue<MInteger>("N"), 1123);
}

TEST(VariableRandomContextTest,
     RandomNamedVariableWithExtraConstraintsShouldWork) {
  Context context = Context()
                        .WithVariable("N", MInteger())
                        .WithVariable("X", MInteger(Between("N", "N+100000")))
                        .WithValue<MInteger>("N", 1123);

  RandomEngine engine({1, 2, 3}, "v0.1");
  VariableRandomContext ctx(context.Variables(), context.Values(), engine);
  // Very unlikely to have a false positive...
  EXPECT_EQ(ctx.RandomValue<MInteger>("X", MInteger(AtMost("N"))), 1123);
}

TEST(VariableRandomContextTest, MinAndMaxValueShouldWork) {
  {  // No value set
    Context context =
        Context().WithVariable("N", MInteger(Between(1, 1'000'000)));
    RandomEngine engine({1, 2, 3}, "v0.1");
    VariableRandomContext ctx(context.Variables(), context.Values(), engine);
    EXPECT_EQ(ctx.MinValue<MInteger>("N"), 1);
    EXPECT_EQ(ctx.MaxValue<MInteger>("N"), 1'000'000);
  }
  {  // Value set
    Context context = Context()
                          .WithVariable("N", MInteger(Between(1, 1'000'000)))
                          .WithValue<MInteger>("N", 1123);
    RandomEngine engine({1, 2, 3}, "v0.1");
    VariableRandomContext ctx(context.Variables(), context.Values(), engine);
    EXPECT_EQ(ctx.MinValue<MInteger>("N"), 1123);
    EXPECT_EQ(ctx.MaxValue<MInteger>("N"), 1123);
  }
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
