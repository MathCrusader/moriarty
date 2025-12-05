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

#include "src/constraints/custom_constraint.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/context.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/testing/gtest_helpers.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace {

using ::moriarty_testing::HasConstraintViolation;
using ::moriarty_testing::HasNoConstraintViolation;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::IsEmpty;

TEST(CustomConstraintTest, BasicGettersWork) {
  {  // No dependent variables
    moriarty_internal::VariableSet variables;
    moriarty_internal::ValueSet values;
    ConstraintContext ctx("X", variables, values);

    CustomConstraint<MInteger> constraint("positive",
                                          [](int64_t x) { return x > 0; });
    EXPECT_EQ(constraint.GetName(), "positive");
    EXPECT_THAT(constraint.GetDependencies(), IsEmpty());
    EXPECT_EQ(constraint.ToString(), "[CustomConstraint] positive");
    EXPECT_EQ(constraint.CheckValue(ctx, -10).Reason(),
              "`-10` does not satisfy the custom constraint `positive`");
  }
  {  // With context
    moriarty_internal::VariableSet variables;
    moriarty_internal::ValueSet values;
    values.Set<MInteger>("N", 10);
    ConstraintContext ctx("X", variables, values);

    CustomConstraint<MInteger> constraint(
        "bigger_than_N", {"N"}, [](ConstraintContext ctx, int64_t x) {
          return x > ctx.GetValue<MInteger>("N");
        });
    EXPECT_EQ(constraint.GetName(), "bigger_than_N");
    EXPECT_THAT(constraint.GetDependencies(), ElementsAre("N"));
    EXPECT_EQ(constraint.ToString(), "[CustomConstraint] bigger_than_N");
    EXPECT_EQ(constraint.CheckValue(ctx, 5).Reason(),
              "`5` does not satisfy the custom constraint `bigger_than_N`");
  }
}

TEST(CustomConstraintTest, CheckValueShouldWork) {
  {  // No dependent variables
    moriarty_internal::VariableSet variables;
    moriarty_internal::ValueSet values;
    ConstraintContext ctx("X", variables, values);

    CustomConstraint<MInteger> constraint("positive",
                                          [](int64_t x) { return x > 0; });
    EXPECT_THAT(constraint.CheckValue(ctx, 10), HasNoConstraintViolation());
    EXPECT_THAT(constraint.CheckValue(ctx, 0),
                HasConstraintViolation(HasSubstr("positive")));
    EXPECT_THAT(constraint.CheckValue(ctx, -10),
                HasConstraintViolation(HasSubstr("positive")));
  }
  {  // With context
    moriarty_internal::VariableSet variables;
    moriarty_internal::ValueSet values;
    values.Set<MInteger>("N", 10);
    ConstraintContext ctx("X", variables, values);

    CustomConstraint<MInteger> constraint(
        "bigger_than_N", {"N"}, [](ConstraintContext ctx, int64_t x) {
          return x > ctx.GetValue<MInteger>("N");
        });
    EXPECT_THAT(constraint.CheckValue(ctx, 11), HasNoConstraintViolation());
    EXPECT_THAT(constraint.CheckValue(ctx, 10),
                HasConstraintViolation(HasSubstr("bigger_than_N")));
    EXPECT_THAT(constraint.CheckValue(ctx, 9),
                HasConstraintViolation(HasSubstr("bigger_than_N")));
  }
}

}  // namespace
}  // namespace moriarty
