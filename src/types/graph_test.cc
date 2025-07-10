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

#include "src/types/graph.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace moriarty {
namespace {

using testing::AllOf;
using testing::Each;
using testing::ElementsAre;
using testing::IsEmpty;
using testing::SizeIs;

TEST(GraphTest, ConstructorGivesEmptyGraph) {
  Graph graph(10);
  EXPECT_THAT(graph.GetNodeLabels(), AllOf(SizeIs(10), Each(0)));
  EXPECT_THAT(graph.GetAdjacencyList(), AllOf(SizeIs(10), Each(IsEmpty())));
  EXPECT_THAT(graph.GetEdges(), IsEmpty());
}

TEST(GraphTest, LabelsAreDefaultConstructed) {
  Graph graph(10);
  EXPECT_THAT(graph.GetNodeLabels(), AllOf(SizeIs(10), Each(0)));
  graph.AddEdge(2, 3);
  EXPECT_THAT(graph.GetEdges(), ElementsAre(Graph<>::Edge{2, 3, 0}));
}

TEST(GraphTest, NodeLabelsCanBeSetAndGot) {
  Graph graph(10);
  graph.SetNodeLabels({11, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  EXPECT_THAT(graph.GetNodeLabels(),
              ElementsAre(11, 2, 3, 4, 5, 6, 7, 8, 9, 10));
}

TEST(GraphTest, AddEdgeAddsEdge) {
  Graph graph(4);
  graph.AddEdge(2, 3, 5);
  EXPECT_THAT(graph.GetAdjacencyList(),
              ElementsAre(IsEmpty(),                            //
                          IsEmpty(),                            //
                          ElementsAre(Graph<>::Edge{2, 3, 5}),  //
                          ElementsAre(Graph<>::Edge{3, 2, 5})));
  EXPECT_THAT(graph.GetEdges(), ElementsAre(Graph<>::Edge{2, 3, 5}));
}

TEST(GraphTest, AddingAnInvalidEdgeThrows) {
  Graph graph(4);
  EXPECT_THROW(graph.AddEdge(-1, 2), std::out_of_range);
  EXPECT_THROW(graph.AddEdge(2, -1), std::out_of_range);
  EXPECT_THROW(graph.AddEdge(2, 4), std::out_of_range);
  EXPECT_THROW(graph.AddEdge(4, 2), std::out_of_range);
}

}  // namespace
}  // namespace moriarty
