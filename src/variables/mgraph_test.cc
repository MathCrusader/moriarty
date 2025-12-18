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
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
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
using moriarty_testing::Read;
using moriarty_testing::Write;
using testing::AllOf;
using testing::AnyOf;
using testing::Eq;
using testing::Optional;
using testing::Truly;
using testing::UnorderedElementsAre;

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

TEST(MGraphTest, WriteShouldSucceed) {
  EXPECT_EQ(Write(MGraph(), Graph1()), Graph1String());
  EXPECT_EQ(Write(MGraph(), Graph2()), Graph2String());
  EXPECT_EQ(Write(MGraph(), Graph<NoEdgeLabel, NoNodeLabel>(0)), "");
}

TEST(MGraphTest, ReadShouldSucceed) {
  EXPECT_EQ(Read(MGraph(NumNodes(3), NumEdges(3)), Graph1String()), Graph1());
  EXPECT_EQ(Read(MGraph(NumNodes(4), NumEdges(5)), Graph2String()), Graph2());
  EXPECT_EQ(Read(MGraph(NumNodes(0), NumEdges(0)), ""), Graph(0));
}

TEST(MGraphTest, PartialReadShouldSucceed) {
  MGraph graph = MGraph(NumNodes(3), NumEdges(3));

  Context context;
  std::istringstream input(std::string{Graph1String()});
  InputCursor cursor(input, WhitespaceStrictness::kPrecise);
  librarian::ReadVariableContext ctx("A", cursor, context.Variables(),
                                     context.Values());
  auto reader = MGraph<MNoEdgeLabel, MNoNodeLabel>::Reader(ctx, 3, graph);

  for (int i = 0; i < 3; i++) {
    reader.ReadNext(ctx);
    ASSERT_EQ(input.get(), '\n');  // Consume the separator
  }
  EXPECT_EQ(std::move(reader).Finalize(), Graph1());
}

template <typename MEdgeLabel, typename MNodeLabel>
MGraph<MEdgeLabel, MNodeLabel>::value_type ReadMGraphFromFile(
    const MGraph<MEdgeLabel, MNodeLabel>& graph_variable,
    const std::filesystem::path& path, Context& context) {
  SCOPED_TRACE("Reading " + path.string());

  std::ifstream input(path);
  if (!input.is_open()) {
    throw std::runtime_error("Could not open " + path.string());
  }
  InputCursor cursor(input, WhitespaceStrictness::kPrecise);
  librarian::ReadVariableContext ctx("A", cursor, context.Variables(),
                                     context.Values());
  return graph_variable.Read(ctx);
}

template <typename MEdgeLabel, typename MNodeLabel>
void ReadAndVerifyMGraphFromFile(
    const MGraph<MEdgeLabel, MNodeLabel>& graph_variable,
    const std::filesystem::path& path, Context& context) {
  SCOPED_TRACE("Verifying " + path.string());

  auto value = ReadMGraphFromFile(graph_variable, path, context);

  std::stringstream write_ss;
  librarian::WriteVariableContext write_ctx("A", write_ss, context.Variables(),
                                            context.Values());
  graph_variable.Write(write_ctx, value);

  // Re-open the file and compare its full contents to what we written.
  std::ifstream input2(path, std::ios::binary);
  if (!input2.is_open()) {
    FAIL() << "Could not reopen " << path.string();
  }
  std::string file_contents((std::istreambuf_iterator<char>(input2)),
                            std::istreambuf_iterator<char>());
  ASSERT_EQ(file_contents, write_ss.str());
}

TEST(MGraphTest, ReadUnweightedAdjMatrixShouldSucceed) {
  MGraph graph(NumNodes(5), MGraphFormat().AdjacencyMatrix());

  Context context;
  ReadAndVerifyMGraphFromFile(
      graph, "src/variables/testing/mgraph/matrix_sym_binary_0diag.in",
      context);
  ReadAndVerifyMGraphFromFile(
      graph, "src/variables/testing/mgraph/matrix_sym_binary_1diag.in",
      context);
  ReadAndVerifyMGraphFromFile(
      graph, "src/variables/testing/mgraph/matrix_sym_ints_0diag.in", context);
  ReadAndVerifyMGraphFromFile(
      graph, "src/variables/testing/mgraph/matrix_sym_ints_1diag.in", context);
}

