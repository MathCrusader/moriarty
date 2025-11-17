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

#include <cstdint>
#include <format>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>

#include "src/constraints/base_constraints.h"
#include "src/constraints/graph_constraints.h"
#include "src/constraints/numeric_constraints.h"
#include "src/constraints/size_constraints.h"
#include "src/contexts/librarian_context.h"
#include "src/librarian/errors.h"
#include "src/librarian/mvariable.h"
#include "src/librarian/policies.h"
#include "src/types/graph.h"
#include "src/variables/minteger.h"
#include "src/variables/mnone.h"

namespace moriarty {

// MGraph
//
// Describes constraints placed on an undirected graph.
template <typename MEdgeLabel = MNone, typename MNodeLabel = MNone>
class MGraph
    : public librarian::MVariable<MGraph<MEdgeLabel, MNodeLabel>,
                                  Graph<typename MEdgeLabel::value_type,
                                        typename MNodeLabel::value_type>> {
 public:
  using graph_type =
      Graph<typename MEdgeLabel::value_type, typename MNodeLabel::value_type>;
  class Reader;  // Forward declaration
  using chunked_reader_type = Reader;

  // Create an MGraph from a set of constraints. Logically equivalent to
  // calling AddConstraint() for each constraint.
  //
  // E.g., MGraph(NumNodes(Between(1, "3 * N + 1")), Connected())
  template <typename... Constraints>
    requires(ConstraintFor<MGraph, Constraints> && ...)
  explicit MGraph(Constraints&&... constraints);

  ~MGraph() override = default;

  using librarian::MVariable<MGraph,
                             graph_type>::AddConstraint;  // Custom constraints

  // ---------------------------------------------------------------------------
  //  Constrain the value to a specific set of values

  // The graph must be exactly this value.
  MGraph& AddConstraint(Exactly<graph_type> constraint);
  // The graph must be one of these values.
  MGraph& AddConstraint(OneOf<graph_type> constraint);

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

  [[nodiscard]] Reader CreateChunkedReader(int num_chunks) const {
    return Reader();
  }

  // MGraph::CoreConstraints
  //
  // A base set of constraints for `MGraph` that are used during generation.
  // Note: Returned references are invalidated after any non-const call to this
  // class or the corresponding `MGraph`.
  class CoreConstraints {
   public:
    const std::optional<MInteger>& NumNodes() const;
    const std::optional<MInteger>& NumEdges() const;
    bool IsConnected() const;
    bool MultiEdgesAllowed() const;
    bool LoopsAllowed() const;

   private:
    friend class MGraph;
    struct Data {
      std::optional<MInteger> num_nodes;
      std::optional<MInteger> num_edges;
      bool is_connected = false;
      bool multi_edges_allowed = true;
      bool loops_allowed = true;
    };
    librarian::CowPtr<Data> data_;
  };

 private:
  CoreConstraints core_constraints_;

  // ---------------------------------------------------------------------------
  //  MVariable overrides
  graph_type GenerateImpl(librarian::ResolverContext ctx) const override;
  graph_type ReadImpl(librarian::ReaderContext ctx) const override;
  void PrintImpl(librarian::PrinterContext ctx,
                 const graph_type& value) const override;
  std::optional<graph_type> GetUniqueValueImpl(
      librarian::AnalysisContext ctx) const override;
  // ---------------------------------------------------------------------------

 public:
  // Internal-to-Moriarty class. You shouldn't need to use this directly (its
  // API can and will change without warning).
  //
  // Allows the graph to be read in chunks at a time.
  // For now, this assumes edge list and each node is 0-based.
  //
  // In the future, we'll add options for:
  //  - Adjacency list, adjacency matrix, parent list (in trees), etc.
  class Reader {
   public:
    void ReadNext(librarian::ReaderContext ctx);
    graph_type Finalize() &&;

   private:
    std::vector<std::pair<int, int>> edges_;
  };
};

// -----------------------------------------------------------------------------
//  Implementation details
// -----------------------------------------------------------------------------

template <typename MEdgeLabel, typename MNodeLabel>
template <typename... Constraints>
  requires(ConstraintFor<MGraph<MEdgeLabel, MNodeLabel>, Constraints> && ...)
MGraph<MEdgeLabel, MNodeLabel>::MGraph(Constraints&&... constraints) {
  (this->AddConstraint(std::forward<Constraints>(constraints)), ...);
}

template <typename MEdgeLabel, typename MNodeLabel>
MGraph<MEdgeLabel, MNodeLabel>& MGraph<MEdgeLabel, MNodeLabel>::AddConstraint(
    Exactly<graph_type> constraint) {
  return this->InternalAddExactlyConstraint(std::move(constraint));
}

template <typename MEdgeLabel, typename MNodeLabel>
MGraph<MEdgeLabel, MNodeLabel>& MGraph<MEdgeLabel, MNodeLabel>::AddConstraint(
    OneOf<graph_type> constraint) {
  return this->InternalAddOneOfConstraint(std::move(constraint));
}

template <typename MEdgeLabel, typename MNodeLabel>
MGraph<MEdgeLabel, MNodeLabel>& MGraph<MEdgeLabel, MNodeLabel>::AddConstraint(
    NumNodes constraint) {
  if (!core_constraints_.NumNodes())
    core_constraints_.data_.Mutable().num_nodes = MInteger();
  core_constraints_.data_.Mutable().num_nodes->MergeFrom(
      constraint.GetConstraints());
  return this->InternalAddConstraint(std::move(constraint));
}

template <typename MEdgeLabel, typename MNodeLabel>
MGraph<MEdgeLabel, MNodeLabel>& MGraph<MEdgeLabel, MNodeLabel>::AddConstraint(
    NumEdges constraint) {
  if (!core_constraints_.NumEdges())
    core_constraints_.data_.Mutable().num_edges = MInteger();
  core_constraints_.data_.Mutable().num_edges->MergeFrom(
      constraint.GetConstraints());
  return this->InternalAddConstraint(std::move(constraint));
}

template <typename MEdgeLabel, typename MNodeLabel>
MGraph<MEdgeLabel, MNodeLabel>& MGraph<MEdgeLabel, MNodeLabel>::AddConstraint(
    SizeCategory constraint) {
  return this->AddConstraint(NumNodes(constraint));
}

template <typename MEdgeLabel, typename MNodeLabel>
MGraph<MEdgeLabel, MNodeLabel>& MGraph<MEdgeLabel, MNodeLabel>::AddConstraint(
    Connected constraint) {
  core_constraints_.data_.Mutable().is_connected = true;
  return this->InternalAddConstraint(std::move(constraint));
}

template <typename MEdgeLabel, typename MNodeLabel>
MGraph<MEdgeLabel, MNodeLabel>& MGraph<MEdgeLabel, MNodeLabel>::AddConstraint(
    NoParallelEdges constraint) {
  core_constraints_.data_.Mutable().multi_edges_allowed = false;
  return this->InternalAddConstraint(std::move(constraint));
}

template <typename MEdgeLabel, typename MNodeLabel>
MGraph<MEdgeLabel, MNodeLabel>& MGraph<MEdgeLabel, MNodeLabel>::AddConstraint(
    Loopless constraint) {
  core_constraints_.data_.Mutable().loops_allowed = false;
  return this->InternalAddConstraint(std::move(constraint));
}

template <typename MEdgeLabel, typename MNodeLabel>
MGraph<MEdgeLabel, MNodeLabel>& MGraph<MEdgeLabel, MNodeLabel>::AddConstraint(
    SimpleGraph constraint) {
  core_constraints_.data_.Mutable().multi_edges_allowed = false;
  core_constraints_.data_.Mutable().loops_allowed = false;
  return this->InternalAddConstraint(std::move(constraint));
}

template <typename MEdgeLabel, typename MNodeLabel>
typename MGraph<MEdgeLabel, MNodeLabel>::graph_type
MGraph<MEdgeLabel, MNodeLabel>::GenerateImpl(
    librarian::ResolverContext ctx) const {
  if (this->GetOneOf().HasBeenConstrained())
    return this->GetOneOf().SelectOneOf(
        [&](int n) { return ctx.RandomInteger(n); });

  // TODO: These checks are too strong when we constrain in other ways (simple,
  // tree, connected, planar, etc)
  if (!core_constraints_.NumNodes()) {
    throw GenerationError(ctx.GetVariableName(),
                          "Need NumNodes() to generate a graph",
                          RetryPolicy::kAbort);
  }
  if (!core_constraints_.NumEdges()) {
    throw GenerationError(ctx.GetVariableName(),
                          "Need NumEdges() to generate a graph",
                          RetryPolicy::kAbort);
  }

  MInteger node_con = *core_constraints_.NumNodes();
  node_con.AddConstraint(AtLeast(0));

  int64_t num_nodes = node_con.Generate(ctx.ForSubVariable("num_nodes"));

  MInteger edge_con = *core_constraints_.NumEdges();
  edge_con.AddConstraint(AtLeast(0));
  if (core_constraints_.IsConnected())
    edge_con.AddConstraint(AtLeast(num_nodes - 1));
  if (!core_constraints_.MultiEdgesAllowed()) {
    if (core_constraints_.LoopsAllowed())
      edge_con.AddConstraint(AtMost((num_nodes * (num_nodes + 1)) / 2));
    else
      edge_con.AddConstraint(AtMost((num_nodes * (num_nodes - 1)) / 2));
  }
  int64_t num_edges = edge_con.Generate(ctx.ForSubVariable("num_edges"));

  if (num_nodes == 0 && num_edges > 0) {
    throw GenerationError(ctx.GetVariableName(),
                          "Cannot generate a graph with 0 nodes and >0 edges",
                          RetryPolicy::kAbort);
  }

  std::set<
      std::pair<typename graph_type::NodeIdx, typename graph_type::NodeIdx>>
      seen;
  graph_type G(num_nodes);
  if (core_constraints_.IsConnected()) {
    for (int64_t i = 1; i < num_nodes; ++i) {
      int64_t v = ctx.RandomInteger(i);
      seen.emplace(i, v);
      seen.emplace(v, i);
      G.AddEdge(i, v);
    }
  }

  while (G.NumEdges() < num_edges) {
    int64_t u = ctx.RandomInteger(num_nodes);
    int64_t v = ctx.RandomInteger(num_nodes);
    if (!core_constraints_.LoopsAllowed() && u == v) continue;
    if (!core_constraints_.MultiEdgesAllowed() && !seen.emplace(u, v).second)
      continue;
    seen.emplace(v, u);
    G.AddEdge(u, v);
  }

  return G;
}

template <typename MEdgeLabel, typename MNodeLabel>
typename MGraph<MEdgeLabel, MNodeLabel>::graph_type
MGraph<MEdgeLabel, MNodeLabel>::ReadImpl(librarian::ReaderContext ctx) const {
  if (!core_constraints_.NumNodes())
    ctx.ThrowIOError("Unknown number of nodes before read.");
  std::optional<int64_t> num_nodes =
      core_constraints_.NumNodes()->GetUniqueValue(ctx);
  if (!num_nodes)
    ctx.ThrowIOError("Cannot determine the number of nodes before read.");

  if (!core_constraints_.NumEdges())
    ctx.ThrowIOError("Unknown number of edges before read.");
  std::optional<int64_t> num_edges =
      core_constraints_.NumEdges()->GetUniqueValue(ctx);
  if (!num_edges)
    ctx.ThrowIOError("Cannot determine the number of edges before read.");

  graph_type G(*num_nodes);
  for (int64_t i = 0; i < *num_edges; ++i) {
    int64_t u = ctx.ReadInteger();  // TODO: Read NodeLabels eventually
    ctx.ReadWhitespace(Whitespace::kSpace);
    int64_t v = ctx.ReadInteger();
    ctx.ReadWhitespace(Whitespace::kNewline);
    if (u < 0 || v < 0 || u >= *num_nodes || v >= *num_nodes) {
      ctx.ThrowIOError(std::format(
          "Invalid edge ({}, {}) for graph with {} nodes.", u, v, *num_nodes));
    }
    G.AddEdge(u, v);
  }

  return G;
}

template <typename MEdgeLabel, typename MNodeLabel>
void MGraph<MEdgeLabel, MNodeLabel>::PrintImpl(librarian::PrinterContext ctx,
                                               const graph_type& value) const {
  for (const auto& [u, v, _] : value.GetEdges()) {
    ctx.PrintToken(std::to_string(u));
    ctx.PrintWhitespace(Whitespace::kSpace);
    ctx.PrintToken(std::to_string(v));
    ctx.PrintWhitespace(Whitespace::kNewline);
  }
}

template <typename MEdgeLabel, typename MNodeLabel>
std::optional<typename MGraph<MEdgeLabel, MNodeLabel>::graph_type>
MGraph<MEdgeLabel, MNodeLabel>::GetUniqueValueImpl(
    librarian::AnalysisContext ctx) const {
  auto option = this->GetOneOf().GetUniqueValue();
  if (option) return *option;

  std::optional<int64_t> nodes =
      core_constraints_.NumNodes()
          ? core_constraints_.NumNodes()->GetUniqueValue(ctx)
          : std::nullopt;
  std::optional<int64_t> edges =
      core_constraints_.NumEdges()
          ? core_constraints_.NumEdges()->GetUniqueValue(ctx)
          : std::nullopt;

  if (nodes && edges && *edges == 0) return graph_type(*nodes);

  if (nodes && *nodes == 1 && edges) {  // Single node with lots of loops
    graph_type G(*nodes);
    for (int64_t i = 0; i < *edges; ++i) {
      G.AddEdge(0, 0);
    }
    return G;
  }
  return std::nullopt;
}

template <typename MEdgeLabel, typename MNodeLabel>
void MGraph<MEdgeLabel, MNodeLabel>::Reader::ReadNext(
    librarian::ReaderContext ctx) {
  int u = ctx.ReadInteger();
  ctx.ReadWhitespace(Whitespace::kSpace);
  int v = ctx.ReadInteger();
  if (u < 0 || v < 0) ctx.ThrowIOError("Edge endpoints must be >= 0.");
  edges_.emplace_back(u, v);
}

template <typename MEdgeLabel, typename MNodeLabel>
typename MGraph<MEdgeLabel, MNodeLabel>::graph_type
MGraph<MEdgeLabel, MNodeLabel>::Reader::Finalize() && {
  int max_n = 0;
  for (const auto& [u, v] : edges_) max_n = std::max({max_n, u, v});
  graph_type g(max_n + 1);
  for (const auto& [u, v] : edges_) g.AddEdge(u, v);
  return g;
}

template <typename MEdgeLabel, typename MNodeLabel>
const std::optional<MInteger>&
MGraph<MEdgeLabel, MNodeLabel>::CoreConstraints::NumNodes() const {
  return data_->num_nodes;
}

template <typename MEdgeLabel, typename MNodeLabel>
const std::optional<MInteger>&
MGraph<MEdgeLabel, MNodeLabel>::CoreConstraints::NumEdges() const {
  return data_->num_edges;
}

template <typename MEdgeLabel, typename MNodeLabel>
bool MGraph<MEdgeLabel, MNodeLabel>::CoreConstraints::IsConnected() const {
  return data_->is_connected;
}

template <typename MEdgeLabel, typename MNodeLabel>
bool MGraph<MEdgeLabel, MNodeLabel>::CoreConstraints::MultiEdgesAllowed()
    const {
  return data_->multi_edges_allowed;
}

template <typename MEdgeLabel, typename MNodeLabel>
bool MGraph<MEdgeLabel, MNodeLabel>::CoreConstraints::LoopsAllowed() const {
  return data_->loops_allowed;
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_MGRAPH_H_
