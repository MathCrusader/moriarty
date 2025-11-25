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
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "src/constraints/base_constraints.h"
#include "src/constraints/graph_constraints.h"
#include "src/constraints/numeric_constraints.h"
#include "src/constraints/size_constraints.h"
#include "src/contexts/librarian_context.h"
#include "src/librarian/errors.h"
#include "src/librarian/io_config.h"
#include "src/librarian/mvariable.h"
#include "src/librarian/policies.h"
#include "src/librarian/util/debug_string.h"
#include "src/types/graph.h"
#include "src/variables/minteger.h"
#include "src/variables/mnone.h"

namespace moriarty {

using MNoEdgeLabel = MNone;
using MNoNodeLabel = MNone;

// MGraphFormat
//
// Each graph has two configurable aspects to its format:
//
// Overall style:
//   - Edge list: each edge is listed on its own line.
//   - Adjacency matrix: the graph is represented as a matrix.
// Node style:
//   - 0-based indexing: nodes are numbered from 0 to N-1.
//   - 1-based indexing: nodes are numbered from 1 to N.
//   - Node labels: nodes are labelled according to their node labels.
class MGraphFormat {
 public:
  // Sets the format to edge list. Each edge is listed on its own line.
  // If the graph has no edge labels, each line contains two node values.
  // If the graph has edge labels, each line contains three items: two node
  // values and the edge label.
  //
  // This is the default.
  MGraphFormat& EdgeList();
  // Returns if the format is an edge list. See `EdgeList()`.
  bool IsEdgeList() const;

  // Sets the format to adjacency matrix. The graph is represented as a matrix.
  // The entries of the matrix depend on the underlying graph. The matrix must
  // always be symmetric for undirected graphs.
  //
  // Unlabelled graphs:
  //   an integer representing the number of edges between nodes.
  // Labelled graphs:
  //   the label of the edge between nodes, or a special value representing no
  //   edge (currently "0", but this may be configurable in the future).
  MGraphFormat& AdjacencyMatrix();
  // Returns if the format is an adjacency matrix. See `AdjacencyMatrix()`.
  bool IsAdjacencyMatrix() const;

  // Sets the node style to 0-based indexing. Nodes are numbered from 0 to N-1.
  //
  // This is the default.
  MGraphFormat& ZeroBased();
  // Returns if the node style is 0-based indexing. See `ZeroBased()`.
  bool IsZeroBased() const;

  // Sets the node style to 1-based indexing. Nodes are numbered from 1 to N.
  MGraphFormat& OneBased();
  // Returns if the node style is 1-based indexing. See `OneBased()`.
  bool IsOneBased() const;

  // Sets the node style to use node labels. Nodes are labelled according to
  // their node labels. The second template argument of MGraph<___, MNodeLabel>
  // indicates the type of the node labels.
  MGraphFormat& NodeLabelsStyle();
  // Returns if the node style is node labels. See `NodeLabelsStyle()`.
  bool IsNodeLabelsStyle() const;

  // Take any non-defaults in `other` and apply them to this format.
  void Merge(const MGraphFormat& other);

 private:
  enum class Style { kEdgeList, kAdjacencyMatrix, kNonExhaustiveList };
  enum class NodeStyle { k0Based, k1Based, kNodeLabels, kNonExhaustiveList };

  Style style_ = Style::kEdgeList;
  NodeStyle node_style_ = NodeStyle::k0Based;
  MGraphFormat& SetStyle(Style style);
  MGraphFormat& SetNodeStyle(NodeStyle node_style);
};

// MGraph
//
// Describes constraints placed on an undirected graph. By default, graphs have
// no edge or node labels. To add labels, provide MEdgeLabel and MNodeLabel
// types that are Moriarty variables.
//
// Typical "labels" are weights, capacities, colours, names, etc.
//
// Use tuples for multi-dimensional labels, e.g., flow networks with capacity
// and cost. MGraph<MTuple<MInteger, MInteger>> would be a graph where each edge
// has a capacity and cost.
template <typename MEdgeLabel = MNoEdgeLabel,
          typename MNodeLabel = MNoNodeLabel>
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

  // ---------------------------------------------------------------------------
  //  Built-in items