TEST(MGraphTest, ReadWeightedAdjMatrixShouldSucceed) {
  MGraph<MInteger> graph(NumNodes(5), MGraphFormat().AdjacencyMatrix());

  Context context;
  ReadAndVerifyMGraphFromFile(
      graph, "src/variables/testing/mgraph/matrix_sym_binary_0diag.in",
      context);
  ReadAndVerifyMGraphFromFile(
      graph, "src/variables/testing/mgraph/matrix_sym_binary_1diag.in",
      context);
  ReadAndVerifyMGraphFromFile(
      graph, "src/variables/testing/mgraph/matrix_sym_ints_0diag.in", context);
  ReadAndVerifyMGraphFromFile(
      graph, "src/variables/testing/mgraph/matrix_sym_ints_1diag.in", context);
}

TEST(MGraphTest, ReadUnweightedEdgeListShouldSucceed) {
  Context context;
  {
    SCOPED_TRACE("Distinct ints, distinct lines");
    MGraph graph(NumNodes(5), NumEdges(6),
                 MGraphFormat().EdgeList().OneBased());
    ReadAndVerifyMGraphFromFile(graph,
                                "src/variables/testing/mgraph/"
                                "list_pairs_1to5_distinctints_distinctlines.in",
                                context);
    auto G = ReadMGraphFromFile(graph,
                                "src/variables/testing/mgraph/"
                                "list_pairs_1to5_distinctints_distinctlines.in",
                                context);
    EXPECT_EQ(G.NumNodes(), 5);
    EXPECT_EQ(G.NumEdges(), 6);
    EXPECT_THAT(G.GetEdges(),
                UnorderedElementsAre(Graph<>::Edge{0, 1, NoType{}},  //
                                     Graph<>::Edge{1, 3, NoType{}},  //
                                     Graph<>::Edge{2, 0, NoType{}},  //
                                     Graph<>::Edge{3, 4, NoType{}},  //
                                     Graph<>::Edge{4, 0, NoType{}},  //
                                     Graph<>::Edge{4, 1, NoType{}}));
  }
  {
    SCOPED_TRACE("Duplicate ints, distinct lines");
    MGraph graph(NumNodes(5), NumEdges(6),
                 MGraphFormat().EdgeList().OneBased());
    ReadAndVerifyMGraphFromFile(graph,
                                "src/variables/testing/mgraph/"
                                "list_pairs_1to5_dupints_distinctlines.in",
                                context);
    auto G = ReadMGraphFromFile(graph,
                                "src/variables/testing/mgraph/"
                                "list_pairs_1to5_dupints_distinctlines.in",
                                context);
    EXPECT_EQ(G.NumNodes(), 5);
    EXPECT_EQ(G.NumEdges(), 6);
    EXPECT_THAT(G.GetEdges(),
                UnorderedElementsAre(Graph<>::Edge{0, 1, NoType{}},  //
                                     Graph<>::Edge{1, 3, NoType{}},  //
                                     Graph<>::Edge{2, 2, NoType{}},  //
                                     Graph<>::Edge{3, 4, NoType{}},  //
                                     Graph<>::Edge{4, 4, NoType{}},  //
                                     Graph<>::Edge{4, 1, NoType{}}));
  }
  {
    SCOPED_TRACE("Distinct ints, duplicate lines");
    MGraph graph(NumNodes(5), NumEdges(8),
                 MGraphFormat().EdgeList().OneBased());
    ReadAndVerifyMGraphFromFile(graph,
                                "src/variables/testing/mgraph/"
                                "list_pairs_1to5_distinctints_duplines.in",
                                context);
    auto G = ReadMGraphFromFile(graph,
                                "src/variables/testing/mgraph/"
                                "list_pairs_1to5_distinctints_duplines.in",
                                context);
    EXPECT_EQ(G.NumNodes(), 5);
    EXPECT_EQ(G.NumEdges(), 8);
    EXPECT_THAT(G.GetEdges(),
                UnorderedElementsAre(Graph<>::Edge{0, 1, NoType{}},  //
                                     Graph<>::Edge{1, 3, NoType{}},  //
                                     Graph<>::Edge{2, 0, NoType{}},  //
                                     Graph<>::Edge{3, 4, NoType{}},  //
                                     Graph<>::Edge{4, 0, NoType{}},  //
                                     Graph<>::Edge{3, 4, NoType{}},  //
                                     Graph<>::Edge{2, 0, NoType{}},  //
                                     Graph<>::Edge{0, 2, NoType{}}));
  }
}

