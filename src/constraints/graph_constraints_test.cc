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

#include "src/constraints/graph_constraints.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/constraints/numeric_constraints.h"
#include "src/librarian/testing/gtest_helpers.h"
#include "src/types/graph.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace {

using ::moriarty_testing::Context;
using ::moriarty_testing::GeneratedValuesAre;
using ::moriarty_testing::HasConstraintViolation;
using ::moriarty_testing::HasNoConstraintViolation;
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::Ge;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Le;

// These aren't really tests, moreso to check for visual consistency.
TEST(GraphConstraintsTest, ToStringLooksReasonable) {
  EXPECT_EQ(NumNodes(Between(1, 10)).ToString(),
            "is a graph whose number of nodes is between 1 and 10");
  EXPECT_EQ(NumEdges(Between(1, 10)).ToString(),
            "is a graph whose number of edges is between 1 and 10");
  EXPECT_EQ(Connected().ToString(), "is a connected graph");
  EXPECT_EQ(NoParallelEdges().ToString(), "is a graph with no parallel edges");
  EXPECT_EQ(Loopless().ToString(), "is a graph with no loops");
  EXPECT_EQ(SimpleGraph().ToString(), "is a simple graph");
  EXPECT_EQ(NodeLabels<MInteger>(Between(1, 10)).ToString(),
            "each node label is between 1 and 10");
  EXPECT_EQ(EdgeLabels<MInteger>(Between(1, 10)).ToString(),
            "each edge label is between 1 and 10");
}

// These are only sort of tests... It doesn't matter exactly what the strings
// are, but I'm checking for them exactly here so we manually verify that the
// messages have consistency.
TEST(GraphConstraintsTest, UnsatisfiedReasonLooksReasonable) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  librarian::AnalysisContext ctx("N", variables, values);

  EXPECT_THAT(NumNodes(Between(1, 10)).CheckValue(ctx, Graph(25)),
              HasConstraintViolation(
                  "number of nodes (which is 25) is not between 1 and 10"));
  EXPECT_THAT(NumEdges(Between(1, 10)).CheckValue(ctx, Graph(25)),
              HasConstraintViolation(
                  "number of edges (which is 0) is not between 1 and 10"));
  EXPECT_THAT(Connected().CheckValue(Graph(25)),
              HasConstraintViolation(
                  "is not connected (no path from node 0 to node 1)"));
  EXPECT_THAT(
      NoParallelEdges().CheckValue(Graph(25).AddEdge(0, 1).AddEdge(0, 1)),
      HasConstraintViolation(
          "contains a parallel edge (between nodes 0 and 1)"));
  EXPECT_THAT(Loopless().CheckValue(Graph(25).AddEdge(1, 1)),
              HasConstraintViolation("contains a loop at node 1"));
  EXPECT_THAT(
      SimpleGraph().CheckValue(Graph(25).AddEdge(1, 1)),
      HasConstraintViolation("is not simple: contains a loop at node 1"));

  Graph<NoEdgeLabel, int64_t> graph_with_node_labels(3);
  graph_with_node_labels.SetNodeLabels({5, 10, 15});
  EXPECT_THAT(NodeLabels<MInteger>(Between(1, 10))
                  .CheckValue(ctx, graph_with_node_labels),
              HasConstraintViolation(
                  "node 2's label (which is `15`) is not between 1 and 10"));

  Graph<int64_t, NoNodeLabel> graph_with_edge_labels(3);
  graph_with_edge_labels.AddEdge(0, 1, 5).AddEdge(1, 2, 15);
  EXPECT_THAT(EdgeLabels<MInteger>(Between(1, 10))
                  .CheckValue(ctx, graph_with_edge_labels),
              HasConstraintViolation(
                  "edge 1's label (which is `15`) is not between 1 and 10"));
}

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

TEST(GraphConstraintsTest, NodeAndEdgeLabelsGetConstraintsAreCorrect) {
  EXPECT_THAT(NodeLabels<MInteger>(Exactly(10)).GetConstraints(),
              GeneratedValuesAre(10));
  EXPECT_THAT(EdgeLabels<MInteger>(Exactly(10)).GetConstraints(),
              GeneratedValuesAre(10));
}