  // Returns "MGraph". Used in generic debugging/error messages.
  [[nodiscard]] std::string Typename() const override { return "MGraph"; }

  // Custom constraints (see `AddCustomConstraint` in MVariable)
  using librarian::MVariable<MGraph, graph_type>::AddConstraint;

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

  // ---------------------------------------------------------------------------
  //  Constrain edge/node labels

  // All edge labels in the graph must satisfy this constraint.
  MGraph& AddConstraint(EdgeLabels<MEdgeLabel> constraint);
  // All node labels in the graph must satisfy this constraint.
  MGraph& AddConstraint(NodeLabels<MNodeLabel> constraint);

  // ---------------------------------------------------------------------------
  //  Constrain the I/O format of the graph

  // Change the I/O format of the graph.
  // Note: I/O constraints behave as overrides instead of merges.
  MGraph& AddConstraint(MGraphFormat constraint);
  // Returns the I/O format for this graph.
  [[nodiscard]] MGraphFormat& Format();
  // Returns the I/O format for this graph.
  [[nodiscard]] MGraphFormat Format() const;

  // MGraph::CoreConstraints
  //
  // A base set of constraints for `MGraph` that are used during generation.
  // Note: Returned references are invalidated after any non-const call to this
  // class or the corresponding `MGraph`.
  class CoreConstraints {
   public:
    // Returns all constraints on how many nodes are in the graph.
    [[nodiscard]] const MInteger& NumNodes() const;
    // Returns whether the number of nodes has been constrained.
    [[nodiscard]] bool NumNodesConstrained() const;

    // Returns all constraints on how many edges are in the graph.
    [[nodiscard]] const MInteger& NumEdges() const;
    // Returns whether the number of edges has been constrained.
    [[nodiscard]] bool NumEdgesConstrained() const;

    // Returns all constraints on edge labels in the graph.
    [[nodiscard]] const MEdgeLabel& EdgeLabels() const;
    // Returns whether edge labels have been constrained.
    [[nodiscard]] bool EdgeLabelsConstrained() const;

    // Returns all constraints on node labels in the graph.
    [[nodiscard]] const MNodeLabel& NodeLabels() const;
    // Returns whether node labels have been constrained.
    [[nodiscard]] bool NodeLabelsConstrained() const;

    // Returns whether the graph is constrained to be connected.
    [[nodiscard]] bool IsConnected() const;
    // Returns whether multi-edges are allowed in the graph.
    [[nodiscard]] bool MultiEdgesAllowed() const;
    // Returns whether loops are allowed in the graph.
    [[nodiscard]] bool LoopsAllowed() const;

   private:
    friend class MGraph;
    enum Flags : uint32_t {
      kNumNodes = 1 << 0,
      kNumEdges = 1 << 1,
      kEdgeLabels = 1 << 2,
      kNodeLabels = 1 << 3,

      kIsConnected = 1 << 4,           // Default: false
      kMultiEdgesDisallowed = 1 << 5,  // Default: false
      kLoopsDisallowed = 1 << 6,       // Default: false
    };
    struct Data {
      std::underlying_type_t<Flags> touched = 0;

      MInteger num_nodes;
      MInteger num_edges;
      MEdgeLabel edge_label_constraints;
      MNodeLabel node_label_constraints;
    };
    librarian::CowPtr<Data> data_;
    bool IsSet(Flags flag) const;
  };
  [[nodiscard]] CoreConstraints GetCoreConstraints() const;

  // MGraph::Reader
  //
  // Reads a graph value from a stream in chunks.
  class Reader {
   public:
    explicit Reader(librarian::ReaderContext ctx, int num_chunks,
                    Ref<const MGraph> variable);
    void ReadNext(librarian::ReaderContext ctx);
    graph_type Finalize() &&;

   private:
    graph_type G_;
    int64_t chunks_read_ = 0;
    Ref<const MGraph> variable_;

    // We need to use indices in std::get<>, since it's possible they are the
    // same type.
    static constexpr int IMtxIdx = 0;
    static constexpr int LMtxIdx = 1;
    using IMtx = std::vector<std::vector<int64_t>>;
    using LMtx = std::vector<std::vector<typename MEdgeLabel::value_type>>;
    std::variant<IMtx, LMtx> adjacency_matrix_;

