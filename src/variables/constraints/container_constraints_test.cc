/*
 * Copyright 2025 Darcy Best
 * Copyright 2024 Google LLC
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

#include "src/variables/constraints/container_constraints.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/contexts/librarian/analysis_context.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/test_utils.h"
#include "src/variables/constraints/numeric_constraints.h"
#include "src/variables/minteger.h"
#include "src/variables/mstring.h"

namespace moriarty {
namespace {

using ::moriarty_testing::Context;
using ::moriarty_testing::GeneratedValuesAre;
using ::testing::AllOf;
using ::testing::Ge;
using ::testing::Le;

TEST(ContainerConstraintsTest, LengthConstraintsAreCorrect) {
  EXPECT_THAT(Length(10).GetConstraints(), GeneratedValuesAre(10));
  EXPECT_THAT(Length("2 * N").GetConstraints(),
              GeneratedValuesAre(14, Context().WithValue<MInteger>("N", 7)));
  EXPECT_THAT(Length(AtLeast("X"), AtMost(15)).GetConstraints(),
              GeneratedValuesAre(AllOf(Ge(3), Le(15)),
                                 Context().WithValue<MInteger>("X", 3)));
}

TEST(ContainerConstraintsTest, LengthToStringWorks) {
  EXPECT_EQ(Length(Between(1, 10)).ToString(),
            "has length that is between 1 and 10");
}

TEST(ContainerConstraintsTest, LengthUnsatisfiedReasonWorks) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  librarian::AnalysisContext ctx("N", variables, values);

  EXPECT_EQ(Length(Between(1, 10))
                .UnsatisfiedReason(ctx, std::string("this string is long")),
            "has length (which is `19`) that is not between 1 and 10");
  EXPECT_EQ(
      Length(Between(3, 10)).UnsatisfiedReason(ctx, std::vector<int>{1, 2}),
      "has length (which is `2`) that is not between 3 and 10");
}

TEST(ContainerConstraintsTest, LengthIsSatisfiedWithWorks) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  librarian::AnalysisContext ctx("N", variables, values);

  EXPECT_TRUE(
      Length(Between(1, 10)).IsSatisfiedWith(ctx, std::string("123456")));
  EXPECT_TRUE(Length(Between(1, 10)).IsSatisfiedWith(ctx, std::string("1")));
  EXPECT_TRUE(
      Length(Between(1, 10)).IsSatisfiedWith(ctx, std::string("1234567890")));
  EXPECT_TRUE(
      Length(Between(1, 10)).IsSatisfiedWith(ctx, std::vector{1, 2, 3}));

  EXPECT_FALSE(Length(Between(1, 10)).IsSatisfiedWith(ctx, std::string("")));
  EXPECT_FALSE(Length(Between(1, 10)).IsSatisfiedWith(ctx, std::vector<int>{}));
  EXPECT_FALSE(Length(Between(2, 3)).IsSatisfiedWith(ctx, std::vector{1}));
}

TEST(ContainerConstraintsTest, ElementsConstraintsAreCorrect) {
  EXPECT_THAT(Elements<MInteger>(Between(1, 10)).GetConstraints(),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
  EXPECT_THAT(Elements<MInteger>(AtLeast("X"), AtMost(15)).GetConstraints(),
              GeneratedValuesAre(AllOf(Ge(3), Le(15)),
                                 Context().WithValue<MInteger>("X", 3)));
}

TEST(ContainerConstraintsTest, ElementsToStringWorks) {
  EXPECT_EQ(Elements<MInteger>(Between(1, 10)).ToString(),
            "each element is between 1 and 10");
  EXPECT_EQ(Elements<MString>(Length(Between(1, 10))).ToString(),
            "each element has length that is between 1 and 10");
}

TEST(ContainerConstraintsTest, ElementsUnsatisfiedReasonWorks) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  librarian::AnalysisContext ctx("N", variables, values);

  EXPECT_EQ(
      Elements<MInteger>(Between(1, 10)).UnsatisfiedReason(ctx, {-1, 2, 3}),
      "array index 0 (which is `-1`) is not between 1 and 10");
  EXPECT_EQ(
      Elements<MString>(Length(Between(3, 10)))
          .UnsatisfiedReason(ctx,
                             std::vector<std::string>{"hello", "moto", "me"}),
      "array index 2 (which is `me`) has length (which is `2`) that is not "
      "between 3 and 10");
}

TEST(ContainerConstraintsTest, ElementsIsSatisfiedWithWorks) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  values.Set<MInteger>("X", 10);
  librarian::AnalysisContext ctx("N", variables, values);

  {
    EXPECT_TRUE(Elements<MString>(Length(Between(1, 10)))
                    .IsSatisfiedWith(ctx, std::vector<std::string>{}));
    EXPECT_TRUE(
        Elements<MString>(Length(Between(1, 10)))
            .IsSatisfiedWith(ctx, std::vector<std::string>{"hello", "moto"}));
    EXPECT_TRUE(
        Elements<MString>(Length(Between(1, "X")))
            .IsSatisfiedWith(ctx, std::vector<std::string>{"hello", "moto"}));
  }
  {
    EXPECT_TRUE(Elements<MInteger>(Between(1, 10))
                    .IsSatisfiedWith(ctx, std::vector<int64_t>{1, 2, 3}));
    EXPECT_TRUE(Elements<MInteger>(Between(1, "X"))
                    .IsSatisfiedWith(ctx, std::vector<int64_t>{1}));
  }
  {
    EXPECT_FALSE(Elements<MString>(Length(Between(1, 4)))
                     .IsSatisfiedWith(ctx, std::vector<std::string>{"hello"}));
    EXPECT_FALSE(
        Elements<MString>(Length(Between(2, 10)))
            .IsSatisfiedWith(ctx, std::vector<std::string>{"hello", "m"}));
  }
  {
    EXPECT_FALSE(Elements<MInteger>(Between(1, 10))
                     .IsSatisfiedWith(ctx, std::vector<int64_t>{1, 2, 11}));
    EXPECT_FALSE(Elements<MInteger>(Between(1, "X"))
                     .IsSatisfiedWith(ctx, std::vector<int64_t>{1, 2, 11}));
  }
}

TEST(ContainerConstraintsTest, ElementConstraintsAreCorrect) {
  EXPECT_THAT((Element<0, MInteger>(Between(1, 10)).GetConstraints()),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
  EXPECT_THAT((Element<1, MInteger>(AtLeast("X"), AtMost(15)).GetConstraints()),
              GeneratedValuesAre(AllOf(Ge(3), Le(15)),
                                 Context().WithValue<MInteger>("X", 3)));
}

TEST(ContainerConstraintsTest, ElementToStringWorks) {
  EXPECT_EQ((Element<50, MInteger>(Between(1, 10)).ToString()),
            "tuple index 50 is between 1 and 10");
  EXPECT_EQ((Element<0, MString>(Length(Between(1, 10))).ToString()),
            "tuple index 0 has length that is between 1 and 10");
}

TEST(ContainerConstraintsTest, ElementUnsatisfiedReasonWorks) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  librarian::AnalysisContext ctx("N", variables, values);

  EXPECT_EQ((Element<0, MInteger>(Between(1, 10)).UnsatisfiedReason(ctx, -1)),
            "tuple index 0 (which is `-1`) is not between 1 and 10");
  EXPECT_EQ(
      (Element<12, MString>(Length(Between(1, 3)))
           .UnsatisfiedReason(ctx, "hello")),
      "tuple index 12 (which is `hello`) has length (which is `5`) that is not "
      "between 1 and 3");
}

TEST(ContainerConstraintsTest, ElementIsSatisfiedWithWorks) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  values.Set<MInteger>("X", 10);
  librarian::AnalysisContext ctx("N", variables, values);

  {
    EXPECT_TRUE((Element<0, MString>(Length(Between(1, 10)))
                     .IsSatisfiedWith(ctx, "123")));
    EXPECT_TRUE((Element<0, MString>(Length(Between(1, 10)))
                     .IsSatisfiedWith(ctx, "hello")));
    EXPECT_TRUE((Element<0, MString>(Length(Between(1, "X")))
                     .IsSatisfiedWith(ctx, "moto")));
  }
  {
    EXPECT_FALSE((Element<0, MString>(Length(Between(1, 4)))
                      .IsSatisfiedWith(ctx, "hello")));
    EXPECT_FALSE((Element<0, MString>(Length(Between(2, "X")))
                      .IsSatisfiedWith(ctx, "m")));
  }
}

TEST(ContainerConstraintsTest, DistinctElementsToStringWorks) {
  EXPECT_EQ(DistinctElements().ToString(), "has distinct elements");
}

TEST(ContainerConstraintsTest, DistinctElementsUnsatisfiedReasonWorks) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  librarian::AnalysisContext ctx("N", variables, values);

  EXPECT_EQ(DistinctElements().UnsatisfiedReason(ctx, std::vector{11, 22, 11}),
            "array indices 0 and 2 (which are `11`) are not distinct");
  EXPECT_EQ(DistinctElements().UnsatisfiedReason(
                ctx, std::vector<std::string>{"hello", "hell", "help", "hell"}),
            "array indices 1 and 3 (which are `hell`) are not distinct");
  EXPECT_EQ(
      DistinctElements().UnsatisfiedReason(ctx, std::vector{1, 2, 3, 2, 2, 3}),
      "array indices 1 and 3 (which are `2`) are not distinct");
}

TEST(ContainerConstraintsTest, DistinctElementsIsSatisfiedWithWorks) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  librarian::AnalysisContext ctx("N", variables, values);

  {
    EXPECT_TRUE(
        DistinctElements().IsSatisfiedWith(ctx, std::vector<std::string>{}));
    EXPECT_TRUE(DistinctElements().IsSatisfiedWith(
        ctx, std::vector<std::string>{"hello", "moto"}));
  }
  {
    EXPECT_FALSE(DistinctElements().IsSatisfiedWith(
        ctx, std::vector<std::string>{"a", "a"}));
    EXPECT_FALSE(DistinctElements().IsSatisfiedWith(
        ctx, std::vector<std::string>{"a", "ba", "ba"}));
  }
}

}  // namespace
}  // namespace moriarty
