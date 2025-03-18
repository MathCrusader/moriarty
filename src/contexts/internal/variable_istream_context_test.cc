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

#include "src/contexts/internal/variable_istream_context.h"

#include <stdexcept>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/internal/value_set.h"
#include "src/test_case.h"
#include "src/testing/status_test_util.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/constraints/container_constraints.h"
#include "src/variables/marray.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

using ::moriarty_testing::ThrowsVariableNotFound;
using ::testing::ElementsAre;

TEST(VariableIStreamContextTest, ReadNamedVariableShouldWork) {
  ValueSet values;
  std::istringstream ss("10");
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("X", MInteger()));
  VariableIStreamContext ctx(ss, WhitespaceStrictness::kPrecise, variables,
                             values);
  EXPECT_EQ(ctx.ReadVariable<MInteger>("X"), 10);
}

TEST(VariableIStreamContextTest,
     ReadNamedVariableWithUnknownVariableShouldFail) {
  ValueSet values;
  std::istringstream ss("10");
  VariableSet variables;
  VariableIStreamContext ctx(ss, WhitespaceStrictness::kPrecise, variables,
                             values);
  EXPECT_THAT([&] { (void)ctx.ReadVariable<MInteger>("X"); },
              ThrowsVariableNotFound("X"));
}

TEST(VariableIStreamContextTest,
     ReadNamedVariableShouldBeAbleToInspectOtherValues) {
  std::istringstream ss("11 22 33");

  ValueSet values;
  values.Set<MInteger>("N", 3);

  VariableSet variables;
  MORIARTY_EXPECT_OK(variables.AddVariable("N", MInteger()));
  MORIARTY_EXPECT_OK(variables.AddVariable("A", MArray<MInteger>(Length("N"))));

  VariableIStreamContext ctx(ss, WhitespaceStrictness::kPrecise, variables,
                             values);
  EXPECT_THAT(ctx.ReadVariable<MArray<MInteger>>("A"), ElementsAre(11, 22, 33));
}

TEST(VariableIStreamContextTest, ReadNamedVariableShouldRespectWhitespace) {
  ValueSet values;
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MArray<MInteger>(Length(2))));
  {
    std::istringstream ss("11 22");
    VariableIStreamContext ctx(ss, WhitespaceStrictness::kPrecise, variables,
                               values);
    EXPECT_THAT(ctx.ReadVariable<MArray<MInteger>>("A"), ElementsAre(11, 22));
  }
  {
    std::istringstream ss("11    22");
    VariableIStreamContext ctx(ss, WhitespaceStrictness::kFlexible, variables,
                               values);
    EXPECT_THAT(ctx.ReadVariable<MArray<MInteger>>("A"), ElementsAre(11, 22));
  }
  {
    std::istringstream ss("11    22");
    VariableIStreamContext ctx(ss, WhitespaceStrictness::kPrecise, variables,
                               values);
    EXPECT_THROW(
        { (void)ctx.ReadVariable<MArray<MInteger>>("A"); }, std::runtime_error);
  }
}

TEST(VariableIStreamContextTest, ReadUnnamedVariableShouldWork) {
  ValueSet values;
  VariableSet variables;

  std::istringstream ss("10");
  VariableIStreamContext ctx(ss, WhitespaceStrictness::kPrecise, variables,
                             values);
  EXPECT_EQ(ctx.ReadVariable(MInteger()), 10);
}

TEST(VariableIStreamContextTest,
     ReadUnnamedVariableShouldBeAbleToInspectOtherValues) {
  std::istringstream ss("11 22 33");

  ValueSet values;
  values.Set<MInteger>("N", 3);

  VariableSet variables;
  MORIARTY_EXPECT_OK(variables.AddVariable("N", MInteger()));

  VariableIStreamContext ctx(ss, WhitespaceStrictness::kPrecise, variables,
                             values);
  EXPECT_THAT(ctx.ReadVariable(MArray<MInteger>(Length("N"))),
              ElementsAre(11, 22, 33));
}

