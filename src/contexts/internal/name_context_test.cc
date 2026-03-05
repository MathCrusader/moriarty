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

#include "src/contexts/internal/name_context.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

using ::testing::ElementsAre;

TEST(NameContextTest, GetVariableNameShouldReturnCorrectName) {
  NameContext ctx("X");
  EXPECT_EQ(ctx.GetLocalVariableName(), "X");
}

TEST(NameContextTest, ForSubVariableShouldReturnCorrectName) {
  NameContext ctx("X");
  NameContext sub_ctx = ctx.ForSubVariable("Y");
  EXPECT_EQ(sub_ctx.GetLocalVariableName(), "Y");
}

TEST(NameContextTest, ForIndexedSubVariableShouldReturnCorrectName) {
  NameContext ctx("X");
  NameContext indexed_ctx = ctx.ForIndexedSubVariable(
      [](int index) { return "index " + std::to_string(index); }, 2);
  EXPECT_EQ(indexed_ctx.GetLocalVariableName(), "index 2");
}

TEST(NameContextTest, GetVariableStackShouldReturnCorrectStack) {
  {
    NameContext ctx("X");
    EXPECT_THAT(ctx.GetVariableStack(), ElementsAre("X"));
  }
  {
    NameContext ctx("X");
    NameContext sub_ctx = ctx.ForSubVariable("Y");

    EXPECT_THAT(sub_ctx.GetVariableStack(), ElementsAre("X", "Y"));
  }
  {
    NameContext ctx("X");
    NameContext sub_ctx = ctx.ForSubVariable("Y");
    NameContext indexed_ctx = sub_ctx.ForIndexedSubVariable(
        [](int index) { return "index " + std::to_string(index); }, 2);

    EXPECT_THAT(indexed_ctx.GetVariableStack(),
                ElementsAre("X", "Y", "index 2"));
  }
  {
    NameContext ctx("X");
    NameContext sub_ctx = ctx.ForSubVariable("Y");
    NameContext indexed_ctx = sub_ctx.ForIndexedSubVariable(
        [](int index) { return "index " + std::to_string(index); }, 2);

    EXPECT_THAT(indexed_ctx
                    .ForIndexedSubVariable(
                        [](int x) { return "inner " + std::to_string(x); }, 3)
                    .GetVariableStack(),
                ElementsAre("X", "Y", "index 2", "inner 3"));
  }
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
