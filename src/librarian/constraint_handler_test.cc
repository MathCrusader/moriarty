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
#include "src/contexts/librarian/analysis_context.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"

namespace moriarty {
namespace librarian {
namespace {

using ::testing::AllOf;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Not;

struct Even {
  bool IsSatisfiedWith(AnalysisContext ctx, int value) const {
    return value % 2 == 0;
  }
  std::string Explanation(AnalysisContext ctx, int value) const {
    return std::format("`{}` is not even", value);
  }
  std::string ToString() const { return "is even"; }
};

struct Positive {
  bool IsSatisfiedWith(AnalysisContext ctx, int value) const {
    return value > 0;
  }
  std::string Explanation(AnalysisContext ctx, int value) const {
    return std::format("`{}` is not positive", value);
  }
  std::string ToString() const { return "is positive"; }
};

TEST(ConstraintHandlerTest, EmptyHandlerShouldBeSatisfiedWithEverything) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  AnalysisContext ctx("X", variables, values);

  {
    ConstraintHandler<int> handler("");
    EXPECT_TRUE(handler.IsSatisfiedWith(ctx, 5));
    EXPECT_TRUE(handler.IsSatisfiedWith(ctx, 0));
  }
  {
    ConstraintHandler<std::string> handler("");
    EXPECT_TRUE(handler.IsSatisfiedWith(ctx, "hello"));
    EXPECT_TRUE(handler.IsSatisfiedWith(ctx, ""));
  }
}

TEST(ConstraintHandlerTest, ToStringShouldBePrefixed) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  AnalysisContext ctx("X", variables, values);

  {
    ConstraintHandler<int> handler("prefix");
    EXPECT_EQ(handler.ToString(), "prefix\n");
  }
  {
    ConstraintHandler<int> handler("prefix");
    handler.AddConstraint(Even());
    EXPECT_THAT(handler.ToString(), testing::StartsWith("prefix\n"));
  }
}

TEST(ConstraintHandlerTest, ToStringShouldWork) {
  {
    ConstraintHandler<int> handler("prefix");
    EXPECT_EQ(handler.ToString(), "prefix\n");
  }
  {
    ConstraintHandler<int> handler("prefix");
    handler.AddConstraint(Even());
    handler.AddConstraint(Positive());
    EXPECT_EQ(handler.ToString(), R"(prefix
 * is even
 * is positive
)");
  }
}

TEST(ConstraintHandlerTest, ExplanationShouldBeEmptyForSatisfiedConstraints) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  AnalysisContext ctx("X", variables, values);

  {
    ConstraintHandler<int> handler("");
    EXPECT_TRUE(handler.IsSatisfiedWith(ctx, 5));
    EXPECT_THAT(handler.Explanation(ctx, 5), IsEmpty());
  }
  {
    ConstraintHandler<std::string> handler("");
    EXPECT_TRUE(handler.IsSatisfiedWith(ctx, "hello"));
    EXPECT_THAT(handler.Explanation(ctx, "hello"), IsEmpty());
  }
}

TEST(ConstraintHandlerTest, ExplanationShouldContainRelevantMessages) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  AnalysisContext ctx("X", variables, values);

  ConstraintHandler<int> handler("");
  handler.AddConstraint(Even());
  handler.AddConstraint(Positive());

  EXPECT_THAT(handler.Explanation(ctx, -5),
              AllOf(HasSubstr("even"), HasSubstr("positive")));
  EXPECT_THAT(handler.Explanation(ctx, 5),
              AllOf(HasSubstr("even"), Not(HasSubstr("positive"))));
  EXPECT_THAT(handler.Explanation(ctx, 0),
              AllOf(Not(HasSubstr("even")), HasSubstr("positive")));
  EXPECT_THAT(handler.Explanation(ctx, 10),
              AllOf(Not(HasSubstr("even")), Not(HasSubstr("positive"))));
}

TEST(ConstraintHandlerTest, IsSatisfiedWithShouldReturnIfAnyFail) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  AnalysisContext ctx("X", variables, values);

  ConstraintHandler<int> handler("");
  handler.AddConstraint(Even());
  handler.AddConstraint(Positive());

  EXPECT_FALSE(handler.IsSatisfiedWith(ctx, -5));
  EXPECT_FALSE(handler.IsSatisfiedWith(ctx, 5));
  EXPECT_FALSE(handler.IsSatisfiedWith(ctx, 0));
  EXPECT_TRUE(handler.IsSatisfiedWith(ctx, 10));
}

}  // namespace
}  // namespace librarian
}  // namespace moriarty
