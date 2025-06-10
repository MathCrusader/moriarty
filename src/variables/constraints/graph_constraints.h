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

#include "src/contexts/librarian_context.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/graph.h"
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
  [[nodiscard]] bool IsSatisfiedWith(librarian::AnalysisContext ctx,
                                     const Graph<>& value) const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns a string explaining why the value does not satisfy the constraints.
  // It is assumed that IsSatisfiedWith() returned false.
  [[nodiscard]] std::string UnsatisfiedReason(librarian::AnalysisContext ctx,
                                              const Graph<>& value) const;

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
  [[nodiscard]] bool IsSatisfiedWith(librarian::AnalysisContext ctx,
                                     const Graph<>& value) const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns a string explaining why the value does not satisfy the constraints.
  // It is assumed that IsSatisfiedWith() returned false.
  [[nodiscard]] std::string UnsatisfiedReason(librarian::AnalysisContext ctx,
                                              const Graph<>& value) const;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const;

 private:
  MInteger num_edges_;
};

// The graph must have exactly one connected component.
//
// In particular, the graph with 0 nodes is *not* connected.
class Connected : public MConstraint {
 public:
  explicit Connected() {}

  // Determines if the graph is connected.
  [[nodiscard]] bool IsSatisfiedWith(librarian::AnalysisContext ctx,
                                     const Graph<>& value) const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns a string explaining why the value does not satisfy the constraints.
  // It is assumed that IsSatisfiedWith() returned false.
  [[nodiscard]] std::string UnsatisfiedReason(librarian::AnalysisContext ctx,
                                              const Graph<>& value) const;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const;
};

// The graph must contain no parallel edges. That is, the edge (u, v) is only
// present in the graph at most once. This limits the graph to ((n+1) choose
// 2) nodes if the graph contains loops and (n choose 2) if the graph is
// loopless.
class NoParallelEdges : public MConstraint {
 public:
  explicit NoParallelEdges() {}

  // Determines if the graph has the correct number of edges.
  [[nodiscard]] bool IsSatisfiedWith(librarian::AnalysisContext ctx,
                                     const Graph<>& value) const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns a string explaining why the value does not satisfy the constraints.
  // It is assumed that IsSatisfiedWith() returned false.
  [[nodiscard]] std::string UnsatisfiedReason(librarian::AnalysisContext ctx,
                                              const Graph<>& value) const;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const;
};

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

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_CONSTRAINTS_GRAPH_CONSTRAINTS_H_