    graph_type::NodeIdx ReadNodeLabel(librarian::ReaderContext ctx);
    MEdgeLabel::value_type ReadEdgeLabel(librarian::ReaderContext ctx);

    void ReadNextEdgeList(librarian::ReaderContext ctx);
    void ReadNextAdjacencyMatrix(librarian::ReaderContext ctx);
    graph_type FinalizeEdgeList() &&;
    graph_type FinalizeAdjacencyMatrix() &&;
  };

 private:
  CoreConstraints core_constraints_;
  MGraphFormat format_;

  // ---------------------------------------------------------------------------
  //  MVariable overrides
  graph_type GenerateImpl(librarian::ResolverContext ctx) const override;
  graph_type ReadImpl(librarian::ReaderContext ctx) const override;
  void PrintImpl(librarian::PrinterContext ctx,
                 const graph_type& value) const override;
  std::optional<graph_type> GetUniqueValueImpl(
      librarian::AnalysisContext ctx) const override;
  // ---------------------------------------------------------------------------
};

// -----------------------------------------------------------------------------
//  Implementation details
// -----------------------------------------------------------------------------

template <typename MEdgeLabel>
concept HasEdgeLabels = !std::is_same_v<MEdgeLabel, MNoEdgeLabel>;

template <typename MNodeLabel>
concept HasNodeLabels = !std::is_same_v<MNodeLabel, MNoNodeLabel>;

template <typename MEdgeLabel, typename MNodeLabel>
template <typename... Constraints>
  requires(ConstraintFor<MGraph<MEdgeLabel, MNodeLabel>, Constraints> && ...)
MGraph<MEdgeLabel, MNodeLabel>::MGraph(Constraints&&... constraints) {
  static_assert(MoriartyVariable<MEdgeLabel> && MoriartyVariable<MNodeLabel>,
                "The T and U used in MGraph<T, U> must be a Moriarty variable. "
                "For example, "
                "MGraph<MInteger, MString> or MGraph<MCustomType, MNone>.");
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
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kNumNodes;
  constraints.num_nodes.MergeFrom(constraint.GetConstraints());
  return this->InternalAddConstraint(std::move(constraint));
}

template <typename MEdgeLabel, typename MNodeLabel>
MGraph<MEdgeLabel, MNodeLabel>& MGraph<MEdgeLabel, MNodeLabel>::AddConstraint(
    NumEdges constraint) {
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kNumEdges;
  constraints.num_edges.MergeFrom(constraint.GetConstraints());
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
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kIsConnected;
  return this->InternalAddConstraint(std::move(constraint));
}

template <typename MEdgeLabel, typename MNodeLabel>
MGraph<MEdgeLabel, MNodeLabel>& MGraph<MEdgeLabel, MNodeLabel>::AddConstraint(
    NoParallelEdges constraint) {
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kMultiEdgesDisallowed;
  return this->InternalAddConstraint(std::move(constraint));
}

template <typename MEdgeLabel, typename MNodeLabel>
MGraph<MEdgeLabel, MNodeLabel>& MGraph<MEdgeLabel, MNodeLabel>::AddConstraint(
    Loopless constraint) {
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kLoopsDisallowed;
  return this->InternalAddConstraint(std::move(constraint));
}

template <typename MEdgeLabel, typename MNodeLabel>
MGraph<MEdgeLabel, MNodeLabel>& MGraph<MEdgeLabel, MNodeLabel>::AddConstraint(
    SimpleGraph constraint) {
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kMultiEdgesDisallowed;
  constraints.touched |= CoreConstraints::Flags::kLoopsDisallowed;
  return this->InternalAddConstraint(std::move(constraint));
}

template <typename MEdgeLabel, typename MNodeLabel>
MGraph<MEdgeLabel, MNodeLabel>& MGraph<MEdgeLabel, MNodeLabel>::AddConstraint(
    EdgeLabels<MEdgeLabel> constraint) {
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kEdgeLabels;
  constraints.edge_label_constraints.MergeFrom(constraint.GetConstraints());
  return this->InternalAddConstraint(std::move(constraint));
}