TEST(MGraphTest, ReadWeightedEdgeListShouldSucceed) {
  Context context;
  {
    SCOPED_TRACE("Distinct ints, distinct lines");
    MGraph<MInteger> graph(NumNodes(5), NumEdges(6),
                           MGraphFormat().EdgeList().OneBased());
    ReadAndVerifyMGraphFromFile(graph,
                                "src/variables/testing/mgraph/"
                                "list_trips_1to5_distinctints_distinctlines.in",
                                context);
    auto G = ReadMGraphFromFile(graph,
                                "src/variables/testing/mgraph/"
                                "list_trips_1to5_distinctints_distinctlines.in",
                                context);
    EXPECT_EQ(G.NumNodes(), 5);
    EXPECT_EQ(G.NumEdges(), 6);
    EXPECT_THAT(G.GetEdges(),
                UnorderedElementsAre(Graph<int64_t>::Edge{0, 1, 1},   //
                                     Graph<int64_t>::Edge{1, 3, 3},   //
                                     Graph<int64_t>::Edge{2, 0, -1},  //
                                     Graph<int64_t>::Edge{3, 4, 0},   //
                                     Graph<int64_t>::Edge{4, 0, 4},   //
                                     Graph<int64_t>::Edge{4, 1, 3}));
  }
  {
    SCOPED_TRACE("Duplicate ints, distinct lines");
    MGraph<MInteger> graph(NumNodes(5), NumEdges(6),
                           MGraphFormat().EdgeList().OneBased());
    ReadAndVerifyMGraphFromFile(graph,
                                "src/variables/testing/mgraph/"
                                "list_trips_1to5_dupints_distinctlines.in",
                                context);
    auto G = ReadMGraphFromFile(graph,
                                "src/variables/testing/mgraph/"
                                "list_trips_1to5_dupints_distinctlines.in",
                                context);
    EXPECT_EQ(G.NumNodes(), 5);
    EXPECT_EQ(G.NumEdges(), 6);
    EXPECT_THAT(G.GetEdges(),
                UnorderedElementsAre(Graph<int64_t>::Edge{0, 1, -1},  //
                                     Graph<int64_t>::Edge{1, 3, -4},  //
                                     Graph<int64_t>::Edge{2, 2, -5},  //
                                     Graph<int64_t>::Edge{3, 4, 1},   //
                                     Graph<int64_t>::Edge{4, 4, 0},   //
                                     Graph<int64_t>::Edge{4, 1, 10}));
  }
  {
    SCOPED_TRACE("Distinct ints, duplicate lines");
    MGraph<MInteger> graph(NumNodes(5), NumEdges(8),
                           MGraphFormat().EdgeList().OneBased());
    ReadAndVerifyMGraphFromFile(graph,
                                "src/variables/testing/mgraph/"
                                "list_trips_1to5_distinctints_duplines.in",
                                context);
    auto G = ReadMGraphFromFile(graph,
                                "src/variables/testing/mgraph/"
                                "list_trips_1to5_distinctints_duplines.in",
                                context);
    EXPECT_EQ(G.NumNodes(), 5);
    EXPECT_EQ(G.NumEdges(), 8);
    EXPECT_THAT(G.GetEdges(),
                UnorderedElementsAre(Graph<int64_t>::Edge{0, 1, 0},  //
                                     Graph<int64_t>::Edge{1, 3, 1},  //
                                     Graph<int64_t>::Edge{2, 0, 1},  //
                                     Graph<int64_t>::Edge{3, 4, 4},  //
                                     Graph<int64_t>::Edge{4, 0, 3},  //
                                     Graph<int64_t>::Edge{3, 4, 2},  //
                                     Graph<int64_t>::Edge{2, 0, 1},  //
                                     Graph<int64_t>::Edge{0, 2, 1}));
  }
}

