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

#ifndef MORIARTY_SRC_VARIABLES_CONSTRAINTS_GRAPH_CONSTRAINTS_H_
#define MORIARTY_SRC_VARIABLES_CONSTRAINTS_GRAPH_CONSTRAINTS_H_

#include <set>
#include <utility>

#include "src/constraints/base_constraints.h"
#include "src/constraints/constraint_violation.h"
#include "src/contexts/librarian_context.h"
#include "src/types/graph.h"
#include "src/variables/minteger.h"

namespace moriarty {

// Constrains the number of nodes (vertices) in a graph.
class NumNodes : public MConstraint {
 public:
  // The graph must have exactly this many nodes.
  explicit NumNodes(int64_t num_nodes);

  // The number of nodes in the graph must be exactly this integer expression.
  // E.g., NumNodes("3 * N + 1").
  explicit NumNodes(std::string_view expression);

  // The number of nodes in the graph must satisfy all of these constraints.
  // E.g., NumNodes(Between(1, 10), Prime())
  template <typename... Constraints>
    requires(std::constructible_from<MInteger, Constraints...> &&
             sizeof...(Constraints) > 0)
  explicit NumNodes(Constraints&&... constraints);

  // Returns the constraints on the number of nodes.
  [[nodiscard]] MInteger GetConstraints() const;

  // Determines if the graph has the correct number of nodes.
  template <typename EdgeLabel, typename NodeLabel>
  [[nodiscard]] ConstraintViolation CheckValue(
      librarian::AnalysisContext ctx,
      const Graph<EdgeLabel, NodeLabel>& value) const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const;

 private:
  MInteger num_nodes_;
};

// Constrains the number of edges in a graph.
class NumEdges : public MConstraint {
 public:
  // The graph must have exactly this many edges.
  explicit NumEdges(int64_t num_edges);

  // The number of edges in the graph must be exactly this integer expression.
  // E.g., NumEdges("3 * N + 1").
  explicit NumEdges(std::string_view expression);

  // The number of edges in the graph must satisfy all of these constraints.
  // E.g., NumEdges(Between(1, 10), Prime())
  template <typename... Constraints>
    requires(std::constructible_from<MInteger, Constraints...> &&
             sizeof...(Constraints) > 0)
  explicit NumEdges(Constraints&&... constraints);

  // Returns the constraints on the number of edges.
  [[nodiscard]] MInteger GetConstraints() const;

  // Determines if the graph has the correct number of edges.
  template <typename EdgeLabel, typename NodeLabel>
  [[nodiscard]] ConstraintViolation CheckValue(
      librarian::AnalysisContext ctx,
      const Graph<EdgeLabel, NodeLabel>& value) const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const;

 private:
  MInteger num_edges_;
};

// The graph must have exactly one connected component.
//
// In particular, the graph with 0 nodes is *not* connected.
class Connected : public BasicMConstraint {
 public:
  explicit Connected() : BasicMConstraint("is a connected graph") {}

  // Determines if the graph is connected.
  template <typename EdgeLabel, typename NodeLabel>
  [[nodiscard]] ConstraintViolation CheckValue(
      const Graph<EdgeLabel, NodeLabel>& value) const;
};

// The graph must contain no parallel edges. That is, the edge (u, v) is only
// present in the graph at most once.
//
// Note: This limits the graph to ((n+1) choose 2) nodes if the graph contains
// loops and (n choose 2) if the graph is loopless.
class NoParallelEdges : public BasicMConstraint {
 public:
  explicit NoParallelEdges()
      : BasicMConstraint("is a graph with no parallel edges") {}

  // Determines if the graph has any parallel edges.
  template <typename EdgeLabel, typename NodeLabel>
  [[nodiscard]] ConstraintViolation CheckValue(
      const Graph<EdgeLabel, NodeLabel>& value) const;
};

// The graph must not contain any loops. That is, the edge (u, u) is not
// present in the graph.
class Loopless : public BasicMConstraint {
 public:
  explicit Loopless() : BasicMConstraint("is a graph with no loops") {}

  // Determines if the graph has any loops.
  template <typename EdgeLabel, typename NodeLabel>
  [[nodiscard]] ConstraintViolation CheckValue(
      const Graph<EdgeLabel, NodeLabel>& value) const;
};

// The graph is simple if it is loopless and contains no parallel edges.
class SimpleGraph : public BasicMConstraint {
 public:
  explicit SimpleGraph() : BasicMConstraint("is a simple graph") {}