template <typename MEdgeLabel, typename MNodeLabel>
MGraph<MEdgeLabel, MNodeLabel>& MGraph<MEdgeLabel, MNodeLabel>::AddConstraint(
    NodeLabels<MNodeLabel> constraint) {
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kNodeLabels;
  constraints.node_label_constraints.MergeFrom(constraint.GetConstraints());
  return this->InternalAddConstraint(std::move(constraint));
}

template <typename MEdgeLabel, typename MNodeLabel>
MGraph<MEdgeLabel, MNodeLabel>& MGraph<MEdgeLabel, MNodeLabel>::AddConstraint(
    MGraphFormat constraint) {
  Format().Merge(std::move(constraint));
  return *this;
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
  if (!core_constraints_.NumNodesConstrained()) {
    throw GenerationError(ctx.GetVariableName(),
                          "Need NumNodes() to generate a graph",
                          RetryPolicy::kAbort);
  }
  if (!core_constraints_.NumEdgesConstrained()) {
    throw GenerationError(ctx.GetVariableName(),
                          "Need NumEdges() to generate a graph",
                          RetryPolicy::kAbort);
  }

  MInteger node_con = core_constraints_.NumNodes();
  node_con.AddConstraint(AtLeast(0));

  int64_t num_nodes = node_con.Generate(ctx.ForSubVariable("num_nodes"));

  MInteger edge_con = core_constraints_.NumEdges();
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

  auto add_edge_with_label = [this, &G, &seen, &ctx](int64_t u, int64_t v) {
    seen.emplace(u, v);
    seen.emplace(v, u);

    if constexpr (!HasEdgeLabels<MEdgeLabel>) {
      G.AddEdge(u, v);
    } else {
      // Generate edge label
      auto edge_label = core_constraints_.EdgeLabels().Generate(
          ctx.ForSubVariable("edge_label"));
      G.AddEdge(u, v, edge_label);
    }
  };

  if (core_constraints_.IsConnected()) {
    for (int64_t i = 1; i < num_nodes; ++i) {
      int64_t v = ctx.RandomInteger(i);
      add_edge_with_label(i, v);
    }
  }

  while (G.NumEdges() < num_edges) {
    int64_t u = ctx.RandomInteger(num_nodes);
    int64_t v = ctx.RandomInteger(num_nodes);
    if (!core_constraints_.LoopsAllowed() && u == v) continue;
    if (!core_constraints_.MultiEdgesAllowed() && !seen.emplace(u, v).second)
      continue;
    add_edge_with_label(u, v);
  }

  if constexpr (HasNodeLabels<MNodeLabel>) {
    std::vector<typename MNodeLabel::value_type> node_labels;
    node_labels.reserve(num_nodes);
    for (int64_t i = 0; i < num_nodes; ++i) {
      auto node_label = core_constraints_.NodeLabels().Generate(
          ctx.ForSubVariable(std::format("node_label_{}", i)));
      node_labels.push_back(node_label);
    }
    G.SetNodeLabels(node_labels);
  }

  return G;
}

template <typename MEdgeLabel, typename MNodeLabel>
typename MGraph<MEdgeLabel, MNodeLabel>::graph_type
MGraph<MEdgeLabel, MNodeLabel>::ReadImpl(librarian::ReaderContext ctx) const {
  std::optional<int64_t> num_nodes =
      core_constraints_.NumNodes().GetUniqueValue(ctx);
  if (!num_nodes)
    ctx.ThrowIOError("Cannot determine the number of nodes before read.");

  if (Format().IsAdjacencyMatrix()) {
    MGraph::Reader reader(ctx, *num_nodes, *this);
    for (int64_t i = 0; i < *num_nodes; i++) {
      reader.ReadNext(ctx);
      ctx.ReadWhitespace(Whitespace::kNewline);
    }
    return std::move(reader).Finalize();
  }
  std::optional<int64_t> num_edges =
      core_constraints_.NumEdges().GetUniqueValue(ctx);
  if (!num_edges)
    ctx.ThrowIOError("Cannot determine the number of edges before read.");

  if (Format().IsEdgeList()) {
    MGraph::Reader reader(ctx, *num_edges, *this);
    for (int64_t i = 0; i < *num_edges; i++) {
      reader.ReadNext(ctx);
      ctx.ReadWhitespace(Whitespace::kNewline);
    }
    return std::move(reader).Finalize();
  }

  ctx.ThrowIOError("Unsupported MGraph format for reading.");
  return graph_type(0);  // Unreachable
}

