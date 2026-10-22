// Copyright 2026 Darcy Best
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

#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "moriarty/constraints/container_constraints.h"
#include "moriarty/constraints/equality_constraints.h"
#include "moriarty/context.h"
#include "moriarty/librarian/testing/gtest_helpers.h"
#include "moriarty/types/graph.h"
#include "moriarty/variables/marray.h"
#include "moriarty/variables/mstring.h"

namespace moriarty {
namespace {

using moriarty_testing::Context;
using moriarty_testing::HasNoViolation;
using moriarty_testing::HasViolation;

TEST(CaseNormalizationConstraintTest, IsSatisfiedWithUsesLowercase) {
  Context context;
  ConstraintContext ctx("S", context.Variables(), context.Values(),
                        {.case_normalization = CaseNormalization::kLowercase});
  EXPECT_TRUE(ctx.IsSatisfiedWith(MString(Exactly("YES")), "yEs"));
  EXPECT_TRUE(ctx.IsSatisfiedWith(MString(OneOf({"POSSIBLE", "IMPOSSIBLE"})),
                                  "Possible"));
  EXPECT_FALSE(
      ctx.IsSatisfiedWith(MString(OneOf({"POSSIBLE", "IMPOSSIBLE"})), "maybe"));
}

TEST(CaseNormalizationConstraintTest, IsSatisfiedWithUsesUppercase) {
  Context context;
  ConstraintContext ctx("S", context.Variables(), context.Values(),
                        {.case_normalization = CaseNormalization::kUppercase});
  EXPECT_TRUE(ctx.IsSatisfiedWith(MString(Exactly("yes")), "yEs"));
  EXPECT_TRUE(ctx.IsSatisfiedWith(MString(OneOf({"possible", "impossible"})),
                                  "Possible"));
  EXPECT_FALSE(ctx.IsSatisfiedWith(MString(Exactly("yes")), "no"));
}

TEST(CaseNormalizationConstraintTest, DefaultValidationRemainsExact) {
  Context context;
  ConstraintContext ctx("S", context.Variables(), context.Values(), {});
  EXPECT_FALSE(ctx.IsSatisfiedWith(MString(Exactly("YES")), "yes"));
  EXPECT_FALSE(ctx.IsSatisfiedWith(MString(OneOf({"YES", "NO"})), "yes"));
  EXPECT_TRUE(ctx.IsSatisfiedWith(MString(Exactly("YES")), "YES"));
}

TEST(CaseNormalizationConstraintTest, NestedEqualityPreservesStructure) {
  Context context;
  ConstraintContext ctx("A", context.Variables(), context.Values(),
                        {.case_normalization = CaseNormalization::kLowercase});
  using Choice = std::variant<std::string, std::string>;
  using Row = std::tuple<Choice, int>;
  EXPECT_THAT(
      Exactly(std::vector<Row>{{Choice(std::in_place_index<1>, "A"), 2}})
          .Validate(ctx, {{Choice(std::in_place_index<1>, "a"), 2}}),
      HasNoViolation());
  EXPECT_THAT(
      Exactly(std::vector<Row>{{Choice(std::in_place_index<1>, "A"), 2}})
          .Validate(ctx, {{Choice(std::in_place_index<0>, "a"), 2}}),
      HasViolation(testing::_));
  EXPECT_THAT(
      Exactly(std::vector<Row>{{Choice(std::in_place_index<1>, "A"), 2}})
          .Validate(ctx, {{Choice(std::in_place_index<1>, "a"), 3}}),
      HasViolation(testing::_));
}

TEST(CaseNormalizationConstraintTest, ArrayElementsInheritPolicy) {
  Context context;
  ConstraintContext ctx("A", context.Variables(), context.Values(),
                        {.case_normalization = CaseNormalization::kUppercase});
  EXPECT_TRUE(ctx.IsSatisfiedWith(
      MArray<MString>(Elements(OneOf({"yes", "no"}))), {"YES", "nO"}));
  EXPECT_FALSE(ctx.IsSatisfiedWith(
      MArray<MString>(Elements(OneOf({"yes", "no"}))), {"YES", "maybe"}));
}

TEST(CaseNormalizationConstraintTest, GraphEqualityNormalizesLabels) {
  Context context;
  ConstraintContext ctx("G", context.Variables(), context.Values(),
                        {.case_normalization = CaseNormalization::kLowercase});
  Graph<std::string, std::string> upper(2);
  Graph<std::string, std::string> lower(2);
  upper.AddEdge(0, 1, "EDGE");
  lower.AddEdge(0, 1, "edge");
  EXPECT_THAT(Exactly(upper).Validate(ctx, lower), HasNoViolation());
}

TEST(CaseNormalizationConstraintTest, EarlyOneOfChecksRemainExact) {
  EXPECT_THROW(MString(OneOf({"YES"}), OneOf({"yes"})), ImpossibleToSatisfy);
}

}  // namespace
}  // namespace moriarty
