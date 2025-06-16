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

#include "src/variables/constraints/graph_constraints.h"

#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "src/contexts/librarian_context.h"
#include "src/variables/minteger.h"

namespace moriarty {

// ====== NumNodes ======
NumNodes::NumNodes(int64_t num_nodes) : num_nodes_(Exactly(num_nodes)) {}

NumNodes::NumNodes(std::string_view expression)
    : num_nodes_(Exactly(expression)) {}

MInteger NumNodes::GetConstraints() const { return num_nodes_; }

bool NumNodes::IsSatisfiedWith(librarian::AnalysisContext ctx,
                               const Graph<>& value) const {
  return num_nodes_.IsSatisfiedWith(
             ctx, static_cast<int64_t>(value.NumNodes())) == std::nullopt;
}

std::string NumNodes::ToString() const {
  return std::format("is a graph whose number of nodes {}",
                     num_nodes_.ToString());
}

std::vector<std::string> NumNodes::GetDependencies() const {
  return num_nodes_.GetDependencies();
}

std::string NumNodes::UnsatisfiedReason(librarian::AnalysisContext ctx,
                                        const Graph<>& value) const {
  return std::format(
      "number of nodes (which is {}) {}", value.NumNodes(),
      *num_nodes_.IsSatisfiedWith(ctx, static_cast<int64_t>(value.NumNodes())));
}

// ====== NumEdges ======
NumEdges::NumEdges(int64_t num_edges) : num_edges_(Exactly(num_edges)) {}

NumEdges::NumEdges(std::string_view expression)
    : num_edges_(Exactly(expression)) {}

MInteger NumEdges::GetConstraints() const { return num_edges_; }

bool NumEdges::IsSatisfiedWith(librarian::AnalysisContext ctx,
                               const Graph<>& value) const {
  return num_edges_.IsSatisfiedWith(
             ctx, static_cast<int64_t>(value.NumEdges())) == std::nullopt;
}

std::string NumEdges::ToString() const {
  return std::format("is a graph whose number of edges {}",
                     num_edges_.ToString());
}

std::vector<std::string> NumEdges::GetDependencies() const {
  return num_edges_.GetDependencies();
}

std::string NumEdges::UnsatisfiedReason(librarian::AnalysisContext ctx,
                                        const Graph<>& value) const {
  return std::format(
      "number of edges (which is {}) {}", value.NumEdges(),
      *num_edges_.IsSatisfiedWith(ctx, static_cast<int64_t>(value.NumEdges())));
}

namespace {

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

}  // namespace

// ====== Connected ======
bool Connected::IsSatisfiedWith(librarian::AnalysisContext ctx,
                                const Graph<>& value) const {
  if (value.NumNodes() == 0) return false;

  UnionFind uf(value.NumNodes());
  auto edges = value.GetEdges();
  for (const auto& [u, v, _] : edges) uf.unite(u, v);

  for (int i = 1; i < value.NumNodes(); ++i)
    if (uf.find(i) != uf.find(0)) return false;
  return true;
}

std::string Connected::ToString() const { return "is a connected graph"; }

std::vector<std::string> Connected::GetDependencies() const { return {}; }

std::string Connected::UnsatisfiedReason(librarian::AnalysisContext ctx,
                                         const Graph<>& value) const {
  return "is not connected";
}

// ====== NoParallelEdges ======
bool NoParallelEdges::IsSatisfiedWith(librarian::AnalysisContext ctx,
                                      const Graph<>& value) const {
  std::set<std::pair<Graph<>::NodeIdx, Graph<>::NodeIdx>> seen;
  auto edges = value.GetEdges();
  for (const auto& [u, v, _] : edges) {
    if (!seen.emplace(u, v).second) return false;
    if (u != v && !seen.emplace(v, u).second) return false;
  }
  return true;
}

std::string NoParallelEdges::ToString() const {
  return "does not contain any edge multiple times";
}

std::vector<std::string> NoParallelEdges::GetDependencies() const { return {}; }

std::string NoParallelEdges::UnsatisfiedReason(librarian::AnalysisContext ctx,
                                               const Graph<>& value) const {
  std::set<std::pair<Graph<>::NodeIdx, Graph<>::NodeIdx>> seen;
  auto edges = value.GetEdges();
  for (const auto& [u, v, _] : edges) {
    if (!seen.emplace(u, v).second)
      return std::format("contains the edge ({}, {}) multiple times", u, v);
    if (u != v && !seen.emplace(v, u).second)
      return std::format("contains the edge ({}, {}) multiple times", u, v);
  }
  return "contains some edge multiple times";
}

}  // namespace moriarty
