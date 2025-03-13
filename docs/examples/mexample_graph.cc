// Copyright 2025 Darcy Best
// Copyright 2024 Google LLC
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

#include "docs/examples/mexample_graph.h"

#include <stdexcept>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "docs/examples/example_graph.h"
#include "src/contexts/librarian/analysis_context.h"
#include "src/contexts/librarian/resolver_context.h"
#include "src/errors.h"
#include "src/util/status_macro/status_macros.h"
#include "src/variables/constraints/numeric_constraints.h"
#include "src/variables/minteger.h"

namespace moriarty_examples {

MExampleGraph& MExampleGraph::WithNumNodes(moriarty::MInteger num_nodes) {
  if (num_nodes_.has_value()) {
    num_nodes_->MergeFrom(num_nodes);
  } else {
    num_nodes_ = num_nodes;
  }

  return *this;
}

MExampleGraph& MExampleGraph::WithNumEdges(moriarty::MInteger num_edges) {
  if (num_edges_.has_value()) {
    num_edges_->MergeFrom(num_edges);
  } else {
    num_edges_ = num_edges;
  }

  return *this;
}

MExampleGraph& MExampleGraph::IsConnected() {
  is_connected_ = true;
  return *this;
}

ExampleGraph MExampleGraph::GenerateImpl(
    moriarty::librarian::ResolverContext ctx) const {
  if (!num_nodes_.has_value()) {
    throw std::runtime_error("Number of nodes must be constrained");
  }
  if (!num_edges_.has_value()) {
    throw std::runtime_error("Number of edges must be constrained");
  }

  int num_nodes = num_nodes_->Generate(ctx.ForSubVariable("num_nodes"));
  int num_edges = num_edges_->Generate(ctx.ForSubVariable("num_edges"));

  if (is_connected_ && num_edges < num_nodes - 1) {
    moriarty::MInteger num_edges_copy = *num_edges_;
    num_edges_copy.AddConstraint(moriarty::AtLeast(num_nodes - 1));
    num_edges = num_edges_copy.Generate(ctx.ForSubVariable("num_edges"));
  }

  std::vector<std::pair<int, int>> edges;
  if (is_connected_) {
    // This part is not important to understanding Moriarty. We are simply
    // adding edges here to guarantee that we get a connected graph.
    for (int i = 1; i < num_nodes; i++)
      edges.push_back({ctx.RandomInteger(i), i});
  }

  while (edges.size() < num_edges) {
    int u = ctx.RandomInteger(num_nodes);
    int v = ctx.RandomInteger(num_nodes);
    edges.push_back({u, v});
  }

  return ExampleGraph({.num_nodes = num_nodes, .edges = edges});
}

absl::Status MExampleGraph::IsSatisfiedWithImpl(
    moriarty::librarian::AnalysisContext ctx, const ExampleGraph& g) const {
  if (num_nodes_.has_value()) {
    MORIARTY_RETURN_IF_ERROR(num_nodes_->IsSatisfiedWith(ctx, g.num_nodes));
  }

  if (num_edges_.has_value()) {
    MORIARTY_RETURN_IF_ERROR(num_edges_->IsSatisfiedWith(ctx, g.edges.size()));
  }

  if (is_connected_) {
    if (!GraphIsConnected(g)) {
      return moriarty::UnsatisfiedConstraintError("G is not connected");
    }
  }

  return absl::OkStatus();
}

absl::Status MExampleGraph::MergeFromImpl(const MExampleGraph& other) {
  if (other.num_nodes_.has_value()) WithNumNodes(*other.num_nodes_);
  if (other.num_edges_.has_value()) WithNumEdges(*other.num_edges_);
  if (other.is_connected_) IsConnected();

  return absl::OkStatus();
}

}  // namespace moriarty_examples
