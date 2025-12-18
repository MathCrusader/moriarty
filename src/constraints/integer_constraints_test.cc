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

#include "src/constraints/integer_constraints.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/contexts/librarian_context.h"
#include "src/librarian/testing/gtest_helpers.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace {

using ::moriarty::librarian::AnalyzeVariableContext;
using ::moriarty_testing::Context;
using ::moriarty_testing::HasConstraintViolation;
using ::moriarty_testing::HasNoConstraintViolation;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

TEST(IntegerConstraintsTest, ModCheckValue) {
  auto context = Context().WithValue<MInteger>("X", 7);

  AnalyzeVariableContext ctx("N", context.Variables(), context.Values());

  EXPECT_THAT(Mod(1, 10).CheckValue(ctx, 11), HasNoConstraintViolation());
  EXPECT_THAT(Mod(0, 10).CheckValue(ctx, 110), HasNoConstraintViolation());
  EXPECT_THAT(Mod(1, 10).CheckValue(ctx, -9), HasNoConstraintViolation());
  EXPECT_THAT(Mod("X + 4", 10).CheckValue(ctx, 1), HasNoConstraintViolation());
  EXPECT_THAT(Mod(1, "X + 3").CheckValue(ctx, 1), HasNoConstraintViolation());
  EXPECT_THAT(Mod("10 * X + 1", "X + 3").CheckValue(ctx, 1),
              HasNoConstraintViolation());

  EXPECT_THAT(Mod(1, 10).CheckValue(ctx, 6),
              HasConstraintViolation(HasSubstr("6 is not 1 mod 10")));
  EXPECT_THAT(Mod("X + 4", 10).CheckValue(ctx, 6),
              HasConstraintViolation(HasSubstr("6 is not 1 mod 10")));
}

TEST(IntegerConstraintsTest, ModToString) {
  EXPECT_EQ(Mod(4, 7).ToString(), "x % (7) == 4");
  EXPECT_EQ(Mod("N + 1", "3 * M").ToString(), "x % (3 * M) == N + 1");
}

TEST(IntegerConstraintsTest, ModGetDependencies) {
  EXPECT_THAT(Mod(4, 7).GetDependencies(), IsEmpty());
  EXPECT_THAT(Mod("N + 1", "3 * M").GetDependencies(),
              UnorderedElementsAre("N", "M"));
}

}  // namespace
}  // namespace moriarty