template <typename MEdgeLabel, typename MNodeLabel>
void MGraph<MEdgeLabel, MNodeLabel>::PrintImpl(librarian::PrinterContext ctx,
                                               const graph_type& value) const {
  if (format_.IsEdgeList()) {
    auto node_labels = value.GetNodeLabels();
    auto print_node = [this, &ctx,
                       &node_labels](typename graph_type::NodeIdx node) {
      if (format_.IsZeroBased()) {
        ctx.PrintToken(std::to_string(node));
      } else if (format_.IsOneBased()) {
        ctx.PrintToken(std::to_string(node + 1));
      } else if (format_.IsNodeLabelsStyle()) {
        // Node labels
        if constexpr (HasNodeLabels<MNodeLabel>) {
          core_constraints_.NodeLabels().Print(ctx, node_labels[node]);
        } else {
          // FIXME: This should be an IOError.
          throw std::runtime_error(
              "Cannot print node labels when MNodeLabel is MNone.");
        }
      } else {
        throw std::runtime_error("Unreachable code reached.");
      }
    };
    for (const auto& [u, v, w] : value.GetEdges()) {
      print_node(u);
      ctx.PrintWhitespace(Whitespace::kSpace);
      print_node(v);
      if constexpr (HasEdgeLabels<MEdgeLabel>) {
        ctx.PrintWhitespace(Whitespace::kSpace);
        core_constraints_.EdgeLabels().Print(ctx, w);
      }
      ctx.PrintWhitespace(Whitespace::kNewline);
    }
    return;
  }

  if (format_.IsAdjacencyMatrix()) {
    auto adjacency_list = value.GetAdjacencyList();
    if constexpr (!HasEdgeLabels<MEdgeLabel>) {
      std::vector<std::vector<int64_t>> matrix(
          adjacency_list.size(),
          std::vector<int64_t>(adjacency_list.size(), 0));
      for (size_t u = 0; u < adjacency_list.size(); ++u) {
        for (auto [_, v, w] : adjacency_list[u]) {
          matrix[u][v]++;
        }
      }
      for (size_t u = 0; u < adjacency_list.size(); ++u) {
        for (size_t v = 0; v < adjacency_list.size(); ++v) {
          if (v > 0) ctx.PrintWhitespace(Whitespace::kSpace);
          ctx.PrintToken(std::to_string(matrix[u][v]));
        }
        ctx.PrintWhitespace(Whitespace::kNewline);
      }
    } else {  // HasEdgeLabels == true
      std::vector<std::vector<std::optional<typename MEdgeLabel::value_type>>>
          matrix(value.NumNodes(),
                 std::vector<std::optional<typename MEdgeLabel::value_type>>(
                     value.NumNodes()));
      for (const auto& row : adjacency_list) {
        for (auto [u, v, w] : row) {
          if (matrix[u][v].has_value()) {
            // FIXME: This should be an IOError.
            throw std::runtime_error(
                "Cannot print adjacency matrix with multiple edges "
                "between nodes when edge labels are present.");
          }
          matrix[u][v] = w;
        }
      }
      for (size_t u = 0; u < value.NumNodes(); ++u) {
        for (size_t v = 0; v < value.NumNodes(); ++v) {
          if (v > 0) ctx.PrintWhitespace(Whitespace::kSpace);
          if (!matrix[u][v]) {
            ctx.PrintToken("0");  // FIXME: Better representation of no edge
          } else {
            core_constraints_.EdgeLabels().Print(ctx, *matrix[u][v]);
          }
        }
        ctx.PrintWhitespace(Whitespace::kNewline);
      }
    }
  }
}