TEST(VariableIStreamContextTest, ReadUnnamedVariableShouldRespectWhitespace) {
  ValueSet values;
  VariableSet variables;
  {
    std::istringstream ss("11 22");
    VariableIStreamContext ctx(ss, WhitespaceStrictness::kPrecise, variables,
                               values);
    EXPECT_THAT(ctx.ReadVariable(MArray<MInteger>(Length(2))),
                ElementsAre(11, 22));
  }
  {
    std::istringstream ss("11    22");
    VariableIStreamContext ctx(ss, WhitespaceStrictness::kFlexible, variables,
                               values);
    EXPECT_THAT(ctx.ReadVariable(MArray<MInteger>(Length(2))),
                ElementsAre(11, 22));
  }
  {
    std::istringstream ss("11    22");
    VariableIStreamContext ctx(ss, WhitespaceStrictness::kPrecise, variables,
                               values);
    EXPECT_THROW(
        { (void)ctx.ReadVariable(MArray<MInteger>(Length(2))); },
        std::runtime_error);
  }
}

TEST(VariableIStreamContextTest, ReadVariableToShouldWork) {
  ValueSet values;
  VariableSet variables;
  ConcreteTestCase test_case;
  MORIARTY_ASSERT_OK(variables.AddVariable("X", MInteger()));
  std::istringstream ss("10");
  VariableIStreamContext ctx(ss, WhitespaceStrictness::kPrecise, variables,
                             values);
  ctx.ReadVariableTo("X", test_case);

  ValueSet new_values = UnsafeExtractConcreteTestCaseInternals(test_case);
  EXPECT_EQ(new_values.Get<MInteger>("X"), 10);
}

TEST(VariableIStreamContextTest, ReadVariableToWithUnknownVariableShouldFail) {
  ValueSet values;
  VariableSet variables;
  ConcreteTestCase test_case;
  std::istringstream ss("10");
  VariableIStreamContext ctx(ss, WhitespaceStrictness::kPrecise, variables,
                             values);
  EXPECT_THAT([&] { ctx.ReadVariableTo("X", test_case); },
              ThrowsVariableNotFound("X"));
}

TEST(VariableIStreamContextTest,
     ReadVariableToShouldBeAbleToInspectOtherValues) {
  std::istringstream ss("11 22 33");

  ValueSet values;
  values.Set<MInteger>("N", 3);

  VariableSet variables;
  MORIARTY_EXPECT_OK(variables.AddVariable("N", MInteger()));
  MORIARTY_EXPECT_OK(variables.AddVariable("A", MArray<MInteger>(Length("N"))));

  VariableIStreamContext ctx(ss, WhitespaceStrictness::kPrecise, variables,
                             values);
  ConcreteTestCase test_case;
  ctx.ReadVariableTo("A", test_case);
  ValueSet new_values = UnsafeExtractConcreteTestCaseInternals(test_case);
  EXPECT_THAT(new_values.Get<MArray<MInteger>>("A"), ElementsAre(11, 22, 33));
}

TEST(VariableIStreamContextTest, ReadVariableToShouldRespectWhitespace) {
  ValueSet values;
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MArray<MInteger>(Length(2))));
  {
    std::istringstream ss("11 22");
    VariableIStreamContext ctx(ss, WhitespaceStrictness::kPrecise, variables,
                               values);
    ConcreteTestCase test_case;
    ctx.ReadVariableTo("A", test_case);
    ValueSet new_values = UnsafeExtractConcreteTestCaseInternals(test_case);
    EXPECT_THAT(new_values.Get<MArray<MInteger>>("A"), ElementsAre(11, 22));
  }
  {
    std::istringstream ss("11    22");
    VariableIStreamContext ctx(ss, WhitespaceStrictness::kFlexible, variables,
                               values);
    ConcreteTestCase test_case;
    ctx.ReadVariableTo("A", test_case);
    ValueSet new_values = UnsafeExtractConcreteTestCaseInternals(test_case);
    EXPECT_THAT(new_values.Get<MArray<MInteger>>("A"), ElementsAre(11, 22));
  }
  {
    std::istringstream ss("11    22");
    VariableIStreamContext ctx(ss, WhitespaceStrictness::kPrecise, variables,
                               values);
    ConcreteTestCase test_case;
    EXPECT_THROW({ ctx.ReadVariableTo("A", test_case); }, std::runtime_error);
  }
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
