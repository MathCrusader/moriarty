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

#ifndef MORIARTY_SRC_VARIABLES_MGRAPH_H_
#define MORIARTY_SRC_VARIABLES_MGRAPH_H_

#include <optional>
#include <string>

#include "src/contexts/librarian_context.h"
#include "src/librarian/mvariable.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/constraints/graph_constraints.h"
#include "src/variables/constraints/size_constraints.h"
#include "src/variables/graph.h"
#include "src/variables/minteger.h"

namespace moriarty {

// MGraph
//
// Describes constraints placed on an undirected graph.
class MGraph : public librarian::MVariable<MGraph, Graph<>> {
 public:
  // Create an MGraph from a set of constraints. Logically equivalent to
  // calling AddConstraint() for each constraint.
  //
  // E.g., MGraph(NumNodes(Between(1, "3 * N + 1")), Connected())
  template <typename... Constraints>
    requires(ConstraintFor<MGraph, Constraints> && ...)
  explicit MGraph(Constraints&&... constraints);

  ~MGraph() override = default;

  using MVariable<MGraph, Graph<>>::AddConstraint;  // Custom constraints

  // ---------------------------------------------------------------------------
  //  Constrain the value to a specific set of values

  // The graph must be exactly this value.
  MGraph& AddConstraint(Exactly<Graph<>> constraint);
  // The graph must be one of these values.
  MGraph& AddConstraint(OneOf<Graph<>> constraint);

  // ---------------------------------------------------------------------------
  //  Constrain the size of the graph

  // The number of nodes in the graph must satisfy this constraint.
  MGraph& AddConstraint(NumNodes constraint);
  // The number of edges in the graph must satisfy this constraint.
  MGraph& AddConstraint(NumEdges constraint);
  // The total size of the graph should be approximately this size.
  MGraph& AddConstraint(SizeCategory constraint);

  // ---------------------------------------------------------------------------
  //  Constrain typical graph theory items

  // The graph is connected
  MGraph& AddConstraint(Connected constraint);
  // The graph has no parallel edges
  MGraph& AddConstraint(NoParallelEdges constraint);
  // The graph has no self-loops
  MGraph& AddConstraint(Loopless constraint);
  // The graph is simple (no parallel edges or self-loops)
  MGraph& AddConstraint(SimpleGraph constraint);

  [[nodiscard]] std::string Typename() const override { return "MGraph"; }

 private:
  std::optional<MInteger> num_nodes_constraints_;
  std::optional<MInteger> num_edges_constraints_;
  bool is_connected_ = false;
  bool multi_edges_allowed_ = true;
  bool loops_allowed_ = true;

  // ---------------------------------------------------------------------------
  //  MVariable overrides
  Graph<> GenerateImpl(librarian::ResolverContext ctx) const override;
  Graph<> ReadImpl(librarian::ReaderContext ctx) const override;
  void PrintImpl(librarian::PrinterContext ctx,
                 const Graph<>& value) const override;
  std::optional<Graph<>> GetUniqueValueImpl(
      librarian::AnalysisContext ctx) const override;
  // ---------------------------------------------------------------------------
};

// -----------------------------------------------------------------------------
//  Implementation details
// -----------------------------------------------------------------------------

template <typename... Constraints>
  requires(ConstraintFor<MGraph, Constraints> && ...)
MGraph::MGraph(Constraints&&... constraints) {
  (AddConstraint(std::forward<Constraints>(constraints)), ...);
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_MGRAPH_H_