template <typename MEdgeLabel, typename MNodeLabel>
std::optional<typename MGraph<MEdgeLabel, MNodeLabel>::graph_type>
MGraph<MEdgeLabel, MNodeLabel>::GetUniqueValueImpl(
    librarian::AnalysisContext ctx) const {
  auto option = this->GetOneOf().GetUniqueValue();
  if (option) return *option;

  std::optional<int64_t> nodes =
      core_constraints_.NumNodes().GetUniqueValue(ctx);
  std::optional<int64_t> edges =
      core_constraints_.NumEdges().GetUniqueValue(ctx);

  if (nodes && *nodes == 0 && edges && *edges == 0) return graph_type(0);
  if (!nodes || !edges) return std::nullopt;

  std::optional<typename MNodeLabel::value_type> node_label;
  if constexpr (HasNodeLabels<MNodeLabel>) {
    node_label = core_constraints_.NodeLabels().GetUniqueValue(ctx);
    if (!node_label)
      return std::nullopt;  // Remaining graphs have at least one node.
  }

  if (*edges == 0) {  // Several nodes, but no edges
    if constexpr (std::is_same_v<MNodeLabel, MNoNodeLabel>) {
      return graph_type(*nodes);
    } else {
      graph_type G(*nodes);
      G.SetNodeLabels(
          std::vector<typename MNodeLabel::value_type>(*nodes, *node_label));
      return G;
    }
  }

  if (*nodes == 1) {  // Single node with lots of loops
    if constexpr (!HasEdgeLabels<MEdgeLabel>) {
      graph_type G(*nodes);
      for (int64_t i = 0; i < *edges; ++i) G.AddEdge(0, 0);
      return G;
    } else {
      // I have edge labels, so they must be unique, too.
      auto edge_label = core_constraints_.EdgeLabels().GetUniqueValue(ctx);
      if (!edge_label) return std::nullopt;

      graph_type G(*nodes);
      G.SetNodeLabels({*node_label});
      for (int64_t i = 0; i < *edges; ++i) G.AddEdge(0, 0, *edge_label);
      return G;
    }
  }

  return std::nullopt;
}

template <typename MEdgeLabel, typename MNodeLabel>
MGraphFormat MGraph<MEdgeLabel, MNodeLabel>::Format() const {
  return format_;
}

template <typename MEdgeLabel, typename MNodeLabel>
MGraphFormat& MGraph<MEdgeLabel, MNodeLabel>::Format() {
  return format_;
}

template <typename MEdgeLabel, typename MNodeLabel>
typename MGraph<MEdgeLabel, MNodeLabel>::CoreConstraints
MGraph<MEdgeLabel, MNodeLabel>::GetCoreConstraints() const {
  return core_constraints_;
}

template <typename MEdgeLabel, typename MNodeLabel>
MGraph<MEdgeLabel, MNodeLabel>::Reader::Reader(librarian::ReaderContext ctx,
                                               int num_chunks,
                                               Ref<const MGraph> variable)
    : G_(0), variable_(variable) {
  const CoreConstraints& constraints = variable_.get().GetCoreConstraints();
  std::optional<int64_t> num_nodes = constraints.NumNodes().GetUniqueValue(ctx);
  if (!num_nodes)
    ctx.ThrowIOError("Cannot determine the number of nodes before read.");

  if (variable_.get().Format().IsAdjacencyMatrix()) {
    if constexpr (!HasEdgeLabels<MEdgeLabel>) {
      adjacency_matrix_.template emplace<IMtxIdx>(
          *num_nodes, std::vector<int64_t>(*num_nodes));
    } else {
      adjacency_matrix_.template emplace<LMtxIdx>(
          *num_nodes, std::vector<typename MEdgeLabel::value_type>(*num_nodes));
    }
  }

  if (variable_.get().Format().IsEdgeList()) {
    std::optional<int64_t> num_edges =
        constraints.NumEdges().GetUniqueValue(ctx);
    if (!num_edges)
      ctx.ThrowIOError(
          "Cannot determine the number of edges before reading edge list.");

    if (*num_edges != num_chunks) {
      ctx.ThrowIOError(
          std::format("MGraph::Reader expected to read {} chunks, but got {}.",
                      librarian::DebugString(*num_edges),
                      librarian::DebugString(num_chunks)));
    }
  }

  G_ = graph_type(*num_nodes);
}

