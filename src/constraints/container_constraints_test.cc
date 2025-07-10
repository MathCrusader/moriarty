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

#include "src/constraints/container_constraints.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/constraints/numeric_constraints.h"
#include "src/contexts/librarian_context.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/testing/gtest_helpers.h"
#include "src/variables/minteger.h"
#include "src/variables/mstring.h"

namespace moriarty {
namespace {

using ::moriarty_testing::Context;
using ::moriarty_testing::GeneratedValuesAre;
using ::moriarty_testing::HasConstraintViolation;
using ::moriarty_testing::HasNoConstraintViolation;
using ::testing::AllOf;
using ::testing::Ge;
using ::testing::HasSubstr;
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
                .CheckValue(ctx, std::string("this string is long"))
                .Reason(),
            "has length (which is `19`) that is not between 1 and 10");
  EXPECT_EQ(
      Length(Between(3, 10)).CheckValue(ctx, std::vector<int>{1, 2}).Reason(),
      "has length (which is `2`) that is not between 3 and 10");
}

TEST(ContainerConstraintsTest, LengthIsSatisfiedWithWorks) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  librarian::AnalysisContext ctx("N", variables, values);

  EXPECT_THAT(Length(Between(1, 10)).CheckValue(ctx, std::string("123456")),
              HasNoConstraintViolation());
  EXPECT_THAT(Length(Between(1, 10)).CheckValue(ctx, std::string("1")),
              HasNoConstraintViolation());
  EXPECT_THAT(Length(Between(1, 10)).CheckValue(ctx, std::string("1234567890")),
              HasNoConstraintViolation());
  EXPECT_THAT(Length(Between(1, 10)).CheckValue(ctx, std::vector{1, 2, 3}),
              HasNoConstraintViolation());

  EXPECT_THAT(Length(Between(1, 10)).CheckValue(ctx, std::string("")),
              HasConstraintViolation(HasSubstr("length")));
  EXPECT_THAT(Length(Between(1, 10)).CheckValue(ctx, std::vector<int>{}),
              HasConstraintViolation(HasSubstr("length")));
  EXPECT_THAT(Length(Between(2, 3)).CheckValue(ctx, std::vector{1}),
              HasConstraintViolation(HasSubstr("length")));
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
      Elements<MInteger>(Between(1, 10)).CheckValue(ctx, {-1, 2, 3}).Reason(),
      "array index 0 (which is `-1`) is not between 1 and 10");
  EXPECT_EQ(
      Elements<MString>(Length(Between(3, 10)))
          .CheckValue(ctx, std::vector<std::string>{"hello", "moto", "me"})
          .Reason(),
      "array index 2 (which is `me`) has length (which is `2`) that is not "
      "between 3 and 10");
}

TEST(ContainerConstraintsTest, ElementsIsSatisfiedWithWorks) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  values.Set<MInteger>("X", 10);
  librarian::AnalysisContext ctx("N", variables, values);

  {
    EXPECT_THAT(Elements<MString>(Length(Between(1, 10)))
                    .CheckValue(ctx, std::vector<std::string>{}),
                HasNoConstraintViolation());
    EXPECT_THAT(Elements<MString>(Length(Between(1, 10)))
                    .CheckValue(ctx, std::vector<std::string>{"hello", "moto"}),
                HasNoConstraintViolation());
    EXPECT_THAT(Elements<MString>(Length(Between(1, "X")))
                    .CheckValue(ctx, std::vector<std::string>{"hello", "moto"}),
                HasNoConstraintViolation());
  }
  {
    EXPECT_THAT(Elements<MInteger>(Between(1, 10))
                    .CheckValue(ctx, std::vector<int64_t>{1, 2, 3}),
                HasNoConstraintViolation());
    EXPECT_THAT(Elements<MInteger>(Between(1, "X"))
                    .CheckValue(ctx, std::vector<int64_t>{1}),
                HasNoConstraintViolation());
  }
  {
    EXPECT_THAT(Elements<MString>(Length(Between(1, 4)))
                    .CheckValue(ctx, std::vector<std::string>{"hello"}),
                HasConstraintViolation(HasSubstr("length")));
    EXPECT_THAT(Elements<MString>(Length(Between(2, 10)))
                    .CheckValue(ctx, std::vector<std::string>{"hello", "m"}),
                HasConstraintViolation(HasSubstr("length")));
  }
  {
    EXPECT_THAT(Elements<MInteger>(Between(1, 10))
                    .CheckValue(ctx, std::vector<int64_t>{1, 2, 11}),
                HasConstraintViolation(HasSubstr("is not between")));
    EXPECT_THAT(Elements<MInteger>(Between(1, "X"))
                    .CheckValue(ctx, std::vector<int64_t>{1, 2, 11}),
                HasConstraintViolation(HasSubstr("is not between")));
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

  EXPECT_EQ((Element<0, MInteger>(Between(1, 10)).CheckValue(ctx, -1).Reason()),
            "tuple index 0 (which is `-1`) is not between 1 and 10");
  EXPECT_EQ(
      (Element<12, MString>(Length(Between(1, 3)))
           .CheckValue(ctx, "hello")
           .Reason()),
      "tuple index 12 (which is `hello`) has length (which is `5`) that is not "
      "between 1 and 3");
}

