/*
 * Copyright 2025 Darcy Best
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

#ifndef MORIARTY_SRC_VARIABLES_MARRAY_H_
#define MORIARTY_SRC_VARIABLES_MARRAY_H_

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_cat.h"
#include "src/contexts/librarian/analysis_context.h"
#include "src/contexts/librarian/printer_context.h"
#include "src/contexts/librarian/reader_context.h"
#include "src/contexts/librarian/resolver_context.h"
#include "src/librarian/io_config.h"
#include "src/librarian/mvariable.h"
#include "src/util/locked_optional.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/constraints/container_constraints.h"
#include "src/variables/constraints/io_constraints.h"
#include "src/variables/constraints/size_constraints.h"
#include "src/variables/minteger.h"

namespace moriarty {

// MArray<>
//
// Describes constraints placed on an array. The elements of the array must have
// a corresponding MVariable, and general constraints on the elements are
// controlled via the `Elements<>` constraint.
//
// In order to generate, the length of the array must be constrained (via the
// `Length` constraint).
template <typename MElementType>
class MArray : public librarian::MVariable<
                   MArray<MElementType>,
                   std::vector<typename MElementType::value_type>> {
 public:
  using element_value_type = typename MElementType::value_type;
  using vector_value_type = typename std::vector<element_value_type>;

  // Create an MArray from a set of constraints. Logically equivalent to
  // calling AddConstraint() for each constraint.
  //
  // E.g., MArray<MInteger>(Elements<MInteger>(Between(1, 10)), Length(15))
  template <typename... Constraints>
    requires(std::derived_from<std::decay_t<Constraints>, MConstraint> && ...)
  explicit MArray(Constraints&&... constraints);

  // Create an MArray with this set of constraints on each element.
  explicit MArray(MElementType element_constraints);

  ~MArray() override = default;

  // ---------------------------------------------------------------------------
  //  Constrain the value to a specific set of values

  // The array must have exactly this value.
  MArray& AddConstraint(Exactly<vector_value_type> constraint);
  // The array must be one of these values.
  MArray& AddConstraint(OneOf<vector_value_type> constraint);

  // ---------------------------------------------------------------------------
  //  Constrain the length of the array

  // The array must have this length.
  MArray& AddConstraint(Length constraint);
  // The array should be approximately this size.
  MArray& AddConstraint(SizeCategory constraint);

  // ---------------------------------------------------------------------------
  //  Constrain the elements of the array

  // The array's elements must satisfy these constraints.
  MArray& AddConstraint(Elements<MElementType> constraint);
  // The array's elements must be distinct.
  MArray& AddConstraint(DistinctElements constraint);

  // ---------------------------------------------------------------------------
  //  Constrain the array's I/O

  // The array's elements are separated by this whitespace (default = space)
  MArray& AddConstraint(IOSeparator constraint);

  // Typename()
  //
  // Returns a string representing the name of this type (for example,
  // "MArray<MInteger>"). This is mostly used for debugging/error messages.
  [[nodiscard]] std::string Typename() const override;

 private:
  MElementType element_constraints_;
  std::optional<MInteger> length_;
  bool distinct_elements_ = false;
  librarian::LockedOptional<Whitespace> separator_ =
      librarian::LockedOptional<Whitespace>{Whitespace::kSpace};

  // GenerateNDistinctImpl()
  //
  // Same as GenerateImpl(), but guarantees that all elements are distinct.
  vector_value_type GenerateNDistinctImpl(librarian::ResolverContext ctx,
                                          int n) const;

  // GenerateUnseenElement()
  //
  // Returns a element that is not in `seen`. `remaining_retries` is the maximum
  // number of times `Generate()` may be called. This function updates
  // `remaining_retries`.
  element_value_type GenerateUnseenElement(
      librarian::ResolverContext ctx,
      const absl::flat_hash_set<element_value_type>& seen,
      int& remaining_retries, int index) const;

  // GetNumberOfRetriesForDistinctElements()
  //
  // Returns the number of total calls to Generate() that should be done in
  // order to confidently generate `n` distinct elements. Exact probabilities
  // may change over time, but is aimed at <1% failure rate at the moment.
  static int GetNumberOfRetriesForDistinctElements(int n);

  // ---------------------------------------------------------------------------
  //  MVariable overrides
  vector_value_type GenerateImpl(librarian::ResolverContext ctx) const override;
  vector_value_type ReadImpl(librarian::ReaderContext ctx) const override;
  void PrintImpl(librarian::PrinterContext ctx,
                 const vector_value_type& value) const override;
  std::vector<MArray<MElementType>> ListEdgeCasesImpl(
      librarian::AnalysisContext ctx) const override;
  // ---------------------------------------------------------------------------
};

// Class template argument deduction (CTAD). Allows for `MArray(MInteger())`
// instead of `MArray<MInteger>(MInteger())`. See `NestedMArray()` for nesting
// multiple `MArrays` inside one another.
template <typename MoriartyElementType>
MArray(MoriartyElementType) -> MArray<MoriartyElementType>;

// NestedMArray()
//
// Returns an array to `elements`.
//
// CTAD does not allow for nested `MArray`s. `MArray(MArray(MInteger())) is
// interpreted as `MArray<MInteger>(MInteger())`, since the outer `MArray` is
// calling the copy constructor.
//
// Examples:
//   `NestedMArray(MArray(MInteger()))`
//   `NestedMArray(NestedMArray(MArray(MInteger())))`
//   `NestedMArray(NestedMArray(MInteger()))`
template <typename MElementType, typename... Constraints>
  requires std::constructible_from<MArray<MElementType>, Constraints...>
MArray<MArray<MElementType>> NestedMArray(MArray<MElementType> elements,
                                          Constraints&&... constraints) {
  MArray<MArray<MElementType>> res(std::move(elements));
  (res.AddConstraint(std::forward<Constraints>(constraints)), ...);
  return res;
}

// -----------------------------------------------------------------------------
//  Template Implementation Below

template <typename T>
MArray<T>::MArray(T element_constraints)
    : MArray<T>(Elements<T>(std::move(element_constraints))) {}

template <typename T>
template <typename... Constraints>
  requires(std::derived_from<std::decay_t<Constraints>, MConstraint> && ...)
MArray<T>::MArray(Constraints&&... constraints) {
  static_assert(
      std::derived_from<T, librarian::MVariable<T, typename T::value_type>>,
      "The T used in MArray<T> must be a Moriarty variable. For example, "
      "MArray<MInteger> or MArray<MCustomType>.");
  (AddConstraint(std::forward<Constraints>(constraints)), ...);
}

template <typename T>
MArray<T>& MArray<T>::AddConstraint(Exactly<vector_value_type> constraint) {
  return this->InternalAddExactlyConstraint(std::move(constraint));
}

template <typename T>
MArray<T>& MArray<T>::AddConstraint(OneOf<vector_value_type> constraint) {
  return this->InternalAddOneOfConstraint(std::move(constraint));
}

template <typename T>
MArray<T>& MArray<T>::AddConstraint(Elements<T> constraint) {
  element_constraints_.MergeFrom(constraint.GetConstraints());
  return this->InternalAddConstraint(std::move(constraint));
}

template <typename T>
MArray<T>& MArray<T>::AddConstraint(Length constraint) {
  if (!length_) length_ = MInteger();
  length_->MergeFrom(constraint.GetConstraints());
  return this->InternalAddConstraint(std::move(constraint));
}

template <typename T>
MArray<T>& MArray<T>::AddConstraint(DistinctElements constraint) {
  distinct_elements_ = true;
  return this->InternalAddConstraint(std::move(constraint));
}

template <typename T>
MArray<T>& MArray<T>::AddConstraint(IOSeparator constraint) {
  if (!separator_.Set(constraint.GetSeparator())) {
    throw std::runtime_error(
        "Attempting to set multiple I/O separators for the same MArray.");
  }
  return this->InternalAddConstraint(std::move(constraint));
}

template <typename T>
MArray<T>& MArray<T>::AddConstraint(SizeCategory constraint) {
  return AddConstraint(Length(constraint));
}

template <typename T>
std::string MArray<T>::Typename() const {
  return absl::StrCat("MArray<", element_constraints_.Typename(), ">");
}

template <typename MElementType>
auto MArray<MElementType>::GenerateImpl(librarian::ResolverContext ctx) const
    -> vector_value_type {
  if (this->GetOneOf().HasBeenConstrained())
    return this->GetOneOf().SelectOneOf(
        [&](int n) { return ctx.RandomInteger(n); });

  if (!length_) {
    throw std::runtime_error(
        "Attempting to generate an array with no length parameter given.");
  }

  MInteger length_local = *length_;

  // Ensure that the size is non-negative.
  length_local.AddConstraint(AtLeast(0));

  int length = length_local.Generate(ctx.ForSubVariable("length"));

  if (distinct_elements_) return GenerateNDistinctImpl(ctx, length);

  vector_value_type res;
  res.reserve(length);

  for (int i = 0; i < length; i++) {
    res.push_back(element_constraints_.Generate(
        ctx.ForSubVariable("elem[" + std::to_string(i) + "]")));
  }

  return res;
}

template <typename MElementType>
auto MArray<MElementType>::GenerateNDistinctImpl(librarian::ResolverContext ctx,
                                                 int n) const
    -> vector_value_type {
  vector_value_type res;
  res.reserve(n);

  absl::flat_hash_set<element_value_type> values_seen;
  int remaining_retries = GetNumberOfRetriesForDistinctElements(n);

  for (int i = 0; i < n && remaining_retries > 0; i++) {
    element_value_type value =
        GenerateUnseenElement(ctx, values_seen, remaining_retries, i);
    values_seen.insert(value);
    res.push_back(std::move(value));
  }

  return res;
}

template <typename MElementType>
auto MArray<MElementType>::GenerateUnseenElement(
    librarian::ResolverContext ctx,
    const absl::flat_hash_set<element_value_type>& seen, int& remaining_retries,
    int index) const -> element_value_type {
  for (; remaining_retries > 0; remaining_retries--) {
    element_value_type value =
        element_constraints_.Generate(ctx.ForSubVariable("elem"));
    if (!seen.contains(value)) return value;
  }

  throw std::runtime_error("Cannot generate enough distinct values for array.");
}

template <typename MElementType>
int MArray<MElementType>::GetNumberOfRetriesForDistinctElements(int n) {
  // The worst case is randomly generating n numbers between 1 and n.
  //
  //   T   := number of iterations to get all n values.
  //   H_n := Harmonic number (1/1 + 1/2 + ... + 1/n).
  //
  //      Prob(|T - n * H_n| > c * n) < pi^2 / (6 * c^2)
  //
  // Thus, if c = 14:
  //
  //      Prob(T > n * H_n + 14 * n) < 1%.
  double H_n = 0;
  for (int i = n; i >= 1; i--) H_n += 1.0 / i;

  return n * H_n + 14 * n;
}

template <typename MoriartyElementType>
void MArray<MoriartyElementType>::PrintImpl(
    librarian::PrinterContext ctx, const vector_value_type& value) const {
  for (int i = 0; i < value.size(); i++) {
    if (i > 0) ctx.PrintWhitespace(separator_.Get());
    element_constraints_.Print(ctx, value[i]);
  }
}

template <typename MoriartyElementType>
MArray<MoriartyElementType>::vector_value_type
MArray<MoriartyElementType>::ReadImpl(librarian::ReaderContext ctx) const {
  if (!length_)
    throw std::runtime_error("Unknown length of array before read.");

  std::optional<int64_t> length = length_->GetUniqueValue(ctx);

  if (!length) {
    throw std::runtime_error(
        "Cannot determine the length of array before read.");
  }

  vector_value_type res;
  res.reserve(*length);
  for (int i = 0; i < *length; i++) {
    if (i > 0) ctx.ReadWhitespace(separator_.Get());
    res.push_back(element_constraints_.Read(ctx));
  }
  return res;
}

template <typename MoriartyElementType>
std::vector<MArray<MoriartyElementType>>
MArray<MoriartyElementType>::ListEdgeCasesImpl(
    librarian::AnalysisContext ctx) const {
  if (!length_) {
    throw std::runtime_error(
        "Attempting to get difficult instances of an Array with no "
        "length parameter given.");
  }
  std::vector<MArray<MoriartyElementType>> cases;
  std::vector<MInteger> length_cases = length_->ListEdgeCases(ctx);

  cases.reserve(length_cases.size());
  for (const auto& c : length_cases) {
    cases.push_back(MArray(Length(c)));
  }
  // TODO(hivini): Add cases for sort and the difficult instances of the
  // variable it holds.
  return cases;
};

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_MARRAY_H_
