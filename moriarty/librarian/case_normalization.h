// Copyright 2026 Darcy Best
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

#ifndef MORIARTY_LIBRARIAN_CASE_NORMALIZATION_H_
#define MORIARTY_LIBRARIAN_CASE_NORMALIZATION_H_

#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "moriarty/librarian/policies.h"

namespace moriarty {

template <typename EdgeLabel, typename NodeLabel>
class Graph;

namespace librarian {

// NormalizeCase()
//
// Converts an ASCII letter according to `normalization`. With kNone, returns
// the original character. Non-ASCII bytes and non-letters are unchanged.
[[nodiscard]] char NormalizeCase(char value, CaseNormalization normalization);

// NormalizeCase()
//
// Returns a copy of `value` with its ASCII letters normalized. Does not change
// the original string, whitespace, punctuation, or non-ASCII bytes.
[[nodiscard]] std::string NormalizeCase(std::string_view value,
                                        CaseNormalization normalization);

// AreEqual()
//
// Compares values of the same type. Normalizes char, string, and string_view
// values and recursively compares vectors, tuples, variants, and graphs.
// Other types use ordinary equality.
// Does not copy values, except for graph labels and edges (whose getters copy).
template <typename T>
[[nodiscard]] bool AreEqual(const T& lhs, const T& rhs,
                            CaseNormalization normalization);

// -----------------------------------------------------------------------------
// Internal utilities below

namespace case_normalization_internal {

[[nodiscard]] bool EqualText(std::string_view lhs, std::string_view rhs,
                             CaseNormalization normalization);

template <typename T>
  requires(std::equality_comparable<T>)
[[nodiscard]] bool EqualValue(const T& lhs, const T& rhs,
                              CaseNormalization normalization);

template <typename T>
[[nodiscard]] bool EqualValue(const std::vector<T>& lhs,
                              const std::vector<T>& rhs,
                              CaseNormalization normalization);

template <typename... T>
[[nodiscard]] bool EqualValue(const std::tuple<T...>& lhs,
                              const std::tuple<T...>& rhs,
                              CaseNormalization normalization);

template <typename... T>
[[nodiscard]] bool EqualValue(const std::variant<T...>& lhs,
                              const std::variant<T...>& rhs,
                              CaseNormalization normalization);

template <typename EdgeLabel, typename NodeLabel>
[[nodiscard]] bool EqualValue(const Graph<EdgeLabel, NodeLabel>& lhs,
                              const Graph<EdgeLabel, NodeLabel>& rhs,
                              CaseNormalization normalization);

// -----------------------------------------------------------------------------
// Template implementations

template <typename T>
  requires(std::equality_comparable<T>)
bool EqualValue(const T& lhs, const T& rhs, CaseNormalization) {
  return lhs == rhs;
}

template <typename T>
bool EqualValue(const std::vector<T>& lhs, const std::vector<T>& rhs,
                CaseNormalization normalization) {
  if (lhs.size() != rhs.size()) return false;
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    if (!AreEqual(lhs[i], rhs[i], normalization)) return false;
  }
  return true;
}

template <typename... T>
bool EqualValue(const std::tuple<T...>& lhs, const std::tuple<T...>& rhs,
                CaseNormalization normalization) {
  return [&]<std::size_t... I>(std::index_sequence<I...>) {
    return (AreEqual(std::get<I>(lhs), std::get<I>(rhs), normalization) && ...);
  }(std::index_sequence_for<T...>{});
}

template <typename... T>
bool EqualValue(const std::variant<T...>& lhs, const std::variant<T...>& rhs,
                CaseNormalization normalization) {
  if (lhs.index() != rhs.index()) return false;
  if (lhs.valueless_by_exception()) return true;
  return [&]<std::size_t... I>(std::index_sequence<I...>) {
    return ((lhs.index() == I &&
             AreEqual(std::get<I>(lhs), std::get<I>(rhs), normalization)) ||
            ...);
  }(std::index_sequence_for<T...>{});
}

template <typename EdgeLabel, typename NodeLabel>
bool EqualValue(const Graph<EdgeLabel, NodeLabel>& lhs,
                const Graph<EdgeLabel, NodeLabel>& rhs,
                CaseNormalization normalization) {
  if (normalization == CaseNormalization::kNone) return lhs == rhs;
  if (lhs.NumNodes() != rhs.NumNodes() || lhs.NumEdges() != rhs.NumEdges())
    return false;
  if (!AreEqual(lhs.GetNodeLabels(), rhs.GetNodeLabels(), normalization))
    return false;
  const auto left_edges = lhs.GetEdges();
  const auto right_edges = rhs.GetEdges();
  for (std::size_t i = 0; i < left_edges.size(); ++i) {
    const auto& left = left_edges[i];
    const auto& right = right_edges[i];
    if (left.u != right.u || left.v != right.v ||
        !AreEqual(left.e, right.e, normalization))
      return false;
  }
  return true;
}

}  // namespace case_normalization_internal

template <typename T>
bool AreEqual(const T& lhs, const T& rhs, CaseNormalization normalization) {
  if constexpr (std::same_as<T, char>) {
    return NormalizeCase(lhs, normalization) ==
           NormalizeCase(rhs, normalization);
  } else if constexpr (std::same_as<T, std::string> ||
                       std::same_as<T, std::string_view>) {
    return case_normalization_internal::EqualText(lhs, rhs, normalization);
  } else {
    return case_normalization_internal::EqualValue(lhs, rhs, normalization);
  }
}

}  // namespace librarian
}  // namespace moriarty

#endif  // MORIARTY_LIBRARIAN_CASE_NORMALIZATION_H_
