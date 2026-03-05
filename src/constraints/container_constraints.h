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

#ifndef MORIARTY_CONSTRAINTS_CONTAINER_CONSTRAINTS_H_
#define MORIARTY_CONSTRAINTS_CONTAINER_CONSTRAINTS_H_

#include <concepts>
#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "src/constraints/base_constraints.h"
#include "src/constraints/constraint_violation.h"
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
  ValidationResult Validate(ConstraintContext ctx,
                            const Container& value) const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const;

 private:
  MInteger length_;
};

// Constraints that all elements of a container must satisfy.
template <typename... MConstraints>
class Elements : public MConstraint {
 public:
  static_assert(sizeof...(MConstraints) > 0,
                "Elements must have at least one constraint.");

  // The elements of the container must satisfy all of these constraints.
  // E.g., Elements(Between(1, 10), Prime())
  template <typename... Constraints>
    requires(sizeof...(Constraints) > 0)
  explicit Elements(Constraints&&... element_constraints);

  // Returns the constraints on the elements.
  [[nodiscard]] std::tuple<MConstraints...> GetConstraints() const;

 private:
  std::tuple<MConstraints...> element_constraints_;
};

// CTAD for Elements<>
template <typename... Constraints>
Elements(Constraints&&...) -> Elements<std::decay_t<Constraints>...>;

// Constraints that all elements of a container must satisfy.
//
// This is very similar to `Elements`, except you must explicitly specify the
// MVariable that the constraints are for. Prefer to use `Elements` for
// readability.
template <typename MElementType>
class StronglyTypedElements : public MConstraint {
 public:
  template <typename... Constraints>
    requires(std::constructible_from<MElementType, Constraints...> &&
             sizeof...(Constraints) > 0)
  explicit StronglyTypedElements(Constraints&&... element_constraints);

  template <typename... Constraints>
    requires(sizeof...(Constraints) > 0)
  explicit StronglyTypedElements(Elements<Constraints...> element_constraints);

  // Returns the constraints on the elements.
  [[nodiscard]] MElementType GetConstraints() const;

  // Determines if the container's elements satisfy all constraints.
  ValidationResult Validate(
      ConstraintContext ctx,
      const std::vector<typename MElementType::value_type>& value) const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const;

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
  ValidationResult Validate(ConstraintContext ctx,
                            const MElementType::value_type& value) const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const;

 private:
  MElementType element_constraints_;
};

// Constraint stating that the elements of a container must be distinct.
class DistinctElements : public BasicMConstraint {
 public:
  explicit DistinctElements() : BasicMConstraint("has distinct elements") {}

  // Determines if the container's elements satisfy all constraints.
  template <typename T>
  ValidationResult Validate(const std::vector<T>& value) const;
};

// Constraint stating that the elements of a container must be sorted.
// Comp is the comparator used to compare elements, and Proj is the
// projection function applied to each element before comparison (similar to
// std::sort).
template <typename MElementType, typename Comp = std::ranges::less,
          typename Proj = std::identity>
class Sorted : public BasicMConstraint {
 public:
  explicit Sorted(Comp comp = {}, Proj proj = {})
      : BasicMConstraint("is sorted"), comp_(comp), proj_(proj) {}

  // Determines if the container is sorted.
  ValidationResult Validate(
      const std::vector<typename MElementType::value_type>& value) const;

  // Compares two elements using the provided comparator and projection.
  [[nodiscard]] bool Compare(const MElementType::value_type& lhs,
                             const MElementType::value_type& rhs) const;

  // Returns a function that can be used to compare elements using `comp` and
  // `proj`.
  [[nodiscard]] std::function<bool(const typename MElementType::value_type&,
                                   const typename MElementType::value_type&)>
  GetComparator() const;

