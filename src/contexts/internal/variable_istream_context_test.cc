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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/internal/value_set.h"
#include "src/librarian/errors.h"
#include "src/librarian/io_config.h"
#include "src/librarian/policies.h"
#include "src/test_case.h"
#include "src/testing/gtest_helpers.h"
#include "src/variables/constraints/container_constraints.h"
#include "src/variables/marray.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

using ::moriarty_testing::Context;
using ::moriarty_testing::ThrowsVariableNotFound;
using ::testing::ElementsAre;

TEST(VariableIStreamContextTest, ReadNamedVariableShouldWork) {
  std::istringstream ss("10");
  Context context = Context().WithVariable("X", MInteger());
  InputCursor cr(ss, WhitespaceStrictness::kPrecise);
  VariableIStreamContext ctx(cr, context.Variables(), context.Values());
  EXPECT_EQ(ctx.ReadVariable<MInteger>("X"), 10);
}

TEST(VariableIStreamContextTest,
     ReadNamedVariableWithUnknownVariableShouldFail) {
  std::istringstream ss("10");
  Context context;
  InputCursor cr(ss, WhitespaceStrictness::kPrecise);
  VariableIStreamContext ctx(cr, context.Variables(), context.Values());
  EXPECT_THAT([&] { (void)ctx.ReadVariable<MInteger>("X"); },
              ThrowsVariableNotFound("X"));
}

TEST(VariableIStreamContextTest,
     ReadNamedVariableShouldBeAbleToInspectOtherValues) {
  std::istringstream ss("11 22 33");

  Context context = Context()
                        .WithVariable("N", MInteger())
                        .WithVariable("A", MArray<MInteger>(Length("N")))
                        .WithValue<MInteger>("N", 3);

  InputCursor cr(ss, WhitespaceStrictness::kPrecise);
  VariableIStreamContext ctx(cr, context.Variables(), context.Values());
  EXPECT_THAT(ctx.ReadVariable<MArray<MInteger>>("A"), ElementsAre(11, 22, 33));
}

TEST(VariableIStreamContextTest, ReadNamedVariableShouldRespectWhitespace) {
  Context context = Context().WithVariable("A", MArray<MInteger>(Length(2)));
  {
    std::istringstream ss("11 22");
    InputCursor cr(ss, WhitespaceStrictness::kPrecise);
    VariableIStreamContext ctx(cr, context.Variables(), context.Values());
    EXPECT_THAT(ctx.ReadVariable<MArray<MInteger>>("A"), ElementsAre(11, 22));
  }
  {
    std::istringstream ss("11    22");
    InputCursor cr(ss, WhitespaceStrictness::kFlexible);
    VariableIStreamContext ctx(cr, context.Variables(), context.Values());
    EXPECT_THAT(ctx.ReadVariable<MArray<MInteger>>("A"), ElementsAre(11, 22));
  }
  {
    std::istringstream ss("11    22");
    InputCursor cr(ss, WhitespaceStrictness::kPrecise);
    VariableIStreamContext ctx(cr, context.Variables(), context.Values());
    EXPECT_THROW({ (void)ctx.ReadVariable<MArray<MInteger>>("A"); }, IOError);
  }
}

TEST(VariableIStreamContextTest, ReadUnnamedVariableShouldWork) {
  Context context;
  std::istringstream ss("10");
  InputCursor cr(ss, WhitespaceStrictness::kPrecise);
  VariableIStreamContext ctx(cr, context.Variables(), context.Values());
  EXPECT_EQ(ctx.ReadVariable(MInteger()), 10);
}

TEST(VariableIStreamContextTest,
     ReadUnnamedVariableShouldBeAbleToInspectOtherValues) {
  std::istringstream ss("11 22 33");

  Context context =
      Context().WithVariable("N", MInteger()).WithValue<MInteger>("N", 3);

  InputCursor cr(ss, WhitespaceStrictness::kPrecise);
  VariableIStreamContext ctx(cr, context.Variables(), context.Values());
  EXPECT_THAT(ctx.ReadVariable(MArray<MInteger>(Length("N"))),
              ElementsAre(11, 22, 33));
}