TEST(MGraphTest, ReadEdgeListWithNodeLabelsShouldSucceed) {
  // The node labels will just be a string. So we can still use the numbers.
  Context context;
  {
    SCOPED_TRACE("Distinct ints, distinct lines");
    MGraph<MNoEdgeLabel, MString> graph(NumNodes(5), NumEdges(6),
                                        MGraphFormat().EdgeList().OneBased());
    ReadAndVerifyMGraphFromFile(graph,
                                "src/variables/testing/mgraph/"
                                "list_pairs_1to5_distinctints_distinctlines.in",
                                context);
    auto G = ReadMGraphFromFile(graph,
                                "src/variables/testing/mgraph/"
                                "list_pairs_1to5_distinctints_distinctlines.in",
                                context);
    EXPECT_EQ(G.NumNodes(), 5);
    EXPECT_EQ(G.NumEdges(), 6);
    EXPECT_THAT(G.GetEdges(),
                UnorderedElementsAre(
                    Graph<NoEdgeLabel, std::string>::Edge{0, 1, NoType{}},  //
                    Graph<NoEdgeLabel, std::string>::Edge{1, 3, NoType{}},  //
                    Graph<NoEdgeLabel, std::string>::Edge{2, 0, NoType{}},  //
                    Graph<NoEdgeLabel, std::string>::Edge{3, 4, NoType{}},  //
                    Graph<NoEdgeLabel, std::string>::Edge{4, 0, NoType{}},  //
                    Graph<NoEdgeLabel, std::string>::Edge{4, 1, NoType{}}));
  }
  {
    SCOPED_TRACE("Duplicate ints, distinct lines");
    MGraph<MNoEdgeLabel, MString> graph(NumNodes(5), NumEdges(6),
                                        MGraphFormat().EdgeList().OneBased());
    ReadAndVerifyMGraphFromFile(graph,
                                "src/variables/testing/mgraph/"
                                "list_pairs_1to5_dupints_distinctlines.in",
                                context);
    auto G = ReadMGraphFromFile(graph,
                                "src/variables/testing/mgraph/"
                                "list_pairs_1to5_dupints_distinctlines.in",
                                context);
    EXPECT_EQ(G.NumNodes(), 5);
    EXPECT_EQ(G.NumEdges(), 6);
    EXPECT_THAT(G.GetEdges(),
                UnorderedElementsAre(
                    Graph<NoEdgeLabel, std::string>::Edge{0, 1, NoType{}},  //
                    Graph<NoEdgeLabel, std::string>::Edge{1, 3, NoType{}},  //
                    Graph<NoEdgeLabel, std::string>::Edge{2, 2, NoType{}},  //
                    Graph<NoEdgeLabel, std::string>::Edge{3, 4, NoType{}},  //
                    Graph<NoEdgeLabel, std::string>::Edge{4, 4, NoType{}},  //
                    Graph<NoEdgeLabel, std::string>::Edge{4, 1, NoType{}}));
  }
  {
    SCOPED_TRACE("Distinct ints, duplicate lines");
    MGraph<MNoEdgeLabel, MString> graph(NumNodes(5), NumEdges(8),
                                        MGraphFormat().EdgeList().OneBased());
    ReadAndVerifyMGraphFromFile(graph,
                                "src/variables/testing/mgraph/"
                                "list_pairs_1to5_distinctints_duplines.in",
                                context);
    auto G = ReadMGraphFromFile(graph,
                                "src/variables/testing/mgraph/"
                                "list_pairs_1to5_distinctints_duplines.in",
                                context);
    EXPECT_EQ(G.NumNodes(), 5);
    EXPECT_EQ(G.NumEdges(), 8);
    EXPECT_THAT(G.GetEdges(),
                UnorderedElementsAre(
                    Graph<NoEdgeLabel, std::string>::Edge{0, 1, NoType{}},  //
                    Graph<NoEdgeLabel, std::string>::Edge{1, 3, NoType{}},  //
                    Graph<NoEdgeLabel, std::string>::Edge{2, 0, NoType{}},  //
                    Graph<NoEdgeLabel, std::string>::Edge{3, 4, NoType{}},  //
                    Graph<NoEdgeLabel, std::string>::Edge{4, 0, NoType{}},  //
                    Graph<NoEdgeLabel, std::string>::Edge{3, 4, NoType{}},  //
                    Graph<NoEdgeLabel, std::string>::Edge{2, 0, NoType{}},  //
                    Graph<NoEdgeLabel, std::string>::Edge{0, 2, NoType{}}));
  }
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
  librarian::AnalyzeVariableContext ctx("MGraph", variables, values);

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
  librarian::AnalyzeVariableContext ctx("MGraphTest", variables, values);

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
