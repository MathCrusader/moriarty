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

#include "src/variables/mgraph.h"

#include <optional>
#include <string_view>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/internal/variable_set.h"
#include "src/librarian/errors.h"
#include "src/testing/gtest_helpers.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/constraints/graph_constraints.h"
#include "src/variables/graph.h"

namespace moriarty {
namespace {

using moriarty_testing::Context;
using moriarty_testing::GeneratedValuesAre;
using moriarty_testing::GenerateThrowsGenerationError;
using moriarty_testing::Print;
using moriarty_testing::Read;
using testing::AllOf;
using testing::AnyOf;
using testing::Eq;
using testing::Optional;

Graph<> Graph1() {
  Graph G(3);
  G.AddEdge(0, 1);
  G.AddEdge(1, 2);
  G.AddEdge(2, 1);
  return G;
}
std::string_view Graph1String() {
  return R"(0 1
1 2
2 1
)";
}
Graph<> Graph2() {
  Graph G(4);
  G.AddEdge(2, 1);
  G.AddEdge(0, 1);
  G.AddEdge(1, 2);
  G.AddEdge(2, 3);
  G.AddEdge(3, 0);
  return G;
}
std::string_view Graph2String() {
  return R"(2 1
0 1
1 2
2 3
3 0
)";
}

TEST(MGraphTest, TypenameIsCorrect) {
  EXPECT_EQ(MGraph().Typename(), "MGraph");
}

TEST(MGraphTest, PrintShouldSucceed) {
  EXPECT_EQ(Print(MGraph(), Graph1()), Graph1String());
  EXPECT_EQ(Print(MGraph(), Graph2()), Graph2String());
  EXPECT_EQ(Print(MGraph(), Graph<>(0)), "");
}

TEST(MGraphTest, ReadShouldSucceed) {
  EXPECT_EQ(Read(MGraph(NumNodes(3), NumEdges(3)), Graph1String()), Graph1());
  EXPECT_EQ(Read(MGraph(NumNodes(4), NumEdges(5)), Graph2String()), Graph2());
  EXPECT_EQ(Read(MGraph(NumNodes(0), NumEdges(0)), ""), Graph<>(0));
}

TEST(MGraphTest, ReadShouldFailOnInvalidInput) {
  EXPECT_THROW(
      { (void)Read(MGraph(NumNodes(2), NumEdges(3)), "0 1\n1 2\n2 1\n"); },
      IOError);
  EXPECT_THROW(
      {
        (void)Read(MGraph(NumNodes(4), NumEdges(6)),
                   "2 1\n0 1\n1 2\n2 3\n3 0\n");
      },
      IOError);
}

MATCHER_P(NumNodesIs, matcher, "has the correct number of nodes") {
  return ::testing::ExplainMatchResult(matcher, arg.NumNodes(),
                                       result_listener);
}

MATCHER_P(NumEdgesIs, matcher, "has the correct number of edges") {
  return ::testing::ExplainMatchResult(matcher, arg.NumEdges(),
                                       result_listener);
}

TEST(MGraphTest, GenerateShouldSucceed) {
  EXPECT_THAT(MGraph(NumNodes(1), NumEdges(3)),
              GeneratedValuesAre(AllOf(NumNodesIs(Eq(1)), NumEdgesIs(Eq(3)))));
  EXPECT_THAT(MGraph(NumNodes(0), NumEdges(0)),
              GeneratedValuesAre(AllOf(NumNodesIs(Eq(0)), NumEdgesIs(Eq(0)))));
  EXPECT_THAT(MGraph(NumNodes(3), NumEdges(3)),
              GeneratedValuesAre(AllOf(NumNodesIs(Eq(3)), NumEdgesIs(Eq(3)))));
  EXPECT_THAT(
      MGraph(NumNodes(10), NumEdges(15)),
      GeneratedValuesAre(AllOf(NumNodesIs(Eq(10)), NumEdgesIs(Eq(15)))));
}

TEST(MGraphTest, GenerateShouldFailOnMissingConstraints) {
  EXPECT_THAT(MGraph(NumNodes(3)),
              GenerateThrowsGenerationError("", Context()));
  EXPECT_THAT(MGraph(NumNodes(0), NumEdges(3)),
              GenerateThrowsGenerationError("", Context()));
}

TEST(MGraphTest, GetUniqueValueShouldSucceed) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  librarian::AnalysisContext ctx("MGraph", variables, values);

  EXPECT_THAT(MGraph(NumNodes(0), NumEdges(0)).GetUniqueValue(ctx),
              Optional(Graph<>(0)));
  EXPECT_THAT(MGraph(NumNodes(10), NumEdges(0)).GetUniqueValue(ctx),
              Optional(Graph<>(10)));
  {
    Graph G(1);
    for (int i = 0; i < 3; ++i) G.AddEdge(0, 0);
    EXPECT_THAT(MGraph(NumNodes(1), NumEdges(3)).GetUniqueValue(ctx),
                Optional(G));
  }
  EXPECT_EQ(MGraph(NumNodes(3)).GetUniqueValue(ctx), std::nullopt);
}

TEST(MGraphTest, ExactlyAndOneOfConstraintsShouldWork) {
  EXPECT_THAT(MGraph(Exactly(Graph1())), GeneratedValuesAre(Eq(Graph1())));
  EXPECT_THAT(MGraph(OneOf({Graph1(), Graph2()})),
              GeneratedValuesAre(AnyOf(Eq(Graph1()), Eq(Graph2()))));
}

}  // namespace
}  // namespace moriarty
