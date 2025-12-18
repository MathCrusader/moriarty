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

#include "src/contexts/internal/variable_ostream_context.h"

#include "gtest/gtest.h"
#include "src/librarian/testing/gtest_helpers.h"
#include "src/test_case.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

using ::moriarty_testing::Context;
using ::moriarty_testing::ThrowsValueNotFound;
using ::moriarty_testing::ThrowsVariableNotFound;

TEST(VariableOStreamContextTest, WriteNamedVariableSimpleCaseShouldWork) {
  std::ostringstream ss;
  Context context =
      Context().WithVariable("X", MInteger()).WithValue<MInteger>("X", 10);
  VariableOStreamContext ctx(ss, context.Variables(), context.Values());

  ctx.WriteVariable("X");

  EXPECT_EQ(ss.str(), "10");
}

TEST(VariableOStreamContextTest,
     WriteNamedVariableWithUnknownVariableShouldFail) {
  std::ostringstream ss;
  Context context;
  VariableOStreamContext ctx(ss, context.Variables(), context.Values());

  EXPECT_THAT([&] { ctx.WriteVariable("X"); }, ThrowsVariableNotFound("X"));
}

TEST(VariableOStreamContextTest, WriteUnnamedVariableSimpleCaseShouldWork) {
  std::ostringstream ss;
  Context context;
  VariableOStreamContext ctx(ss, context.Variables(), context.Values());

  ctx.WriteVariable(MInteger(), 10);

  EXPECT_EQ(ss.str(), "10");
}

TEST(VariableOStreamContextTest, WriteVariableFromSimpleCaseShouldWork) {
  std::ostringstream ss;
  Context context = Context().WithVariable("X", MInteger());
  VariableOStreamContext ctx(ss, context.Variables(), context.Values());

  TestCase test_case;
  test_case.SetValue<MInteger>("X", 10);

  ctx.WriteVariableFrom("X", test_case);

  EXPECT_EQ(ss.str(), "10");
}

TEST(VariableOStreamContextTest, WriteVariableToWithUnknownVariableShouldFail) {
  {  // No variable or value
    std::ostringstream ss;
    Context context;
    VariableOStreamContext ctx(ss, context.Variables(), context.Values());

    TestCase test_case;
    EXPECT_THAT([&] { ctx.WriteVariableFrom("X", test_case); },
                ThrowsVariableNotFound("X"));
  }
  {  // No value
    std::ostringstream ss;
    Context context = Context().WithVariable("X", MInteger());
    VariableOStreamContext ctx(ss, context.Variables(), context.Values());

    TestCase test_case;
    EXPECT_THAT([&] { ctx.WriteVariableFrom("X", test_case); },
                ThrowsValueNotFound("X"));
  }
  {  // No variable
    std::ostringstream ss;
    Context context;
    VariableOStreamContext ctx(ss, context.Variables(), context.Values());

    TestCase test_case;
    test_case.SetValue<MInteger>("X", 10);
    EXPECT_THAT([&] { ctx.WriteVariableFrom("X", test_case); },
                ThrowsVariableNotFound("X"));
  }
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
