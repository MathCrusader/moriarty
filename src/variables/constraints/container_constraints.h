/*
 * Copyright 2024 Google LLC
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

#ifndef MORIARTY_SRC_VARIABLES_CONSTRAINTS_CONTAINERS_H_
#define MORIARTY_SRC_VARIABLES_CONSTRAINTS_CONTAINERS_H_

#include <algorithm>
#include <concepts>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "src/contexts/librarian_context.h"
#include "src/util/debug_string.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/minteger.h"

namespace moriarty {

// Constraint stating that the container must have this length.
class Length : public MConstraint {
 public:
  // The length must be exactly this value.
  // E.g., Length(10)
  template <typename Integer>
    requires std::integral<Integer>
  explicit Length(Integer value);

  // The length must be exactly this integer expression.
  // E.g., Length("3 * N + 1").
  explicit Length(std::string_view expression);

  // The length must satisfy all of these constraints.
  // E.g., Length(Between(1, 10), Prime())
  template <typename... Constraints>
    requires(std::constructible_from<MInteger, Constraints...> &&
             sizeof...(Constraints) > 0)
  explicit Length(Constraints&&... constraints);

  // Returns the constraints on the length.
  [[nodiscard]] MInteger GetConstraints() const;

  // Determines if the container has the correct length.
  template <typename Container>
  [[nodiscard]] bool IsSatisfiedWith(librarian::AnalysisContext ctx,
                                     const Container& value) const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns a string explaining why the value does not satisfy the constraints.
  // It is assumed that IsSatisfiedWith() returned false.
  template <typename Container>
  [[nodiscard]] std::string UnsatisfiedReason(librarian::AnalysisContext ctx,
                                              const Container& value) const;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const;

 private:
  MInteger length_;
};

// Constraints that all elements of a container must satisfy.
template <typename MElementType>
class Elements : public MConstraint {
 public:
  // The elements of the container must satisfy all of these constraints.
  // E.g., Elements<MInteger>(Between(1, 10), Prime())
  template <typename... Constraints>
    requires(std::constructible_from<MElementType, Constraints...> &&
             sizeof...(Constraints) > 0)
  explicit Elements(Constraints&&... element_constraints);

  // Returns the constraints on the elements.
  [[nodiscard]] MElementType GetConstraints() const;

  // Determines if the container's elements satisfy all constraints.
  [[nodiscard]] bool IsSatisfiedWith(
      librarian::AnalysisContext ctx,
      const std::vector<typename MElementType::value_type>& value) const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns a string explaining why the value does not satisfy the constraints.
  // It is assumed that IsSatisfiedWith() returned false.
  [[nodiscard]] std::string UnsatisfiedReason(
      librarian::AnalysisContext ctx,
      const std::vector<typename MElementType::value_type>& value) const;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const;

 private:
  MElementType element_constraints_;
};

// Constraints that the I-th element of a container (probably a tuple) must
// satisfy.
template <size_t I, typename MElementType>
class Element : public MConstraint {
 public:
  // The I-th element of the container must satisfy all of these constraints.
  // E.g., Element<2, MInteger>(Between(1, 10), Prime())
  template <typename... Constraints>
    requires(std::constructible_from<MElementType, Constraints...> &&
             sizeof...(Constraints) > 0)
  explicit Element(Constraints&&... element_constraints);

  // Returns the constraints on the elements.
  [[nodiscard]] MElementType GetConstraints() const;

  // Determines if an object satisfy all constraints.
  [[nodiscard]] bool IsSatisfiedWith(
      librarian::AnalysisContext ctx,
      const MElementType::value_type& value) const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns a string explaining why the value does not satisfy the constraints.
  // It is assumed that IsSatisfiedWith() returned false.
  [[nodiscard]] std::string UnsatisfiedReason(
      librarian::AnalysisContext ctx,
      const MElementType::value_type& value) const;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const;

 private:
  MElementType element_constraints_;
};

// Constraint stating that the elements of a container must be distinct.
class DistinctElements : public MConstraint {
 public:
  // The elements of the container must all be distinct.
  explicit DistinctElements() = default;

  // Determines if the container's elements satisfy all constraints.
  template <typename T>
  [[nodiscard]] bool IsSatisfiedWith(librarian::AnalysisContext ctx,
                                     const std::vector<T>& value) const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns a string explaining why the value does not satisfy the constraints.
  // It is assumed that IsSatisfiedWith() returned false.
  template <typename T>
  [[nodiscard]] std::string UnsatisfiedReason(
      librarian::AnalysisContext ctx, const std::vector<T>& value) const;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const;
};

// -----------------------------------------------------------------------------
//  Template Implementation Below

// ====== Length ======
template <typename Integer>
  requires std::integral<Integer>
Length::Length(Integer value) : length_(Exactly(value)) {}

template <typename... Constraints>
  requires(std::constructible_from<MInteger, Constraints...> &&
           sizeof...(Constraints) > 0)
Length::Length(Constraints&&... constraints)
    : length_(std::forward<Constraints>(constraints)...) {}

template <typename Container>
bool Length::IsSatisfiedWith(librarian::AnalysisContext ctx,
                             const Container& value) const {
  return length_.IsSatisfiedWith(ctx, static_cast<int64_t>(value.size()));
}

template <typename Container>
std::string Length::UnsatisfiedReason(librarian::AnalysisContext ctx,
                                      const Container& value) const {
  return std::format(
      "has length (which is {}) that {}", librarian::DebugString(value.size()),
      length_.UnsatisfiedReason(ctx, static_cast<int64_t>(value.size())));
}

// ====== Elements ======
template <typename MElementType>
template <typename... Constraints>
  requires(std::constructible_from<MElementType, Constraints...> &&
           sizeof...(Constraints) > 0)
Elements<MElementType>::Elements(Constraints&&... element_constraints)
    : element_constraints_(std::forward<Constraints>(element_constraints)...) {}

template <typename MElementType>
MElementType Elements<MElementType>::GetConstraints() const {
  return element_constraints_;
}

template <typename MElementType>
bool Elements<MElementType>::IsSatisfiedWith(
    librarian::AnalysisContext ctx,
    const std::vector<typename MElementType::value_type>& value) const {
  for (const auto& elem : value) {
    if (!element_constraints_.IsSatisfiedWith(ctx, elem)) return false;
  }
  return true;
}

template <typename MElementType>
std::string Elements<MElementType>::ToString() const {
  return std::format("each element {}", element_constraints_.ToString());
}

template <typename MElementType>
std::string Elements<MElementType>::UnsatisfiedReason(
    librarian::AnalysisContext ctx,
    const std::vector<typename MElementType::value_type>& value) const {
  auto it = std::ranges::find_if_not(value, [&](const auto& elem) {
    return element_constraints_.IsSatisfiedWith(ctx, elem);
  });
  if (it == value.end()) {
    throw std::invalid_argument(
        "Elements<>()::UnsatisfiedReason called when all elements ok.");
  }

  return std::format("array index {} (which is {}) {}", it - value.begin(),
                     librarian::DebugString(*it),
                     element_constraints_.UnsatisfiedReason(ctx, *it));
}

template <typename MElementType>
std::vector<std::string> Elements<MElementType>::GetDependencies() const {
  return element_constraints_.GetDependencies();
}

// ====== Element<I> ======
template <size_t I, typename MElementType>
template <typename... Constraints>
  requires(std::constructible_from<MElementType, Constraints...> &&
           sizeof...(Constraints) > 0)
Element<I, MElementType>::Element(Constraints&&... element_constraints)
    : element_constraints_(std::forward<Constraints>(element_constraints)...) {}

template <size_t I, typename MElementType>
MElementType Element<I, MElementType>::GetConstraints() const {
  return element_constraints_;
}

template <size_t I, typename MElementType>
bool Element<I, MElementType>::IsSatisfiedWith(
    librarian::AnalysisContext ctx,
    const MElementType::value_type& value) const {
  return element_constraints_.IsSatisfiedWith(ctx, value);
}

template <size_t I, typename MElementType>
std::string Element<I, MElementType>::ToString() const {
  return std::format("tuple index {} {}", I, element_constraints_.ToString());
}

template <size_t I, typename MElementType>
std::string Element<I, MElementType>::UnsatisfiedReason(
    librarian::AnalysisContext ctx,
    const MElementType::value_type& value) const {
  return std::format("tuple index {} (which is {}) {}", I,
                     librarian::DebugString(value),
                     element_constraints_.UnsatisfiedReason(ctx, value));
}

template <size_t I, typename MElementType>
std::vector<std::string> Element<I, MElementType>::GetDependencies() const {
  return element_constraints_.GetDependencies();
}

// ====== DistinctElements ======
template <typename T>
bool DistinctElements::IsSatisfiedWith(librarian::AnalysisContext ctx,
                                       const std::vector<T>& value) const {
  std::unordered_set<T> seen(value.begin(), value.end());
  return seen.size() == value.size();
}

template <typename T>
std::string DistinctElements::UnsatisfiedReason(
    librarian::AnalysisContext ctx, const std::vector<T>& value) const {
  std::unordered_map<T, int> seen;
  for (int idx = -1; const auto& elem : value) {
    idx++;
    auto [it, inserted] = seen.insert({elem, idx});
    if (!inserted) {
      return std::format(
          "array indices {} and {} (which are {}) are not distinct", it->second,
          idx, librarian::DebugString(elem));
    }
  }
  throw std::invalid_argument(
      "DistinctElements::UnsatisfiedReason called with all elements distinct.");
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_CONSTRAINTS_CONTAINERS_H_
