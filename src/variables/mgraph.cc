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
#include "src/librarian/policies.h"
#include "src/types/graph.h"
#include "src/variables/minteger.h"

namespace moriarty {

MGraph& MGraph::AddConstraint(Exactly<Graph<>> constraint) {
  return InternalAddExactlyConstraint(std::move(constraint));
}

MGraph& MGraph::AddConstraint(OneOf<Graph<>> constraint) {
  return InternalAddOneOfConstraint(std::move(constraint));
}

MGraph& MGraph::AddConstraint(NumNodes constraint) {
  if (!core_constraints_.NumNodes())
    core_constraints_.data_.Mutable().num_nodes = MInteger();
  core_constraints_.data_.Mutable().num_nodes->MergeFrom(
      constraint.GetConstraints());
  return InternalAddConstraint(std::move(constraint));
}

MGraph& MGraph::AddConstraint(NumEdges constraint) {
  if (!core_constraints_.NumEdges())
    core_constraints_.data_.Mutable().num_edges = MInteger();
  core_constraints_.data_.Mutable().num_edges->MergeFrom(
      constraint.GetConstraints());
  return InternalAddConstraint(std::move(constraint));
}

MGraph& MGraph::AddConstraint(SizeCategory constraint) {
  return AddConstraint(NumNodes(constraint));
}

MGraph& MGraph::AddConstraint(Connected constraint) {
  core_constraints_.data_.Mutable().is_connected = true;
  return InternalAddConstraint(std::move(constraint));
}

MGraph& MGraph::AddConstraint(NoParallelEdges constraint) {
  core_constraints_.data_.Mutable().multi_edges_allowed = false;
  return InternalAddConstraint(std::move(constraint));
}

MGraph& MGraph::AddConstraint(Loopless constraint) {
  core_constraints_.data_.Mutable().loops_allowed = false;
  return InternalAddConstraint(std::move(constraint));
}

MGraph& MGraph::AddConstraint(SimpleGraph constraint) {
  core_constraints_.data_.Mutable().multi_edges_allowed = false;
  core_constraints_.data_.Mutable().loops_allowed = false;
  return InternalAddConstraint(std::move(constraint));
}

Graph<> MGraph::GenerateImpl(librarian::ResolverContext ctx) const {
  if (GetOneOf().HasBeenConstrained())
    return GetOneOf().SelectOneOf([&](int n) { return ctx.RandomInteger(n); });

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

  std::set<std::pair<Graph<>::NodeIdx, Graph<>::NodeIdx>> seen;
  Graph G(num_nodes);
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

Graph<> MGraph::ReadImpl(librarian::ReaderContext ctx) const {
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

  Graph G(*num_nodes);
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

void MGraph::PrintImpl(librarian::PrinterContext ctx,
                       const Graph<>& value) const {
  for (const auto& [u, v, _] : value.GetEdges()) {
    ctx.PrintToken(std::to_string(u));
    ctx.PrintWhitespace(Whitespace::kSpace);
    ctx.PrintToken(std::to_string(v));
    ctx.PrintWhitespace(Whitespace::kNewline);
  }
}

std::optional<Graph<>> MGraph::GetUniqueValueImpl(
    librarian::AnalysisContext ctx) const {
  auto option = GetOneOf().GetUniqueValue();
  if (option) return *option;

  std::optional<int64_t> nodes =
      core_constraints_.NumNodes()
          ? core_constraints_.NumNodes()->GetUniqueValue(ctx)
          : std::nullopt;
  std::optional<int64_t> edges =
      core_constraints_.NumEdges()
          ? core_constraints_.NumEdges()->GetUniqueValue(ctx)
          : std::nullopt;

  if (nodes && edges && *edges == 0) return Graph(*nodes);

  if (nodes && *nodes == 1 && edges) {  // Single node with lots of loops
    Graph G(*nodes);
    for (int64_t i = 0; i < *edges; ++i) {
      G.AddEdge(0, 0);
    }
    return G;
  }
  return std::nullopt;
}

void MGraph::PartialReader::ReadNext(librarian::ReaderContext ctx,
                                     int /*idx*/) {
  int u = ctx.ReadInteger();
  ctx.ReadWhitespace(Whitespace::kSpace);
  int v = ctx.ReadInteger();
  if (u < 0 || v < 0) ctx.ThrowIOError("Edge endpoints must be >= 0.");
  edges_.emplace_back(u, v);
}

Graph<> MGraph::PartialReader::Finalize() && {
  int max_n = 0;
  for (const auto& [u, v] : edges_) max_n = std::max({max_n, u, v});
  Graph<> g(max_n + 1);
  for (const auto& [u, v] : edges_) g.AddEdge(u, v);
  return g;
}

const std::optional<MInteger>& MGraph::CoreConstraints::NumNodes() const {
  return data_->num_nodes;
}

const std::optional<MInteger>& MGraph::CoreConstraints::NumEdges() const {
  return data_->num_edges;
}

bool MGraph::CoreConstraints::IsConnected() const {
  return data_->is_connected;
}

bool MGraph::CoreConstraints::MultiEdgesAllowed() const {
  return data_->multi_edges_allowed;
}

bool MGraph::CoreConstraints::LoopsAllowed() const {
  return data_->loops_allowed;
}

}  // namespace moriarty