TEST(GraphConstraintsTest, NumNodesAndNumEdgesDependenciesWork) {
  EXPECT_THAT(NumNodes(Between(1, 10)).GetDependencies(), IsEmpty());
  EXPECT_THAT(NumEdges(Between(1, 10)).GetDependencies(), IsEmpty());
  EXPECT_THAT(NumNodes(Between(1, "3 * N + 1")).GetDependencies(),
              ElementsAre("N"));
  EXPECT_THAT(NumEdges(Between(1, "3 * N + 1")).GetDependencies(),
              ElementsAre("N"));
}

TEST(GraphConstraintsTest, NodeAndEdgeLabelsDependenciesWork) {
  EXPECT_THAT(NodeLabels<MInteger>(Between(1, 10)).GetDependencies(),
              IsEmpty());
  EXPECT_THAT(EdgeLabels<MInteger>(Between(1, 10)).GetDependencies(),
              IsEmpty());
  EXPECT_THAT(NodeLabels<MInteger>(Between(1, "3 * N + 1")).GetDependencies(),
              ElementsAre("N"));
  EXPECT_THAT(EdgeLabels<MInteger>(Between(1, "3 * N + 1")).GetDependencies(),
              ElementsAre("N"));
}

Graph<NoEdgeLabel, NoNodeLabel> CreateGraph(int64_t num_nodes,
                                            int64_t num_edges) {
  Graph graph(num_nodes);
  for (int64_t i = 0; i < num_edges; ++i) {
    graph.AddEdge(i % num_nodes, (i + 1) % num_nodes);
  }
  return graph;
}

TEST(GraphConstraintsTest, NumNodesAndNumEdgesSatisfiedWithWorks) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  librarian::AnalysisContext ctx("N", variables, values);

  EXPECT_THAT(NumNodes(Between(1, 100)).CheckValue(ctx, CreateGraph(5, 3)),
              HasNoConstraintViolation());
  EXPECT_THAT(NumNodes(Between(1, 100)).CheckValue(ctx, CreateGraph(150, 3)),
              HasConstraintViolation(HasSubstr("number of nodes")));
  EXPECT_THAT(NumEdges(Between(1, 100)).CheckValue(ctx, CreateGraph(5, 30)),
              HasNoConstraintViolation());
  EXPECT_THAT(NumEdges(Between(1, 100)).CheckValue(ctx, CreateGraph(5, 105)),
              HasConstraintViolation(HasSubstr("number of edges")));
}

TEST(GraphConstraintsTest, NodeAndEdgeLabelsSatisfiedWithWorks) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  librarian::AnalysisContext ctx("N", variables, values);

  Graph<NoEdgeLabel, int64_t> graph_with_node_labels(3);
  graph_with_node_labels.SetNodeLabels({5, 8, 2});
  EXPECT_THAT(NodeLabels<MInteger>(Between(1, 10))
                  .CheckValue(ctx, graph_with_node_labels),
              HasNoConstraintViolation());
  graph_with_node_labels.SetNodeLabels({5, 18, 2});
  EXPECT_THAT(NodeLabels<MInteger>(Between(1, 10))
                  .CheckValue(ctx, graph_with_node_labels),
              HasConstraintViolation(HasSubstr("node 1's label")));

  Graph<int64_t, NoNodeLabel> graph_with_edge_labels(3);
  graph_with_edge_labels.AddEdge(0, 1, 5).AddEdge(1, 2, 8);
  EXPECT_THAT(EdgeLabels<MInteger>(Between(1, 10))
                  .CheckValue(ctx, graph_with_edge_labels),
              HasNoConstraintViolation());
  graph_with_edge_labels.AddEdge(0, 2, 18);
  EXPECT_THAT(EdgeLabels<MInteger>(Between(1, 10))
                  .CheckValue(ctx, graph_with_edge_labels),
              HasConstraintViolation(HasSubstr("edge 2's label")));
}

}  // namespace
}  // namespace moriarty