  // Determines if the graph is simple.
  template <typename EdgeLabel, typename NodeLabel>
  [[nodiscard]] ConstraintViolation CheckValue(
      const Graph<EdgeLabel, NodeLabel>& value) const;
};

// Constraints that all node labels of a graph must satisfy.
template <typename MLabelType>
class NodeLabels : public MConstraint {
 public:
  // The node labels of the graph must satisfy all of these constraints.
  // E.g., NodeLabels<MInteger>(Between(1, 10), Prime())
  template <typename... Constraints>
    requires(std::constructible_from<MLabelType, Constraints...> &&
             sizeof...(Constraints) > 0)
  explicit NodeLabels(Constraints&&... label_constraints);

  // Returns the constraints on the node labels.
  [[nodiscard]] MLabelType GetConstraints() const;

  // Determines if the graph's node labels satisfy all constraints.
  template <typename EdgeLabel>
  [[nodiscard]] ConstraintViolation CheckValue(
      librarian::AnalysisContext ctx,
      const Graph<EdgeLabel, typename MLabelType::value_type>& value) const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const;

 private:
  MLabelType label_constraints_;
};

// Constraints that all edge labels of a graph must satisfy.
template <typename MLabelType>
class EdgeLabels : public MConstraint {
 public:
  // The edge labels of the graph must satisfy all of these constraints.
  // E.g., EdgeLabels<MInteger>(Between(1, 10), Prime())
  template <typename... Constraints>
    requires(std::constructible_from<MLabelType, Constraints...> &&
             sizeof...(Constraints) > 0)
  explicit EdgeLabels(Constraints&&... label_constraints);

  // Returns the constraints on the edge labels.
  [[nodiscard]] MLabelType GetConstraints() const;

  // Determines if the graph's edge labels satisfy all constraints.
  template <typename NodeLabel>
  [[nodiscard]] ConstraintViolation CheckValue(
      librarian::AnalysisContext ctx,
      const Graph<typename MLabelType::value_type, NodeLabel>& value) const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const;

