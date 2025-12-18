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

#include "src/librarian/constraint_handler.h"

#include <format>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/constraints/constraint_violation.h"
#include "src/contexts/librarian_context.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/testing/gtest_helpers.h"
#include "src/variables/minteger.h"
#include "src/variables/mstring.h"

namespace moriarty {
namespace librarian {
namespace {

using ::moriarty::MInteger;
using ::moriarty::MString;
using ::moriarty_testing::HasConstraintViolation;
using ::moriarty_testing::HasNoConstraintViolation;
using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::HasSubstr;
using ::testing::Not;

struct Even {
  ConstraintViolation CheckValue(AnalyzeVariableContext ctx, int value) const {
    if (value % 2 == 0) return ConstraintViolation::None();
    return ConstraintViolation(std::format("`{}` is not even", value));
  }
  std::string ToString() const { return "is even"; }
  void ApplyTo(MInteger& other) const { throw "unimplemented"; }
};

struct Positive {
  ConstraintViolation CheckValue(AnalyzeVariableContext ctx, int value) const {
    if (value > 0) return ConstraintViolation::None();
    return ConstraintViolation(std::format("`{}` is not positive", value));
  }
  std::string ToString() const { return "is positive"; }
  void ApplyTo(MInteger& other) const { throw "unimplemented"; }
};

TEST(ConstraintHandlerTest, EmptyHandlerShouldBeSatisfiedWithEverything) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  AnalyzeVariableContext ctx("X", variables, values);

  {
    ConstraintHandler<MInteger, int> handler;
    EXPECT_THAT(handler.CheckValue(ctx, 5), HasNoConstraintViolation());
    EXPECT_THAT(handler.CheckValue(ctx, 0), HasNoConstraintViolation());
  }
  {
    ConstraintHandler<MString, std::string> handler;
    EXPECT_THAT(handler.CheckValue(ctx, "hello"), HasNoConstraintViolation());
    EXPECT_THAT(handler.CheckValue(ctx, ""), HasNoConstraintViolation());
  }
}

TEST(ConstraintHandlerTest, ToStringShouldWork) {
  // These tests are a bit fragile...
  {
    ConstraintHandler<MInteger, int> handler;
    EXPECT_EQ(handler.ToString(), "has no constraints");
  }
  {
    ConstraintHandler<MInteger, int> handler;
    handler.AddConstraint(Even());
    EXPECT_EQ(handler.ToString(), "is even");
  }
  {
    ConstraintHandler<MInteger, int> handler;
    handler.AddConstraint(Even());
    handler.AddConstraint(Positive());
    EXPECT_EQ(handler.ToString(), "is even, is positive");
  }
}

TEST(ConstraintHandlerTest, CheckValueShouldContainRelevantMessages) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  AnalyzeVariableContext ctx("X", variables, values);

  ConstraintHandler<MInteger, int> handler;
  handler.AddConstraint(Even());
  handler.AddConstraint(Positive());

  EXPECT_THAT(
      handler.CheckValue(ctx, -5),
      HasConstraintViolation(AnyOf(HasSubstr("even"), HasSubstr("positive"))));
  EXPECT_THAT(handler.CheckValue(ctx, 5),
              HasConstraintViolation(
                  AllOf(HasSubstr("even"), Not(HasSubstr("positive")))));
  EXPECT_THAT(handler.CheckValue(ctx, 0),
              HasConstraintViolation(
                  AllOf(Not(HasSubstr("even")), HasSubstr("positive"))));
  EXPECT_THAT(handler.CheckValue(ctx, 10), HasNoConstraintViolation());
}

TEST(ConstraintHandlerTest, CheckValueShouldReturnIfAnyFail) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  AnalyzeVariableContext ctx("X", variables, values);

  ConstraintHandler<MInteger, int> handler;
  handler.AddConstraint(Even());
  handler.AddConstraint(Positive());

  EXPECT_THAT(
      handler.CheckValue(ctx, -5),
      HasConstraintViolation(AnyOf(HasSubstr("even"), HasSubstr("positive"))));
  EXPECT_THAT(handler.CheckValue(ctx, 5),
              HasConstraintViolation(HasSubstr("even")));
  EXPECT_THAT(handler.CheckValue(ctx, 0),
              HasConstraintViolation(HasSubstr("positive")));
  EXPECT_THAT(handler.CheckValue(ctx, 10), HasNoConstraintViolation());
}

}  // namespace
}  // namespace librarian
}  // namespace moriarty
