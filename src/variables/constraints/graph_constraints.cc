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

#include <format>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "src/contexts/librarian_context.h"
#include "src/variables/constraints/constraint_violation.h"
#include "src/variables/minteger.h"

namespace moriarty {

// ====== NumNodes ======
NumNodes::NumNodes(int64_t num_nodes) : num_nodes_(Exactly(num_nodes)) {}

NumNodes::NumNodes(std::string_view expression)
    : num_nodes_(Exactly(expression)) {}

MInteger NumNodes::GetConstraints() const { return num_nodes_; }

ConstraintViolation NumNodes::CheckValue(librarian::AnalysisContext ctx,
                                         const Graph<>& value) const {
  auto check =
      num_nodes_.CheckValue(ctx, static_cast<int64_t>(value.NumNodes()));
  if (check.IsOk()) return ConstraintViolation::None();
  return ConstraintViolation(std::format("number of nodes (which is {}) {}",
                                         value.NumNodes(), check.Reason()));
}

std::string NumNodes::ToString() const {
  return std::format("is a graph whose number of nodes {}",
                     num_nodes_.ToString());
}

std::vector<std::string> NumNodes::GetDependencies() const {
  return num_nodes_.GetDependencies();
}

// ====== NumEdges ======
NumEdges::NumEdges(int64_t num_edges) : num_edges_(Exactly(num_edges)) {}

NumEdges::NumEdges(std::string_view expression)
    : num_edges_(Exactly(expression)) {}

MInteger NumEdges::GetConstraints() const { return num_edges_; }

ConstraintViolation NumEdges::CheckValue(librarian::AnalysisContext ctx,
                                         const Graph<>& value) const {
  auto check =
      num_edges_.CheckValue(ctx, static_cast<int64_t>(value.NumEdges()));
  if (check.IsOk()) return ConstraintViolation::None();
  return ConstraintViolation(std::format("number of edges (which is {}) {}",
                                         value.NumEdges(), check.Reason()));
}

std::string NumEdges::ToString() const {
  return std::format("is a graph whose number of edges {}",
                     num_edges_.ToString());
}

std::vector<std::string> NumEdges::GetDependencies() const {
  return num_edges_.GetDependencies();
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
ConstraintViolation Connected::CheckValue(const Graph<>& value) const {
  if (value.NumNodes() == 0)
    return ConstraintViolation(
        "is not connected (a graph with 0 nodes is not considered connected)");

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
ConstraintViolation NoParallelEdges::CheckValue(const Graph<>& value) const {
  std::set<std::pair<Graph<>::NodeIdx, Graph<>::NodeIdx>> seen;
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
ConstraintViolation Loopless::CheckValue(const Graph<>& value) const {
  auto edges = value.GetEdges();
  for (const auto& [u, v, _] : edges) {
    if (u == v) {
      return ConstraintViolation(std::format("contains a loop at node {}", u));
    }
  }
  return ConstraintViolation::None();
}

// ====== SimpleGraph ======
ConstraintViolation SimpleGraph::CheckValue(const Graph<>& value) const {
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