 private:
  MLabelType label_constraints_;
};

// ----------------------------------------------------------------------------
//  Template Implementation Below

template <typename... Constraints>
  requires(std::constructible_from<MInteger, Constraints...> &&
           sizeof...(Constraints) > 0)
NumEdges::NumEdges(Constraints&&... constraints)
    : num_edges_(std::forward<Constraints>(constraints)...) {}

template <typename... Constraints>
  requires(std::constructible_from<MInteger, Constraints...> &&
           sizeof...(Constraints) > 0)
NumNodes::NumNodes(Constraints&&... constraints)
    : num_nodes_(std::forward<Constraints>(constraints)...) {}

// ====== NodeLabels ======
template <typename MLabelType>
template <typename... Constraints>
  requires(std::constructible_from<MLabelType, Constraints...> &&
           sizeof...(Constraints) > 0)
NodeLabels<MLabelType>::NodeLabels(Constraints&&... label_constraints)
    : label_constraints_(std::forward<Constraints>(label_constraints)...) {}

template <typename MLabelType>
MLabelType NodeLabels<MLabelType>::GetConstraints() const {
  return label_constraints_;
}

template <typename MLabelType>
template <typename EdgeLabel>
ConstraintViolation NodeLabels<MLabelType>::CheckValue(
    librarian::AnalysisContext ctx,
    const Graph<EdgeLabel, typename MLabelType::value_type>& value) const {
  int idx = -1;
  for (const auto& label : value.GetNodeLabels()) {
    idx++;
    if (auto check = label_constraints_.CheckValue(ctx, label)) {
      return ConstraintViolation(std::format("node {}'s label (which is {}) {}",
                                             idx, librarian::DebugString(label),
                                             check.Reason()));
    }
  }
  return ConstraintViolation::None();
}

template <typename MLabelType>
std::string NodeLabels<MLabelType>::ToString() const {
  return std::format("each node label {}", label_constraints_.ToString());
}

template <typename MLabelType>
std::vector<std::string> NodeLabels<MLabelType>::GetDependencies() const {
  return label_constraints_.GetDependencies();
}

// ====== EdgeLabels ======
template <typename MLabelType>
template <typename... Constraints>
  requires(std::constructible_from<MLabelType, Constraints...> &&
           sizeof...(Constraints) > 0)
EdgeLabels<MLabelType>::EdgeLabels(Constraints&&... label_constraints)
    : label_constraints_(std::forward<Constraints>(label_constraints)...) {}

template <typename MLabelType>
MLabelType EdgeLabels<MLabelType>::GetConstraints() const {
  return label_constraints_;
}

template <typename MLabelType>
template <typename NodeLabel>
ConstraintViolation EdgeLabels<MLabelType>::CheckValue(
    librarian::AnalysisContext ctx,
    const Graph<typename MLabelType::value_type, NodeLabel>& value) const {
  int idx = -1;
  for (const auto& edge : value.GetEdges()) {
    idx++;
    if (auto check = label_constraints_.CheckValue(ctx, edge.e)) {
      return ConstraintViolation(
          std::format("edge {}'s label (which is {}) {}", idx,
                      librarian::DebugString(edge.e), check.Reason()));
    }
  }
  return ConstraintViolation::None();
}

template <typename MLabelType>
std::string EdgeLabels<MLabelType>::ToString() const {
  return std::format("each edge label {}", label_constraints_.ToString());
}

template <typename MLabelType>
std::vector<std::string> EdgeLabels<MLabelType>::GetDependencies() const {
  return label_constraints_.GetDependencies();
}

// ====== NumNodes ======
template <typename EdgeLabel, typename NodeLabel>
ConstraintViolation NumNodes::CheckValue(
    librarian::AnalysisContext ctx,
    const Graph<EdgeLabel, NodeLabel>& value) const {
  auto check =
      num_nodes_.CheckValue(ctx, static_cast<int64_t>(value.NumNodes()));
  if (check.IsOk()) return ConstraintViolation::None();
  return ConstraintViolation(std::format("number of nodes (which is {}) {}",
                                         value.NumNodes(), check.Reason()));
}

// ====== NumEdges ======
template <typename EdgeLabel, typename NodeLabel>
ConstraintViolation NumEdges::CheckValue(
    librarian::AnalysisContext ctx,
    const Graph<EdgeLabel, NodeLabel>& value) const {
  auto check =
      num_edges_.CheckValue(ctx, static_cast<int64_t>(value.NumEdges()));
  if (check.IsOk()) return ConstraintViolation::None();
  return ConstraintViolation(std::format("number of edges (which is {}) {}",
                                         value.NumEdges(), check.Reason()));
}

// ====== Connected ======
template <typename EdgeLabel, typename NodeLabel>
ConstraintViolation Connected::CheckValue(
    const Graph<EdgeLabel, NodeLabel>& value) const {
  if (value.NumNodes() == 0)
    return ConstraintViolation(
        "is not connected (a graph with 0 nodes is not considered connected)");

  class UnionFind {
   public:
    UnionFind(int size) : parent_(size) {
      for (int i = 0; i < size; ++i) parent_[i] = i;
    }

    int find(int i) {
      return parent_[i] = (parent_[i] == i ? i : find(parent_[i]));
    }

    void unite(int i, int j) {
      int I = find(i);
      int J = find(j);
      if (I != J) parent_[I] = J;
    }

   private:
    std::vector<int> parent_;
  };

  UnionFind uf(value.NumNodes());
  auto edges = value.GetEdges();
  for (const auto& [u, v, _] : edges) uf.unite(u, v);

  for (int i = 1; i < value.NumNodes(); ++i)
    if (uf.find(i) != uf.find(0))
      return ConstraintViolation(
          std::format("is not connected (no path from node 0 to node {})", i));
  return ConstraintViolation::None();
}

// ====== NoParallelEdges ======
template <typename EdgeLabel, typename NodeLabel>
ConstraintViolation NoParallelEdges::CheckValue(
    const Graph<EdgeLabel, NodeLabel>& value) const {
  std::set<std::pair<typename Graph<EdgeLabel, NodeLabel>::NodeIdx,
                     typename Graph<EdgeLabel, NodeLabel>::NodeIdx>>
      seen;
  auto edges = value.GetEdges();
  for (const auto& [u, v, _] : edges) {
    if (!seen.emplace(u, v).second)
      return ConstraintViolation(std::format(
          "contains a parallel edge (between nodes {} and {})", u, v));
    if (u != v && !seen.emplace(v, u).second)
      return ConstraintViolation(std::format(
          "contains a parallel edge (between nodes {} and {})", v, u));
  }
  return ConstraintViolation::None();
}

// ====== Loopless ======
template <typename EdgeLabel, typename NodeLabel>
ConstraintViolation Loopless::CheckValue(
    const Graph<EdgeLabel, NodeLabel>& value) const {
  auto edges = value.GetEdges();
  for (const auto& [u, v, _] : edges) {
    if (u == v) {
      return ConstraintViolation(std::format("contains a loop at node {}", u));
    }
  }
  return ConstraintViolation::None();
}

// ====== SimpleGraph ======
template <typename EdgeLabel, typename NodeLabel>
ConstraintViolation SimpleGraph::CheckValue(
    const Graph<EdgeLabel, NodeLabel>& value) const {
  auto check1 = Loopless().CheckValue(value);
  if (!check1.IsOk()) {
    return ConstraintViolation(
        std::format("is not simple: {}", check1.Reason()));
  }
  auto check2 = NoParallelEdges().CheckValue(value);
  if (!check2.IsOk()) {
    return ConstraintViolation(
        std::format("is not simple: {}", check2.Reason()));
  }
  return ConstraintViolation::None();
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_CONSTRAINTS_GRAPH_CONSTRAINTS_H_
