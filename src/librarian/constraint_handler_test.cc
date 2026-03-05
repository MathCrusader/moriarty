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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/constraints/constraint_violation.h"
#include "src/context.h"
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
using ::moriarty_testing::HasNoViolation;
using ::moriarty_testing::HasViolation;
using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::HasSubstr;
using ::testing::Not;

struct Even {
  ValidationResult Validate(ConstraintContext ctx, int value) const {
    if (value % 2 == 0) return ValidationResult::Ok();
    return ValidationResult::Violation(ctx.GetVariableName(), value,
                                       librarian::Expected("even"));
  }
  std::string ToString() const { return "is even"; }
  void ApplyTo(MInteger& other) const { throw "unimplemented"; }
};

struct Positive {
  ValidationResult Validate(ConstraintContext ctx, int value) const {
    if (value > 0) return ValidationResult::Ok();
    return ValidationResult::Violation(ctx.GetVariableName(), value,
                                       librarian::Expected("positive"));
  }
  std::string ToString() const { return "is positive"; }
  void ApplyTo(MInteger& other) const { throw "unimplemented"; }
};

TEST(ConstraintHandlerTest, EmptyHandlerShouldBeSatisfiedWithEverything) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  ConstraintContext ctx("X", variables, values);

  {
    ConstraintHandler<MInteger, int> handler;
    EXPECT_THAT(handler.Validate(ctx, 5), HasNoViolation());
    EXPECT_THAT(handler.Validate(ctx, 0), HasNoViolation());
  }
  {
    ConstraintHandler<MString, std::string> handler;
    EXPECT_THAT(handler.Validate(ctx, "hello"), HasNoViolation());
    EXPECT_THAT(handler.Validate(ctx, ""), HasNoViolation());
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

TEST(ConstraintHandlerTest, ValidateShouldContainRelevantMessages) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  ConstraintContext ctx("X", variables, values);

  ConstraintHandler<MInteger, int> handler;
  handler.AddConstraint(Even());
  handler.AddConstraint(Positive());

  EXPECT_THAT(handler.Validate(ctx, -5),
              HasViolation(AnyOf(HasSubstr("even"), HasSubstr("positive"))));
  EXPECT_THAT(
      handler.Validate(ctx, 5),
      HasViolation(AllOf(HasSubstr("even"), Not(HasSubstr("positive")))));
  EXPECT_THAT(
      handler.Validate(ctx, 0),
      HasViolation(AllOf(Not(HasSubstr("even")), HasSubstr("positive"))));
  EXPECT_THAT(handler.Validate(ctx, 10), HasNoViolation());
}

TEST(ConstraintHandlerTest, ValidateShouldReturnIfAnyFail) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  ConstraintContext ctx("X", variables, values);

  ConstraintHandler<MInteger, int> handler;
  handler.AddConstraint(Even());
  handler.AddConstraint(Positive());

  EXPECT_THAT(handler.Validate(ctx, -5),
              HasViolation(AnyOf(HasSubstr("even"), HasSubstr("positive"))));
  EXPECT_THAT(handler.Validate(ctx, 5), HasViolation(HasSubstr("even")));
  EXPECT_THAT(handler.Validate(ctx, 0), HasViolation(HasSubstr("positive")));
  EXPECT_THAT(handler.Validate(ctx, 10), HasNoViolation());
}

}  // namespace
}  // namespace librarian
}  // namespace moriarty
