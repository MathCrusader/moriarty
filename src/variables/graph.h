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

#ifndef MORIARTY_SRC_VARIABLES_GRAPH_H_
#define MORIARTY_SRC_VARIABLES_GRAPH_H_

#include <cstdint>
#include <format>
#include <stdexcept>
#include <vector>

namespace moriarty {

// Graph
//
// An undirected graph with nodes and edges. Each node and edge can have labels.
// By default, all labels are default constructed (so 0 for int types).
template <typename EdgeLabel = int64_t, typename NodeLabel = int64_t>
class Graph {
 public:
  using NodeIdx = int64_t;  // 0-based index for nodes

  struct Edge {
    NodeIdx u;
    NodeIdx v;
    EdgeLabel e;

    auto operator<=>(const Edge& other) const = default;
  };

  // Constructs a graph with `num_nodes` nodes.
  explicit Graph(NodeIdx num_nodes);

  // Returns the number of nodes in the graph.
  [[nodiscard]] NodeIdx NumNodes() const { return num_nodes_; }

  // Returns the number of edges in the graph.
  [[nodiscard]] int64_t NumEdges() const { return edges_.size(); }

  // Sets the labels for all nodes in the graph.
  void SetNodeLabels(const std::vector<NodeLabel>& node_labels);

  // Returns the labels for all nodes in the graph.
  std::vector<NodeLabel> GetNodeLabels() const;

  // Adds an undirected edge between nodes `u` and `v`.
  Graph& AddEdge(NodeIdx u, NodeIdx v, EdgeLabel edge_label = {});

  // Returns the adjacency list representation of the graph.
  // The order of (u, v) will be altered so that adj[i][j].u == i.
  std::vector<std::vector<Edge>> GetAdjacencyList() const;

  // Returns all edges in the graph. There is no guarantee on the order of
  // edges, nor on the order of (u, v) in each edge.
  std::vector<Edge> GetEdges() const;

  template <typename E, typename V>
  friend bool operator==(const Graph<E, V>& lhs, const Graph<E, V>& rhs);

  std::string DebugString() const;

 private:
  NodeIdx num_nodes_ = 0;
  std::vector<NodeLabel> node_labels_;
  std::vector<Edge> edges_;
};

// ----------------------------------------------------------------------------
//  Template implementation below

template <typename E, typename V>
Graph<E, V>::Graph(NodeIdx num_nodes) : num_nodes_(num_nodes) {
  if (num_nodes < 0) {
    throw std::invalid_argument("Graph(): num_nodes must be non-negative.");
  }
  node_labels_.resize(num_nodes_);
}

template <typename E, typename V>
auto Graph<E, V>::GetAdjacencyList() const
    -> std::vector<std::vector<Graph::Edge>> {
  std::vector<std::vector<Graph::Edge>> adjacency_list(num_nodes_);
  for (const auto& edge : edges_) {
    adjacency_list[edge.u].push_back(edge);
    adjacency_list[edge.v].push_back({edge.v, edge.u, edge.e});
  }
  return adjacency_list;
}

template <typename E, typename V>
auto Graph<E, V>::GetEdges() const -> std::vector<Graph::Edge> {
  return edges_;
}

template <typename E, typename V>
Graph<E, V>& Graph<E, V>::AddEdge(int64_t u, int64_t v, E edge_label) {
  if (u < 0 || v < 0 || u >= num_nodes_ || v >= num_nodes_) {
    throw std::out_of_range("AddEdge(): Node index out of range.");
  }
  edges_.push_back({u, v, edge_label});
  return *this;
}

template <typename E, typename V>
void Graph<E, V>::SetNodeLabels(const std::vector<V>& node_labels) {
  if (node_labels.size() != num_nodes_) {
    throw std::invalid_argument(
        "SetNodeLabels(): Number of elements in node_labels does not match "
        "number of nodes in the graph.");
  }
  node_labels_ = node_labels;
}

template <typename E, typename V>
std::vector<V> Graph<E, V>::GetNodeLabels() const {
  return node_labels_;
}

template <typename E, typename V>
bool operator==(const Graph<E, V>& lhs, const Graph<E, V>& rhs) {
  return lhs.num_nodes_ == rhs.num_nodes_ &&
         lhs.edges_.size() == rhs.edges_.size() && lhs.edges_ == rhs.edges_ &&
         lhs.node_labels_ == rhs.node_labels_;
}

template <typename E, typename V>
std::string Graph<E, V>::DebugString() const {
  return std::format("Graph(num_nodes={}, num_edges={})", num_nodes_,
                     NumEdges());
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_GRAPH_H_