TEST(VariableIStreamContextTest, ReadUnnamedVariableShouldRespectWhitespace) {
  Context context;
  {
    std::istringstream ss("11 22");
    InputCursor cr(ss, WhitespaceStrictness::kPrecise);
    VariableIStreamContext ctx(cr, context.Variables(), context.Values());
    EXPECT_THAT(ctx.ReadVariable(MArray<MInteger>(Length(2))),
                ElementsAre(11, 22));
  }
  {
    std::istringstream ss("11    22");
    InputCursor cr(ss, WhitespaceStrictness::kFlexible);
    VariableIStreamContext ctx(cr, context.Variables(), context.Values());
    EXPECT_THAT(ctx.ReadVariable(MArray<MInteger>(Length(2))),
                ElementsAre(11, 22));
  }
  {
    std::istringstream ss("11    22");
    InputCursor cr(ss, WhitespaceStrictness::kPrecise);
    VariableIStreamContext ctx(cr, context.Variables(), context.Values());
    EXPECT_THROW(
        { (void)ctx.ReadVariable(MArray<MInteger>(Length(2))); }, IOError);
  }
}

TEST(VariableIStreamContextTest, ReadVariableToShouldWork) {
  Context context = Context().WithVariable("X", MInteger());
  ConcreteTestCase test_case;
  std::istringstream ss("10");
  InputCursor cr(ss, WhitespaceStrictness::kPrecise);
  VariableIStreamContext ctx(cr, context.Variables(), context.Values());
  ctx.ReadVariableTo("X", test_case);

  ValueSet new_values = UnsafeExtractConcreteTestCaseInternals(test_case);
  EXPECT_EQ(new_values.Get<MInteger>("X"), 10);
}

TEST(VariableIStreamContextTest, ReadVariableToWithUnknownVariableShouldFail) {
  Context context;
  ConcreteTestCase test_case;
  std::istringstream ss("10");
  InputCursor cr(ss, WhitespaceStrictness::kPrecise);
  VariableIStreamContext ctx(cr, context.Variables(), context.Values());
  EXPECT_THAT([&] { ctx.ReadVariableTo("X", test_case); },
              ThrowsVariableNotFound("X"));
}

TEST(VariableIStreamContextTest,
     ReadVariableToShouldBeAbleToInspectOtherValues) {
  std::istringstream ss("11 22 33");

  // FIXME: ReadVariableTo should be able to inspect global values.
  Context context = Context()
                        .WithVariable("N", MInteger())
                        .WithVariable("A", MArray<MInteger>(Length("N")));

  InputCursor cr(ss, WhitespaceStrictness::kPrecise);
  VariableIStreamContext ctx(cr, context.Variables(), context.Values());
  ConcreteTestCase test_case;
  test_case.SetValue<MInteger>("N", 3);
  ctx.ReadVariableTo("A", test_case);
  ValueSet new_values = UnsafeExtractConcreteTestCaseInternals(test_case);
  EXPECT_THAT(new_values.Get<MArray<MInteger>>("A"), ElementsAre(11, 22, 33));
}

TEST(VariableIStreamContextTest, ReadVariableToShouldRespectWhitespace) {
  Context context = Context().WithVariable("A", MArray<MInteger>(Length(2)));
  {
    std::istringstream ss("11 22");
    InputCursor cr(ss, WhitespaceStrictness::kPrecise);
    VariableIStreamContext ctx(cr, context.Variables(), context.Values());
    ConcreteTestCase test_case;
    ctx.ReadVariableTo("A", test_case);
    ValueSet new_values = UnsafeExtractConcreteTestCaseInternals(test_case);
    EXPECT_THAT(new_values.Get<MArray<MInteger>>("A"), ElementsAre(11, 22));
  }
  {
    std::istringstream ss("11    22");
    InputCursor cr(ss, WhitespaceStrictness::kFlexible);
    VariableIStreamContext ctx(cr, context.Variables(), context.Values());
    ConcreteTestCase test_case;
    ctx.ReadVariableTo("A", test_case);
    ValueSet new_values = UnsafeExtractConcreteTestCaseInternals(test_case);
    EXPECT_THAT(new_values.Get<MArray<MInteger>>("A"), ElementsAre(11, 22));
  }
  {
    std::istringstream ss("11    22");
    InputCursor cr(ss, WhitespaceStrictness::kPrecise);
    VariableIStreamContext ctx(cr, context.Variables(), context.Values());
    ConcreteTestCase test_case;
    EXPECT_THROW({ ctx.ReadVariableTo("A", test_case); }, IOError);
  }
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