template <typename MEdgeLabel, typename MNodeLabel>
auto MGraph<MEdgeLabel, MNodeLabel>::Reader::ReadNodeLabel(
    librarian::ReaderContext ctx) -> graph_type::NodeIdx {
  const MGraphFormat& format = variable_.get().Format();

  if (format.IsZeroBased()) {
    int64_t node_idx = ctx.ReadInteger();
    if (!(0 <= node_idx && node_idx < G_.NumNodes())) {
      ctx.ThrowIOError(std::format(
          "Invalid (0-based) node index {} for graph with {} nodes.", node_idx,
          G_.NumNodes()));
    }
    return node_idx;
  }

  if (format.IsOneBased()) {
    int64_t node_idx = ctx.ReadInteger() - 1;
    if (!(0 <= node_idx && node_idx < G_.NumNodes())) {
      ctx.ThrowIOError(std::format(
          "Invalid (1-based) node index {} for graph with {} nodes.",
          node_idx + 1, G_.NumNodes()));
    }
    return node_idx;
  }

  if (format.IsNodeLabelsStyle()) {
    if constexpr (!HasNodeLabels<MNodeLabel>) {
      ctx.ThrowIOError(
          "MGraph::Reader attempted to read a node label, but node labels "
          "are not defined for this graph.");
    }
    const std::optional<MNodeLabel>& node_label =
        variable_.get().GetCoreConstraints().NodeLabels();
    auto label = node_label ? node_label->Read(ctx) : MNodeLabel().Read(ctx);
    return G_.GetOrAddNodeIndex(label);
  }

  ctx.ThrowIOError(
      "MGraph::Reader does not support NonExhaustiveList node style.");
  return typename graph_type::NodeIdx();  // To silence compiler warning
}

template <typename MEdgeLabel, typename MNodeLabel>
auto MGraph<MEdgeLabel, MNodeLabel>::Reader::ReadEdgeLabel(
    librarian::ReaderContext ctx) -> MEdgeLabel::value_type {
  if constexpr (!HasEdgeLabels<MEdgeLabel>) {
    ctx.ThrowIOError(
        "MGraph::Reader attempted to read an edge label, but edge labels "
        "are not defined for this graph.");
  }
  const std::optional<MEdgeLabel>& edge_label =
      variable_.get().GetCoreConstraints().EdgeLabels();
  return edge_label ? edge_label->Read(ctx) : MEdgeLabel().Read(ctx);
}

template <typename MEdgeLabel, typename MNodeLabel>
void MGraph<MEdgeLabel, MNodeLabel>::Reader::ReadNextEdgeList(
    librarian::ReaderContext ctx) {
  int64_t u = ReadNodeLabel(ctx);
  ctx.ReadWhitespace(Whitespace::kSpace);
  int64_t v = ReadNodeLabel(ctx);

  if constexpr (!HasEdgeLabels<MEdgeLabel>) {
    G_.AddEdge(u, v);
  } else {
    ctx.ReadWhitespace(Whitespace::kSpace);
    G_.AddEdge(u, v, ReadEdgeLabel(ctx));
  }
}

template <typename MEdgeLabel, typename MNodeLabel>
void MGraph<MEdgeLabel, MNodeLabel>::Reader::ReadNextAdjacencyMatrix(
    librarian::ReaderContext ctx) {
  int64_t u = chunks_read_++;

  for (typename graph_type::NodeIdx v = 0; v < G_.NumNodes(); ++v) {
    if (v > 0) ctx.ReadWhitespace(Whitespace::kSpace);

    if constexpr (HasEdgeLabels<MEdgeLabel>) {
      auto edge_label = ReadEdgeLabel(ctx);

      if (u > v && std::get<LMtxIdx>(adjacency_matrix_)[v][u] != edge_label) {
        ctx.ThrowIOError(
            std::format("Asymmetric adjacency matrix entries at ({}, {}) = "
                        "{} and ({}, {}) = {}",
                        u, v, edge_label, v, u,
                        std::get<LMtxIdx>(adjacency_matrix_)[v][u]));
      }
      std::get<LMtxIdx>(adjacency_matrix_)[u][v] = edge_label;
      if (u <= v) G_.AddEdge(u, v, edge_label);
    } else {
      int64_t edge_count = ctx.ReadInteger();
      if (edge_count < 0) {
        ctx.ThrowIOError(std::format(
            "Invalid adjacency matrix entry {} at ({}, {})", edge_count, u, v));
      }
      if (u > v && std::get<IMtxIdx>(adjacency_matrix_)[v][u] != edge_count) {
        ctx.ThrowIOError(
            std::format("Asymmetric adjacency matrix entries at ({}, {}) = {} "
                        "and ({}, {}) = {}",
                        u, v, std::get<IMtxIdx>(adjacency_matrix_)[v][u], v, u,
                        edge_count));
      }
      std::get<IMtxIdx>(adjacency_matrix_)[u][v] = edge_count;
      if (u <= v)
        for (int64_t i = 0; i < edge_count; ++i) G_.AddEdge(u, v);
    }
  }
}

