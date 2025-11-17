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

#include <cstdint>
#include <optional>
#include <string_view>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/constraints/base_constraints.h"
#include "src/constraints/graph_constraints.h"
#include "src/constraints/numeric_constraints.h"
#include "src/contexts/librarian_context.h"
#include "src/internal/variable_set.h"
#include "src/librarian/errors.h"
#include "src/librarian/testing/gtest_helpers.h"
#include "src/types/graph.h"
#include "src/variables/minteger.h"
#include "src/variables/mstring.h"

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
using testing::Truly;

Graph<NoEdgeLabel, NoNodeLabel> Graph1() {
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
Graph<NoEdgeLabel, NoNodeLabel> Graph2() {
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
  EXPECT_EQ(Print(MGraph(), Graph<NoEdgeLabel, NoNodeLabel>(0)), "");
}

TEST(MGraphTest, ReadShouldSucceed) {
  EXPECT_EQ(Read(MGraph(NumNodes(3), NumEdges(3)), Graph1String()), Graph1());
  EXPECT_EQ(Read(MGraph(NumNodes(4), NumEdges(5)), Graph2String()), Graph2());
  EXPECT_EQ(Read(MGraph(NumNodes(0), NumEdges(0)), ""), Graph(0));
}

TEST(MGraphTest, PartialReadShouldSucceed) {
  auto reader = MGraph(NumNodes(3), NumEdges(3)).CreateChunkedReader(2);

  Context context;
  std::istringstream input(std::string{Graph1String()});
  InputCursor cursor(input, WhitespaceStrictness::kPrecise);
  librarian::ReaderContext ctx("A", cursor, Context().Variables(),
                               Context().Values());

  for (int i = 0; i < 3; i++) {
    reader.ReadNext(ctx);
    ASSERT_EQ(input.get(), '\n');  // Consume the separator
  }
  EXPECT_EQ(std::move(reader).Finalize(), Graph1());
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
              Optional(Graph(0)));
  EXPECT_THAT(MGraph(NumNodes(10), NumEdges(0)).GetUniqueValue(ctx),
              Optional(Graph(10)));
  {
    Graph G(1);
    for (int i = 0; i < 3; ++i) G.AddEdge(0, 0);
    EXPECT_THAT(MGraph(NumNodes(1), NumEdges(3)).GetUniqueValue(ctx),
                Optional(G));
  }
  EXPECT_EQ(MGraph(NumNodes(3)).GetUniqueValue(ctx), std::nullopt);
  {  // With edge/node labels
    Graph<int64_t, std::string> expected(1);
    expected.SetNodeLabels(std::vector<std::string>{"node"});
    expected.AddEdge(0, 0, 42);
    expected.AddEdge(0, 0, 42);
    expected.AddEdge(0, 0, 42);
    EXPECT_THAT((MGraph<MInteger, MString>(NumNodes(1), NumEdges(3),
                                           NodeLabels<MString>(Exactly("node")),
                                           EdgeLabels<MInteger>(Exactly(42)))
                     .GetUniqueValue(ctx)),
                Optional(expected));
  }
  {  // With just node labels
    Graph<NoEdgeLabel, std::string> expected(2);
    expected.SetNodeLabels(std::vector<std::string>{"label", "label"});
    EXPECT_THAT(
        (MGraph<MNoEdgeLabel, MString>(NumNodes(2), NumEdges(0),
                                       NodeLabels<MString>(Exactly("label")))
             .GetUniqueValue(ctx)),
        Optional(expected));
  }
  {  // Unique graph, but not unique labels
    Graph<int64_t, int64_t> expected(2);
    expected.SetNodeLabels({1, 1});
    expected.AddEdge(0, 1, 5);
    expected.AddEdge(1, 0, 5);
    EXPECT_EQ((MGraph<MInteger, MInteger>(NumNodes(1), NumEdges(2),
                                          NodeLabels<MInteger>(Between(1, 10)),
                                          EdgeLabels<MInteger>(Exactly(5)))
                   .GetUniqueValue(ctx)),
              std::nullopt);
  }
}

TEST(MGraphTest, ExactlyAndOneOfConstraintsShouldWork) {
  EXPECT_THAT(MGraph(Exactly(Graph1())), GeneratedValuesAre(Eq(Graph1())));
  EXPECT_THAT(MGraph(OneOf({Graph1(), Graph2()})),
              GeneratedValuesAre(AnyOf(Eq(Graph1()), Eq(Graph2()))));
}

TEST(MGraphTest, NodeLabelsAndEdgeLabelsCheckValue) {
  moriarty_internal::VariableSet variables;
  moriarty_internal::ValueSet values;
  librarian::AnalysisContext ctx("MGraphTest", variables, values);

  Graph<int64_t, int64_t> G(2);
  G.SetNodeLabels({10, 20});
  G.AddEdge(0, 1, 100);

  // Node label constraints
  NodeLabels<MInteger> node_labels_ok(AtLeast(5));
  EXPECT_THAT(node_labels_ok.CheckValue(ctx, G),
              moriarty_testing::HasNoConstraintViolation());

  NodeLabels<MInteger> node_labels_fail(AtMost(15));
  EXPECT_THAT(node_labels_fail.CheckValue(ctx, G),
              moriarty_testing::HasConstraintViolation(
                  "node 1's label (which is `20`) is not at most 15"));

  // Edge label constraints
  EdgeLabels<MInteger> edge_labels_ok(AtLeast(50));
  EXPECT_THAT(edge_labels_ok.CheckValue(ctx, G),
              moriarty_testing::HasNoConstraintViolation());

  EdgeLabels<MInteger> edge_labels_fail(AtMost(90));
  EXPECT_THAT(edge_labels_fail.CheckValue(ctx, G),
              moriarty_testing::HasConstraintViolation(
                  "edge 0's label (which is `100`) is not at most 90"));
}

TEST(MGraphTest, NodeLabelsAndEdgeLabelsAreGenerated) {
  // Node labels
  EXPECT_THAT(
      (MGraph<MNoEdgeLabel, MString>(NumNodes(2), NumEdges(1),
                                     NodeLabels<MString>(Exactly("label")))),
      GeneratedValuesAre(Truly([](const Graph<NoEdgeLabel, std::string>& G) {
        return G.NumNodes() == 2 && G.NumEdges() == 1 &&
               G.GetNodeLabels() == std::vector<std::string>{"label", "label"};
      })));

  // Edge labels
  EXPECT_THAT(
      (MGraph<MInteger, MNoNodeLabel>(NumNodes(2), NumEdges(1),
                                      EdgeLabels<MInteger>(Exactly(42)))),
      GeneratedValuesAre(Truly([](const Graph<int64_t, NoNodeLabel>& G) {
        return G.NumNodes() == 2 && G.NumEdges() == 1 &&
               G.GetEdges()[0].e == 42;
      })));

  // Both Node and Edge labels (pretty simplistic checks)
  EXPECT_THAT(
      (MGraph<MInteger, MString>(
          NumNodes(2), NumEdges(1),
          NodeLabels<MString>(Length(2), Alphabet("abc")),
          EdgeLabels<MInteger>(Between(30, 40)))),
      GeneratedValuesAre(Truly([](const Graph<int64_t, std::string>& G) {
        if (G.NumNodes() != 2 || G.NumEdges() != 1) return false;
        auto nodes = G.GetNodeLabels();
        if (nodes[0].size() != 2 || nodes[1].size() != 2) return false;
        auto edges = G.GetEdges();
        return edges[0].e >= 30 && edges[0].e <= 40;
      })));
}

}  // namespace
}  // namespace moriarty
