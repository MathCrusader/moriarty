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

namespace moriarty {

MGraphFormat& MGraphFormat::EdgeList() { return SetStyle(Style::kEdgeList); }

bool MGraphFormat::IsEdgeList() const { return style_ == Style::kEdgeList; }

MGraphFormat& MGraphFormat::AdjacencyMatrix() {
  return SetStyle(Style::kAdjacencyMatrix);
}

bool MGraphFormat::IsAdjacencyMatrix() const {
  return style_ == Style::kAdjacencyMatrix;
}

MGraphFormat& MGraphFormat::ZeroBased() {
  return SetNodeStyle(NodeStyle::k0Based);
}

bool MGraphFormat::IsZeroBased() const {
  return node_style_ == NodeStyle::k0Based;
}

MGraphFormat& MGraphFormat::OneBased() {
  return SetNodeStyle(NodeStyle::k1Based);
}

bool MGraphFormat::IsOneBased() const {
  return node_style_ == NodeStyle::k1Based;
}

MGraphFormat& MGraphFormat::NodeLabelsStyle() {
  return SetNodeStyle(NodeStyle::kNodeLabels);
}

bool MGraphFormat::IsNodeLabelsStyle() const {
  return node_style_ == NodeStyle::kNodeLabels;
}

MGraphFormat& MGraphFormat::SetStyle(Style style) {
  style_ = style;
  return *this;
}

MGraphFormat& MGraphFormat::SetNodeStyle(NodeStyle node_style) {
  node_style_ = node_style;
  return *this;
}

}  // namespace moriarty