 private:
  Comp comp_;
  Proj proj_;
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
ValidationResult Length::Validate(ConstraintContext ctx,
                                  const Container& value) const {
  auto v = length_.Validate(ctx.ForSubVariable("length"), value.size());
  if (v.IsOk()) return ValidationResult::Ok();
  return ValidationResult::Violation(ctx.GetLocalVariableName(), value,
                                     std::move(v));
}

// ====== Elements ======
template <typename... MConstraints>
template <typename... Constraints>
  requires(sizeof...(Constraints) > 0)
Elements<MConstraints...>::Elements(Constraints&&... element_constraints)
    : element_constraints_(std::forward<Constraints>(element_constraints)...) {}

template <typename... MConstraints>
std::tuple<MConstraints...> Elements<MConstraints...>::GetConstraints() const {
  return element_constraints_;
}

template <typename MElementType>
template <typename... Constraints>
  requires(sizeof...(Constraints) > 0)
StronglyTypedElements<MElementType>::StronglyTypedElements(
    Elements<Constraints...> element_constraints)
    : element_constraints_(std::make_from_tuple<MElementType>(
          element_constraints.GetConstraints())) {}

template <typename MElementType>
template <typename... Constraints>
  requires(std::constructible_from<MElementType, Constraints...> &&
           sizeof...(Constraints) > 0)
StronglyTypedElements<MElementType>::StronglyTypedElements(
    Constraints&&... element_constraints)
    : element_constraints_(std::forward<Constraints>(element_constraints)...) {
  static_assert(
      std::constructible_from<MElementType, Constraints...>,
      "Element constraints are not valid for the given MElementType.");
}

template <typename MElementType>
MElementType StronglyTypedElements<MElementType>::GetConstraints() const {
  return element_constraints_;
}

template <typename MElementType>
ValidationResult StronglyTypedElements<MElementType>::Validate(
    ConstraintContext ctx,
    const std::vector<typename MElementType::value_type>& value) const {
  auto index_name = [](int idx) { return std::format("index {}", idx); };
  for (int idx = -1; const auto& elem : value) {
    idx++;
    if (auto v = element_constraints_.Validate(
            ctx.ForIndexedSubVariable(index_name, idx), elem);
        !v.IsOk()) {
      return ValidationResult::Violation(ctx.GetLocalVariableName(), value,
                                         std::move(v));
    }
  }
  return ValidationResult::Ok();
}

template <typename MElementType>
std::string StronglyTypedElements<MElementType>::ToString() const {
  return std::format("each element {}", element_constraints_.ToString());
}

template <typename MElementType>
std::vector<std::string> StronglyTypedElements<MElementType>::GetDependencies()
    const {
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
ValidationResult Element<I, MElementType>::Validate(
    ConstraintContext ctx, const MElementType::value_type& value) const {
  return element_constraints_.Validate(ctx, value);
}

template <size_t I, typename MElementType>
std::string Element<I, MElementType>::ToString() const {
  return std::format("tuple index {} {}", I, element_constraints_.ToString());
}

template <size_t I, typename MElementType>
std::vector<std::string> Element<I, MElementType>::GetDependencies() const {
  return element_constraints_.GetDependencies();
}

// ====== DistinctElements ======
template <typename T>
ValidationResult DistinctElements::Validate(const std::vector<T>& value) const {
  absl::flat_hash_map<T, int> seen;
  for (int idx = -1; const auto& elem : value) {
    idx++;
    auto [it, inserted] = seen.insert({elem, idx});
    if (!inserted) {
      return ValidationResult::Violation(
          std::format("array indices {} and {}", it->second, idx), elem,
          librarian::Expected("distinct elements"));
    }
  }
  return ValidationResult::Ok();
}

// ====== Sorted ======
template <typename MElementType, typename Comp, typename Proj>
bool Sorted<MElementType, Comp, Proj>::Compare(
    const MElementType::value_type& lhs,
    const MElementType::value_type& rhs) const {
  return std::invoke(comp_, std::invoke(proj_, lhs), std::invoke(proj_, rhs));
}

template <typename MElementType, typename Comp, typename Proj>
std::function<bool(const typename MElementType::value_type&,
                   const typename MElementType::value_type&)>
Sorted<MElementType, Comp, Proj>::GetComparator() const {
  return [this](const typename MElementType::value_type& lhs,
                const typename MElementType::value_type& rhs) {
    return Compare(lhs, rhs);
  };
}

template <typename MElementType, typename Comp, typename Proj>
ValidationResult Sorted<MElementType, Comp, Proj>::Validate(
    const std::vector<typename MElementType::value_type>& value) const {
  for (size_t i = 1; i < value.size(); ++i) {
    if (Compare(value[i], value[i - 1]))
      return ValidationResult::Violation(
          std::format("indices {} and {}", i - 1, i),
          std::make_tuple(value[i - 1], value[i]),
          librarian::Expected("sorted"));
  }
  return ValidationResult::Ok();
}

}  // namespace moriarty

#endif  // MORIARTY_CONSTRAINTS_CONTAINER_CONSTRAINTS_H_
