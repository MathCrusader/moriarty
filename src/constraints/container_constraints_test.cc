// Copyright 2025 Darcy Best
// Copyright 2024 Google LLC
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
using ::moriarty_testing::HasNoViolation;
using ::moriarty_testing::HasViolation;
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
  ConstraintContext ctx("N", variables, values);

  EXPECT_THAT(
      Length(Between(1, 10)).Validate(ctx, std::string("this string is long")),
      HasViolation(
          AllOf(HasSubstr("length"), HasSubstr("expected: between 1 and 10"))));
  EXPECT_THAT(Length(Between(3, 10)).Validate(ctx, std::vector<int>{1, 2}),
              HasViolation(AllOf(HasSubstr("length"),
                                 HasSubstr("expected: between 3 and 10"))));
}

TEST(ContainerConstraintsTest, LengthIsSatisfiedWithWorks) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  ConstraintContext ctx("N", variables, values);

  EXPECT_THAT(Length(Between(1, 10)).Validate(ctx, std::string("123456")),
              HasNoViolation());
  EXPECT_THAT(Length(Between(1, 10)).Validate(ctx, std::string("1")),
              HasNoViolation());
  EXPECT_THAT(Length(Between(1, 10)).Validate(ctx, std::string("1234567890")),
              HasNoViolation());
  EXPECT_THAT(Length(Between(1, 10)).Validate(ctx, std::vector{1, 2, 3}),
              HasNoViolation());

  EXPECT_THAT(Length(Between(1, 10)).Validate(ctx, std::string("")),
              HasViolation(HasSubstr("length")));
  EXPECT_THAT(Length(Between(1, 10)).Validate(ctx, std::vector<int>{}),
              HasViolation(HasSubstr("length")));
  EXPECT_THAT(Length(Between(2, 3)).Validate(ctx, std::vector{1}),
              HasViolation(HasSubstr("length")));
}

TEST(ContainerConstraintsTest, ElementsConstraintsAreCorrect) {
  EXPECT_THAT(StronglyTypedElements<MInteger>(Between(1, 10)).GetConstraints(),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
  EXPECT_THAT(StronglyTypedElements<MInteger>(AtLeast("X"), AtMost(15))
                  .GetConstraints(),
              GeneratedValuesAre(AllOf(Ge(3), Le(15)),
                                 Context().WithValue<MInteger>("X", 3)));
}

TEST(ContainerConstraintsTest, ElementsToStringWorks) {
  EXPECT_EQ(StronglyTypedElements<MInteger>(Between(1, 10)).ToString(),
            "each element is between 1 and 10");
  EXPECT_EQ(StronglyTypedElements<MString>(Length(Between(1, 10))).ToString(),
            "each element has length that is between 1 and 10");
}

TEST(ContainerConstraintsTest, ElementsUnsatisfiedReasonWorks) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  ConstraintContext ctx("N", variables, values);

  EXPECT_THAT(
      StronglyTypedElements<MInteger>(Between(1, 10)).Validate(ctx, {-1, 2, 3}),
      HasViolation(AllOf(HasSubstr("index 0"),
                         HasSubstr("expected: between 1 and 10"))));
  EXPECT_THAT(
      StronglyTypedElements<MString>(Length(Between(3, 10)))
          .Validate(ctx, std::vector<std::string>{"hello", "moto", "me"}),
      HasViolation(AllOf(HasSubstr("index 2"),
                         HasSubstr("expected: between 3 and 10"))));
}

TEST(ContainerConstraintsTest, ElementsIsSatisfiedWithWorks) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  values.Set<MInteger>("X", 10);
  ConstraintContext ctx("N", variables, values);

  {
    EXPECT_THAT(StronglyTypedElements<MString>(Length(Between(1, 10)))
                    .Validate(ctx, std::vector<std::string>{}),
                HasNoViolation());
    EXPECT_THAT(StronglyTypedElements<MString>(Length(Between(1, 10)))
                    .Validate(ctx, std::vector<std::string>{"hello", "moto"}),
                HasNoViolation());
    EXPECT_THAT(StronglyTypedElements<MString>(Length(Between(1, "X")))
                    .Validate(ctx, std::vector<std::string>{"hello", "moto"}),
                HasNoViolation());
  }
  {
    EXPECT_THAT(StronglyTypedElements<MInteger>(Between(1, 10))
                    .Validate(ctx, std::vector<int64_t>{1, 2, 3}),
                HasNoViolation());
    EXPECT_THAT(StronglyTypedElements<MInteger>(Between(1, "X"))
                    .Validate(ctx, std::vector<int64_t>{1}),
                HasNoViolation());
  }
  {
    EXPECT_THAT(StronglyTypedElements<MString>(Length(Between(1, 4)))
                    .Validate(ctx, std::vector<std::string>{"hello"}),
                HasViolation(HasSubstr("length")));
    EXPECT_THAT(StronglyTypedElements<MString>(Length(Between(2, 10)))
                    .Validate(ctx, std::vector<std::string>{"hello", "m"}),
                HasViolation(HasSubstr("length")));
  }
  {
    EXPECT_THAT(StronglyTypedElements<MInteger>(Between(1, 10))
                    .Validate(ctx, std::vector<int64_t>{1, 2, 11}),
                HasViolation(AllOf(HasSubstr("index 2"),
                                   HasSubstr("between 1 and 10"))));
    EXPECT_THAT(StronglyTypedElements<MInteger>(Between(1, "X"))
                    .Validate(ctx, std::vector<int64_t>{1, 2, 11}),
                HasViolation(
                    AllOf(HasSubstr("index 2"), HasSubstr("between 1 and X"))));
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
  ConstraintContext ctx("N", variables, values);

  EXPECT_THAT((Element<0, MInteger>(Between(1, 10)).Validate(ctx, -1)),
              HasViolation("between 1 and 10"));
  EXPECT_THAT(
      (Element<12, MString>(Length(Between(1, 3))).Validate(ctx, "hello")),
      HasViolation(AllOf(HasSubstr("length"), HasSubstr("between 1 and 3"))));
}