template <typename MEdgeLabel, typename MNodeLabel>
void MGraph<MEdgeLabel, MNodeLabel>::Reader::ReadNext(
    librarian::ReaderContext ctx) {
  const MGraphFormat& format = variable_.get().Format();

  if (format.IsEdgeList()) {
    ReadNextEdgeList(ctx);
    return;
  }

  // Read one row of adjacency matrix
  if (format.IsAdjacencyMatrix()) {
    ReadNextAdjacencyMatrix(ctx);
    return;
  }

  ctx.ThrowIOError(
      "MGraph::Reader does not support NonExhaustiveList edge style.");
}

template <typename MEdgeLabel, typename MNodeLabel>
typename MGraph<MEdgeLabel, MNodeLabel>::graph_type
MGraph<MEdgeLabel, MNodeLabel>::Reader::Finalize() && {
  return std::move(G_);
}

template <typename MEdgeLabel, typename MNodeLabel>
bool MGraph<MEdgeLabel, MNodeLabel>::CoreConstraints::NumNodesConstrained()
    const {
  return IsSet(Flags::kNumNodes);
}

template <typename MEdgeLabel, typename MNodeLabel>
const MInteger& MGraph<MEdgeLabel, MNodeLabel>::CoreConstraints::NumNodes()
    const {
  return data_->num_nodes;
}

template <typename MEdgeLabel, typename MNodeLabel>
bool MGraph<MEdgeLabel, MNodeLabel>::CoreConstraints::NumEdgesConstrained()
    const {
  return IsSet(Flags::kNumEdges);
}

template <typename MEdgeLabel, typename MNodeLabel>
const MInteger& MGraph<MEdgeLabel, MNodeLabel>::CoreConstraints::NumEdges()
    const {
  return data_->num_edges;
}

template <typename MEdgeLabel, typename MNodeLabel>
bool MGraph<MEdgeLabel, MNodeLabel>::CoreConstraints::EdgeLabelsConstrained()
    const {
  return IsSet(Flags::kEdgeLabels);
}

template <typename MEdgeLabel, typename MNodeLabel>
const MEdgeLabel& MGraph<MEdgeLabel, MNodeLabel>::CoreConstraints::EdgeLabels()
    const {
  return data_->edge_label_constraints;
}

template <typename MEdgeLabel, typename MNodeLabel>
bool MGraph<MEdgeLabel, MNodeLabel>::CoreConstraints::NodeLabelsConstrained()
    const {
  return IsSet(Flags::kNodeLabels);
}

template <typename MEdgeLabel, typename MNodeLabel>
const MNodeLabel& MGraph<MEdgeLabel, MNodeLabel>::CoreConstraints::NodeLabels()
    const {
  return data_->node_label_constraints;
}

template <typename MEdgeLabel, typename MNodeLabel>
bool MGraph<MEdgeLabel, MNodeLabel>::CoreConstraints::IsConnected() const {
  return IsSet(Flags::kIsConnected);
}

template <typename MEdgeLabel, typename MNodeLabel>
bool MGraph<MEdgeLabel, MNodeLabel>::CoreConstraints::MultiEdgesAllowed()
    const {
  return !IsSet(Flags::kMultiEdgesDisallowed);
}

template <typename MEdgeLabel, typename MNodeLabel>
bool MGraph<MEdgeLabel, MNodeLabel>::CoreConstraints::LoopsAllowed() const {
  return !IsSet(Flags::kLoopsDisallowed);
}

template <typename MEdgeLabel, typename MNodeLabel>
bool MGraph<MEdgeLabel, MNodeLabel>::CoreConstraints::IsSet(Flags flag) const {
  return (data_->touched & static_cast<std::underlying_type_t<Flags>>(flag)) !=
         0;
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_MGRAPH_H_
