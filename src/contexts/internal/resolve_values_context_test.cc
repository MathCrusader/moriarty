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

#include "src/contexts/internal/resolve_values_context.h"

#include <cstdint>
#include <stdexcept>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/constraints/numeric_constraints.h"
#include "src/internal/generation_handler.h"
#include "src/internal/random_engine.h"
#include "src/librarian/testing/gtest_helpers.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

using ::moriarty_testing::Context;
using ::testing::AllOf;
using ::testing::Ge;
using ::testing::Le;
using ::testing::Throws;

TEST(ResolveValuesContextTest, GenerateVariableInHappyPathShouldWork) {
  {  // No extra constraints
    Context context = Context().WithVariable("X", MInteger(Between(1, 1000)));
    RandomEngine engine({1, 2, 3}, "v0.1");
    GenerationHandler handler;

    ResolveValuesContext ctx(context.Variables(), context.Values(), engine,
                             handler);
    int64_t X = ctx.GenerateVariable<MInteger>("X");
    EXPECT_THAT(X, AllOf(Ge(1), Le(1000)));
    EXPECT_EQ(context.Values().Get<MInteger>("X"), X);
  }
  {  // Extra constraints
    Context context = Context().WithVariable("X", MInteger(Between(1, 1000)));
    RandomEngine engine({1, 2, 3}, "v0.1");
    GenerationHandler handler;
    ResolveValuesContext ctx(context.Variables(), context.Values(), engine,
                             handler);
    int64_t X = ctx.GenerateVariable<MInteger>("X", MInteger(Exactly(42)));
    EXPECT_EQ(X, 42);
    EXPECT_EQ(context.Values().Get<MInteger>("X"), X);
  }
  {  // Value already known, no extra constraints

    Context context = Context()
                          .WithVariable("X", MInteger(Between(1, 1000)))
                          .WithValue<MInteger>("X", 42);
    RandomEngine engine({1, 2, 3}, "v0.1");
    GenerationHandler handler;

    ResolveValuesContext ctx(context.Variables(), context.Values(), engine,
                             handler);
    EXPECT_EQ(ctx.GenerateVariable<MInteger>("X"), 42);
    EXPECT_EQ(context.Values().Get<MInteger>("X"), 42);
  }
  {  // Value already known, with extra constraints
    Context context = Context()
                          .WithVariable("X", MInteger(Between(1, 1000)))
                          .WithValue<MInteger>("X", 42);
    RandomEngine engine({1, 2, 3}, "v0.1");
    GenerationHandler handler;

    ResolveValuesContext ctx(context.Variables(), context.Values(), engine,
                             handler);
    int64_t X = ctx.GenerateVariable<MInteger>("X", MInteger(Between(40, 45)));
    EXPECT_EQ(X, 42);
    EXPECT_EQ(context.Values().Get<MInteger>("X"), 42);
  }
}

TEST(ResolveValuesContextTest,
     GenerateVariableWithExtraConstraintsButValueDoesNotSatisfyShouldThrow) {
  Context context = Context()
                        .WithVariable("X", MInteger(Between(1, 1000)))
                        .WithValue<MInteger>("X", 42);
  RandomEngine engine({1, 2, 3}, "v0.1");
  GenerationHandler handler;

  ResolveValuesContext ctx(context.Variables(), context.Values(), engine,
                           handler);
  EXPECT_THAT(
      [&] { (void)ctx.GenerateVariable<MInteger>("X", MInteger(Exactly(10))); },
      Throws<std::runtime_error>());
}

TEST(ResolveValuesContextTest,
     GenerateVariableShouldHaveAccessToOtherVariables) {
  Context context = Context()
                        .WithVariable("X", MInteger(Between("N-1", "N")))
                        .WithVariable("N", MInteger(Between(42, 42)));
  RandomEngine engine({1, 2, 3}, "v0.1");
  GenerationHandler handler;

  ResolveValuesContext ctx(context.Variables(), context.Values(), engine,
                           handler);
  int64_t X = ctx.GenerateVariable<MInteger>("X");
  EXPECT_THAT(X, AllOf(Ge(41), Le(42)));
  EXPECT_EQ(context.Values().Get<MInteger>("X"), X);
  EXPECT_EQ(context.Values().Get<MInteger>("N"),
            42LL);  // Sets another variable
}

TEST(ResolveValuesContextTest, AssignVariableShouldSetTheValueInContext) {
  Context context = Context()
                        .WithVariable("X", MInteger(Between("N-1", "N")))
                        .WithVariable("N", MInteger(Between(42, 42)));
  RandomEngine engine({1, 2, 3}, "v0.1");
  GenerationHandler handler;

  ResolveValuesContext ctx(context.Variables(), context.Values(), engine,
                           handler);
  ctx.AssignVariable("X");
  EXPECT_THAT(context.Values().Get<MInteger>("X"), AllOf(Ge(41), Le(42)));
  EXPECT_EQ(context.Values().Get<MInteger>("N"), 42);  // Sets another variable
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