TEST(ContainerConstraintsTest, ElementIsSatisfiedWithWorks) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  values.Set<MInteger>("X", 10);
  ConstraintContext ctx("N", variables, values);

  {
    EXPECT_THAT(
        (Element<0, MString>(Length(Between(1, 10))).Validate(ctx, "123")),
        HasNoViolation());
    EXPECT_THAT(
        (Element<0, MString>(Length(Between(1, 10))).Validate(ctx, "hello")),
        HasNoViolation());
    EXPECT_THAT(
        (Element<0, MString>(Length(Between(1, "X"))).Validate(ctx, "moto")),
        HasNoViolation());
  }
  {
    EXPECT_THAT(
        (Element<0, MString>(Length(Between(1, 4))).Validate(ctx, "hello")),
        HasViolation(HasSubstr("length")));
    EXPECT_THAT(
        (Element<0, MString>(Length(Between(2, "X"))).Validate(ctx, "m")),
        HasViolation(HasSubstr("length")));
  }
}

TEST(ContainerConstraintsTest, DistinctElementsToStringWorks) {
  EXPECT_EQ(DistinctElements().ToString(), "has distinct elements");
}

TEST(ContainerConstraintsTest, DistinctElementsUnsatisfiedReasonWorks) {
  EXPECT_THAT(DistinctElements().Validate(std::vector{11, 22, 11}),
              HasViolation(AllOf(HasSubstr("array indices 0 and 2"),
                                 HasSubstr("distinct elements"))));
  EXPECT_THAT(DistinctElements().Validate(
                  std::vector<std::string>{"hello", "hell", "help", "hell"}),
              HasViolation(AllOf(HasSubstr("array indices 1 and 3"),
                                 HasSubstr("distinct elements"))));
  EXPECT_THAT(DistinctElements().Validate(std::vector{1, 2, 3, 2, 2, 3}),
              HasViolation(AllOf(HasSubstr("array indices 1 and 3"),
                                 HasSubstr("distinct elements"))));
}

TEST(ContainerConstraintsTest, DistinctElementsIsSatisfiedWithWorks) {
  {
    EXPECT_THAT(DistinctElements().Validate(std::vector<std::string>{}),
                HasNoViolation());
    EXPECT_THAT(
        DistinctElements().Validate(std::vector<std::string>{"hello", "moto"}),
        HasNoViolation());
  }
  {
    EXPECT_THAT(DistinctElements().Validate(std::vector<std::string>{"a", "a"}),
                HasViolation(AllOf(HasSubstr("array indices 0 and 1"),
                                   HasSubstr("distinct elements"))));
    EXPECT_THAT(
        DistinctElements().Validate(std::vector<std::string>{"a", "ba", "ba"}),
        HasViolation(AllOf(HasSubstr("array indices 1 and 2"),
                           HasSubstr("distinct elements"))));
  }
}

TEST(ContainerConstraintsTest, SortedWorks) {
  {
    EXPECT_THAT(Sorted<MString>().Validate(std::vector<std::string>{}),
                HasNoViolation());
    EXPECT_THAT((Sorted<MString, std::greater<>>().Validate(
                    std::vector<std::string>{"moto", "hello"})),
                HasNoViolation());
    EXPECT_THAT(
        (Sorted<MString, std::greater<>, std::function<char(std::string)>>(
             std::greater{}, [](std::string s) { return s[1]; })
             .Validate(std::vector<std::string>{"axc", "zby"})),
        HasNoViolation());
  }
  {
    EXPECT_THAT((Sorted<MInteger>().Validate({1, 2, 3, 3, 4})),
                HasNoViolation());
    EXPECT_THAT((Sorted<MInteger, std::greater<>>().Validate({1, 1, 1, 1})),
                HasNoViolation());
  }
  {
    EXPECT_THAT((Sorted<MInteger>().Validate({4, 3, 2, 1})),
                HasViolation(HasSubstr("sorted")));
    EXPECT_THAT((Sorted<MInteger>().Validate({2, 2, 2, 2, 1})),
                HasViolation(HasSubstr("sorted")));
  }
}

}  // namespace
}  // namespace moriarty
