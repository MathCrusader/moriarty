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

#include "src/contexts/internal/variable_ostream_context.h"

#include "gtest/gtest.h"
#include "src/internal/value_set.h"
#include "src/test_case.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace moriarty_internal {
namespace {
using moriarty_testing::ThrowsVariableNotFound;

TEST(VariableOStreamContextTest, PrintNamedVariableSimpleCaseShouldWork) {
  std::ostringstream ss;
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("X", MInteger()));
  ValueSet values;
  values.Set<MInteger>("X", 10);
  VariableOStreamContext ctx(ss, variables, values);

  ctx.PrintVariable("X");

  EXPECT_EQ(ss.str(), "10");
}

TEST(VariableOStreamContextTest,
     PrintNamedVariableWithUnknownVariableShouldFail) {
  std::ostringstream ss;
  VariableSet variables;
  ValueSet values;
  VariableOStreamContext ctx(ss, variables, values);

  EXPECT_THAT([&] { ctx.PrintVariable("X"); }, ThrowsVariableNotFound("X"));
}

TEST(VariableOStreamContextTest, PrintUnnamedVariableSimpleCaseShouldWork) {
  std::ostringstream ss;
  VariableSet variables;
  ValueSet values;
  VariableOStreamContext ctx(ss, variables, values);

  ctx.PrintVariable(MInteger(), 10);

  EXPECT_EQ(ss.str(), "10");
}

TEST(VariableOStreamContextTest, PrintVariableFromSimpleCaseShouldWork) {
  std::ostringstream ss;
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("X", MInteger()));
  ValueSet values;
  VariableOStreamContext ctx(ss, variables, values);

  ConcreteTestCase test_case;
  test_case.SetValue<MInteger>("X", 10);

  ctx.PrintVariableFrom("X", test_case);

  EXPECT_EQ(ss.str(), "10");
}

TEST(VariableOStreamContextTest, PrintVariableToWithUnknownVariableShouldFail) {
  ValueSet values;

  {  // No variable or value
    std::ostringstream ss;
    VariableSet variables;
    VariableOStreamContext ctx(ss, variables, values);

    ConcreteTestCase test_case;
    EXPECT_THAT([&] { ctx.PrintVariableFrom("X", test_case); },
                ThrowsVariableNotFound("X"));
  }
  {  // No value
    std::ostringstream ss;
    VariableSet variables;
    MORIARTY_ASSERT_OK(variables.AddVariable("X", MInteger()));
    VariableOStreamContext ctx(ss, variables, values);

    ConcreteTestCase test_case;
    EXPECT_THROW(
        { ctx.PrintVariableFrom("X", test_case); }, std::runtime_error);
  }
  {  // No variable
    std::ostringstream ss;
    VariableSet variables;
    VariableOStreamContext ctx(ss, variables, values);

    ConcreteTestCase test_case;
    test_case.SetValue<MInteger>("X", 10);
    EXPECT_THAT([&] { ctx.PrintVariableFrom("X", test_case); },
                ThrowsVariableNotFound("X"));
  }
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