TEST(ContainerConstraintsTest, ElementIsSatisfiedWithWorks) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  values.Set<MInteger>("X", 10);
  librarian::AnalysisContext ctx("N", variables, values);

  {
    EXPECT_THAT(
        (Element<0, MString>(Length(Between(1, 10))).CheckValue(ctx, "123")),
        HasNoConstraintViolation());
    EXPECT_THAT(
        (Element<0, MString>(Length(Between(1, 10))).CheckValue(ctx, "hello")),
        HasNoConstraintViolation());
    EXPECT_THAT(
        (Element<0, MString>(Length(Between(1, "X"))).CheckValue(ctx, "moto")),
        HasNoConstraintViolation());
  }
  {
    EXPECT_THAT(
        (Element<0, MString>(Length(Between(1, 4))).CheckValue(ctx, "hello")),
        HasConstraintViolation(HasSubstr("length")));
    EXPECT_THAT(
        (Element<0, MString>(Length(Between(2, "X"))).CheckValue(ctx, "m")),
        HasConstraintViolation(HasSubstr("length")));
  }
}

TEST(ContainerConstraintsTest, DistinctElementsToStringWorks) {
  EXPECT_EQ(DistinctElements().ToString(), "has distinct elements");
}

TEST(ContainerConstraintsTest, DistinctElementsUnsatisfiedReasonWorks) {
  EXPECT_THAT(DistinctElements().CheckValue(std::vector{11, 22, 11}),
              HasConstraintViolation(HasSubstr("not distinct")));
  EXPECT_THAT(DistinctElements().CheckValue(
                  std::vector<std::string>{"hello", "hell", "help", "hell"}),
              HasConstraintViolation(HasSubstr("not distinct")));
  EXPECT_THAT(DistinctElements().CheckValue(std::vector{1, 2, 3, 2, 2, 3}),
              HasConstraintViolation(HasSubstr("not distinct")));
}

TEST(ContainerConstraintsTest, DistinctElementsIsSatisfiedWithWorks) {
  {
    EXPECT_THAT(DistinctElements().CheckValue(std::vector<std::string>{}),
                HasNoConstraintViolation());
    EXPECT_THAT(DistinctElements().CheckValue(
                    std::vector<std::string>{"hello", "moto"}),
                HasNoConstraintViolation());
  }
  {
    EXPECT_THAT(
        DistinctElements().CheckValue(std::vector<std::string>{"a", "a"}),
        HasConstraintViolation(HasSubstr("not distinct")));
    EXPECT_THAT(DistinctElements().CheckValue(
                    std::vector<std::string>{"a", "ba", "ba"}),
                HasConstraintViolation(HasSubstr("not distinct")));
  }
}

TEST(ContainerConstraintsTest, SortedWorks) {
  {
    EXPECT_THAT(Sorted<MString>().CheckValue(std::vector<std::string>{}),
                HasNoConstraintViolation());
    EXPECT_THAT((Sorted<MString, std::greater<>>().CheckValue(
                    std::vector<std::string>{"moto", "hello"})),
                HasNoConstraintViolation());
    EXPECT_THAT(
        (Sorted<MString, std::greater<>, std::function<char(std::string)>>(
             std::greater{}, [](std::string s) { return s[1]; })
             .CheckValue(std::vector<std::string>{"axc", "zby"})),
        HasNoConstraintViolation());
  }
  {
    EXPECT_THAT((Sorted<MInteger>().CheckValue({1, 2, 3, 3, 4})),
                HasNoConstraintViolation());
    EXPECT_THAT((Sorted<MInteger, std::greater<>>().CheckValue({1, 1, 1, 1})),
                HasNoConstraintViolation());
  }
  {
    EXPECT_THAT((Sorted<MInteger>().CheckValue({4, 3, 2, 1})),
                HasConstraintViolation(HasSubstr("sorted")));
    EXPECT_THAT((Sorted<MInteger>().CheckValue({2, 2, 2, 2, 1})),
                HasConstraintViolation(HasSubstr("sorted")));
  }
}

}  // namespace
}  // namespace moriarty
