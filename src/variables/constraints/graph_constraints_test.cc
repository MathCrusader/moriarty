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

#include "src/variables/constraints/graph_constraints.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/testing/gtest_helpers.h"
#include "src/variables/constraints/numeric_constraints.h"
#include "src/variables/graph.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace {

using ::moriarty_testing::Context;
using ::moriarty_testing::GeneratedValuesAre;
using ::testing::AllOf;
using ::testing::Ge;
using ::testing::IsEmpty;
using ::testing::Le;

TEST(GraphConstraintsTest, NumNodesAndEdgesGetConstraintsAreCorrect) {
  EXPECT_THAT(NumNodes(10).GetConstraints(), GeneratedValuesAre(10));
  EXPECT_THAT(NumNodes("2 * N").GetConstraints(),
              GeneratedValuesAre(14, Context().WithValue<MInteger>("N", 7)));
  EXPECT_THAT(NumNodes(AtLeast("X"), AtMost(15)).GetConstraints(),
              GeneratedValuesAre(AllOf(Ge(3), Le(15)),
                                 Context().WithValue<MInteger>("X", 3)));

  EXPECT_THAT(NumEdges(10).GetConstraints(), GeneratedValuesAre(10));
  EXPECT_THAT(NumEdges("2 * N").GetConstraints(),
              GeneratedValuesAre(14, Context().WithValue<MInteger>("N", 7)));
  EXPECT_THAT(NumEdges(AtLeast("X"), AtMost(15)).GetConstraints(),
              GeneratedValuesAre(AllOf(Ge(3), Le(15)),
                                 Context().WithValue<MInteger>("X", 3)));
}

TEST(GraphConstraintsTest, NumNodesAndNumEdgesToStringWorks) {
  EXPECT_EQ(NumNodes(Between(1, 10)).ToString(),
            "is a graph whose number of nodes is between 1 and 10");
  EXPECT_EQ(NumEdges(Between(1, 10)).ToString(),
            "is a graph whose number of edges is between 1 and 10");
}

TEST(GraphConstraintsTest, NumNodesAndNumEdgesUnsatisfiedReasonWorks) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  librarian::AnalysisContext ctx("N", variables, values);

  EXPECT_EQ(NumNodes(Between(1, 10)).UnsatisfiedReason(ctx, Graph<>(25)),
            "number of nodes (which is 25) is not between 1 and 10");
  EXPECT_EQ(NumEdges(Between(1, 10)).UnsatisfiedReason(ctx, Graph<>(25)),
            "number of edges (which is 0) is not between 1 and 10");
}

TEST(GraphConstraintsTest, NumNodesAndNumEdgesDependenciesWork) {
  EXPECT_THAT(NumNodes(Between(1, 10)).GetDependencies(), IsEmpty());
  EXPECT_THAT(NumEdges(Between(1, 10)).GetDependencies(), IsEmpty());
  EXPECT_THAT(NumNodes(Between(1, "3 * N + 1")).GetDependencies(),
              testing::ElementsAre("N"));
  EXPECT_THAT(NumEdges(Between(1, "3 * N + 1")).GetDependencies(),
              testing::ElementsAre("N"));
}

Graph<> CreateGraph(int64_t num_nodes, int64_t num_edges) {
  Graph<> graph(num_nodes);
  for (int64_t i = 0; i < num_edges; ++i) {
    graph.AddEdge(i % num_nodes, (i + 1) % num_nodes);
  }
  return graph;
}

TEST(GraphConstraintsTest, NumNodesAndNumEdgesSatisfiedWithWorks) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  librarian::AnalysisContext ctx("N", variables, values);

  EXPECT_TRUE(
      NumNodes(Between(1, 100)).IsSatisfiedWith(ctx, CreateGraph(5, 3)));
  EXPECT_FALSE(
      NumNodes(Between(1, 100)).IsSatisfiedWith(ctx, CreateGraph(150, 3)));
  EXPECT_TRUE(
      NumEdges(Between(1, 100)).IsSatisfiedWith(ctx, CreateGraph(5, 30)));
  EXPECT_FALSE(
      NumEdges(Between(1, 100)).IsSatisfiedWith(ctx, CreateGraph(5, 105)));
}

}  // namespace
}  // namespace moriarty
