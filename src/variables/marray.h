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

#ifndef MORIARTY_VARIABLES_MARRAY_H_
#define MORIARTY_VARIABLES_MARRAY_H_

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <format>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "src/constraints/base_constraints.h"
#include "src/constraints/container_constraints.h"
#include "src/constraints/size_constraints.h"
#include "src/contexts/librarian_context.h"
#include "src/internal/abstract_variable.h"
#include "src/librarian/errors.h"
#include "src/librarian/io_config.h"
#include "src/librarian/mvariable.h"
#include "src/librarian/policies.h"
#include "src/librarian/util/ref.h"
#include "src/variables/minteger.h"

namespace moriarty {

template <typename MElementType>
class MArray;
namespace librarian {
template <typename MElementType>
struct MVariableValueTypeTrait<MArray<MElementType>> {
  using type = std::vector<typename MElementType::value_type>;
};
}  // namespace librarian

// MArrayFormat
//
// How to format an MArray when reading/writing.
class MArrayFormat {
 public:
  // Sets the whitespace to be used between elements of the array.
  //
  // Default: kSpace
  MArrayFormat& WithSeparator(Whitespace separator);
  // Returns if the format is an edge list. See `EdgeList()`.
  Whitespace GetSeparator() const;

  // Sets the array to be space-separated.
  MArrayFormat& SpaceSeparated();
  // Sets the array to be newline-separated.
  MArrayFormat& NewlineSeparated();

  // Take any non-defaults in `other` and apply them to this format.
  void Merge(const MArrayFormat& other);

 private:
  Whitespace separator_ = Whitespace::kSpace;
};

// MArray<>
//
// Describes constraints placed on an array. The elements of the array must have
// a corresponding MVariable, and general constraints on the elements are
// controlled via the `Elements<>` constraint.
//
// In order to generate, the length of the array must be constrained (via the
// `Length` constraint).
template <typename MElementType>
class MArray : public librarian::MVariable<MArray<MElementType>> {
 public:
  using element_value_type = typename MElementType::value_type;
  using vector_value_type = typename std::vector<element_value_type>;
  class Reader;  // Forward declaration
  using chunked_reader_type = Reader;
  using comparator_type =
      std::function<bool(const element_value_type&, const element_value_type&)>;

  // Create an MArray from a set of constraints. Logically equivalent to
  // calling AddConstraint() for each constraint.
  //
  // E.g., MArray<MInteger>(Elements<MInteger>(Between(1, 10)), Length(15))
  template <typename... Constraints>
    requires(ConstraintFor<MArray, Constraints> && ...)
  explicit MArray(Constraints&&... constraints);

  // Create an MArray with this set of constraints on each element.
  explicit MArray(MElementType element_constraints);

  ~MArray() override = default;

  using librarian::MVariable<MArray>::AddConstraint;  // Custom constraints

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
  // The array's elements must be in sorted order.
  template <typename Comp, typename Proj>
  MArray& AddConstraint(Sorted<MElementType, Comp, Proj> constraint);

  // ---------------------------------------------------------------------------
  //  Constrain the array's I/O

  // Change the I/O format of the array.
  MArray& AddConstraint(MArrayFormat constraint);
  // Returns the I/O format for this array.
  [[nodiscard]] MArrayFormat& Format();
  // Returns the I/O format for this array.
  [[nodiscard]] MArrayFormat Format() const;

  // Typename()
  //
  // Returns a string representing the name of this type (for example,
  // "MArray<MInteger>"). This is mostly used for debugging/error messages.
  [[nodiscard]] std::string Typename() const override;

  // MArray::Reader
  //
  // The partial reader reads one element at a time from the input stream,
  // without reading the separators. Call ReadNext() exactly N times, then call
  // Finalize() to get the final value, which will leave this in a
  // moved-from-state.
  class Reader {
   public:
    explicit Reader(librarian::ReadVariableContext ctx, int num_chunks,
                    Ref<const MArray> variable)
        : variable_(variable) {
      // TODO: Add safety checks if N is massive?
      // TODO: Ensure `length_` is satisfied?
      array_.reserve(num_chunks);
    }

    void ReadNext(librarian::ReadVariableContext ctx) {
      array_.push_back(
          variable_.get().GetCoreConstraints().Elements().Read(ctx));
    }
    vector_value_type Finalize() && { return std::move(array_); }

   private:
    std::vector<element_value_type> array_;
    Ref<const MArray> variable_;
  };

  // MArray::CoreConstraints
  //
  // A base set of constraints for `MArray` that are used during generation.
  // Note: Returned references are invalidated after any non-const call to this
  // class or the corresponding `MArray`.
  class CoreConstraints {
   public:
    bool ElementsConstrained() const;
    const MElementType& Elements() const;

    bool LengthConstrained() const;
    const MInteger& Length() const;

    bool ComparatorConstrained() const;
    const comparator_type& Comparator() const;

    bool DistinctElements() const;

   private:
    friend class MArray;

    enum Flags : uint32_t {
      kElements = 1 << 0,
      kLength = 1 << 1,
      kComparator = 1 << 2,

      kDistinctElements = 1 << 4,  // Default: false
    };

    struct Data {
      std::underlying_type_t<Flags> touched = 0;

      MElementType elements;
      MInteger length;
      comparator_type comparator;
    };
    librarian::CowPtr<Data> data_;
    bool IsSet(Flags flag) const;
  };
  [[nodiscard]] CoreConstraints GetCoreConstraints() const {
    return core_constraints_;
  }

 private:
  CoreConstraints core_constraints_;
  MArrayFormat format_;

  // GenerateNDistinctImpl()
  //
  // Same as GenerateImpl(), but guarantees that all elements are distinct.
  vector_value_type GenerateNDistinctImpl(
      librarian::GenerateVariableContext ctx, int n) const;

  // GenerateUnseenElement()
  //
  // Returns a element that is not in `seen`. `remaining_retries` is the maximum
  // number of times `Generate()` may be called. This function updates
  // `remaining_retries`.
  element_value_type GenerateUnseenElement(
      librarian::GenerateVariableContext ctx,
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
  vector_value_type GenerateImpl(
      librarian::GenerateVariableContext ctx) const override;
  vector_value_type ReadImpl(librarian::ReadVariableContext ctx) const override;
  void WriteImpl(librarian::WriteVariableContext ctx,
                 const vector_value_type& value) const override;
  std::vector<MArray<MElementType>> ListEdgeCasesImpl(
      librarian::AnalyzeVariableContext ctx) const override;
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
  requires(ConstraintFor<MArray<T>, Constraints> && ...)
MArray<T>::MArray(Constraints&&... constraints) {
  static_assert(
      MoriartyVariable<T>,
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
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kElements;
  constraints.elements.MergeFrom(constraint.GetConstraints());
  return this->InternalAddConstraint(std::move(constraint));
}

template <typename T>
MArray<T>& MArray<T>::AddConstraint(Length constraint) {
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kLength;
  constraints.length.MergeFrom(constraint.GetConstraints());
  return this->InternalAddConstraint(std::move(constraint));
}

template <typename T>
MArray<T>& MArray<T>::AddConstraint(DistinctElements constraint) {
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kDistinctElements;
  return this->InternalAddConstraint(std::move(constraint));
}

template <typename T>
template <typename Comp, typename Proj>
MArray<T>& MArray<T>::AddConstraint(Sorted<T, Comp, Proj> constraint) {
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kComparator;
  constraints.comparator = constraint.GetComparator();
  return this->InternalAddConstraint(std::move(constraint));
}

template <typename T>
MArray<T>& MArray<T>::AddConstraint(MArrayFormat constraint) {
  Format().Merge(std::move(constraint));
  return *this;
}

template <typename T>
MArrayFormat& MArray<T>::Format() {
  return format_;
}

template <typename T>
MArrayFormat MArray<T>::Format() const {
  return format_;
}

template <typename T>
MArray<T>& MArray<T>::AddConstraint(SizeCategory constraint) {
  return AddConstraint(Length(constraint));
}

template <typename T>
std::string MArray<T>::Typename() const {
  return std::format("MArray<{}>", core_constraints_.Elements().Typename());
}

template <typename MElementType>
auto MArray<MElementType>::GenerateImpl(
    librarian::GenerateVariableContext ctx) const -> vector_value_type {
  if (this->GetOneOf().HasBeenConstrained())
    return this->GetOneOf().SelectOneOf(
        [&](int n) { return ctx.RandomInteger(n); });

  if (!core_constraints_.LengthConstrained()) {
    throw GenerationError(ctx.GetVariableName(),
                          "Attempting to generate an array with no "
                          "length parameter given.",
                          RetryPolicy::kAbort);
  }

  MInteger length_local = core_constraints_.Length();

  // Ensure that the size is non-negative.
  length_local.AddConstraint(AtLeast(0));

  int length = length_local.Generate(ctx.ForSubVariable("length"));

  vector_value_type res;
  if (core_constraints_.DistinctElements()) {
    res = GenerateNDistinctImpl(ctx, length);
  } else {
    res.reserve(length);
    for (int i = 0; i < length; i++) {
      res.push_back(core_constraints_.Elements().Generate(
          ctx.ForSubVariable("elem[" + std::to_string(i) + "]")));
    }
  }

  if (core_constraints_.ComparatorConstrained())
    std::sort(res.begin(), res.end(), core_constraints_.Comparator());

  return res;
}

template <typename MElementType>
auto MArray<MElementType>::GenerateNDistinctImpl(
    librarian::GenerateVariableContext ctx, int n) const -> vector_value_type {
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
    librarian::GenerateVariableContext ctx,
    const absl::flat_hash_set<element_value_type>& seen, int& remaining_retries,
    int index) const -> element_value_type {
  for (; remaining_retries > 0; remaining_retries--) {
    element_value_type value =
        core_constraints_.Elements().Generate(ctx.ForSubVariable("elem"));
    if (!seen.contains(value)) return value;
  }

  // TODO: We may want to not retry if we can prove it will always fail.
  throw GenerationError(ctx.GetVariableName(),
                        "Cannot generate enough distinct values for array.",
                        RetryPolicy::kRetry);
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
void MArray<MoriartyElementType>::WriteImpl(
    librarian::WriteVariableContext ctx, const vector_value_type& value) const {
  for (int i = 0; i < value.size(); i++) {
    if (i > 0) ctx.WriteWhitespace(Format().GetSeparator());
    core_constraints_.Elements().Write(ctx, value[i]);
  }
}

template <typename MoriartyElementType>
MArray<MoriartyElementType>::vector_value_type
MArray<MoriartyElementType>::ReadImpl(
    librarian::ReadVariableContext ctx) const {
  std::optional<int64_t> length =
      core_constraints_.Length().GetUniqueValue(ctx);
  if (!length)
    ctx.ThrowIOError("Cannot determine the length of array before read.");

  vector_value_type res;
  res.reserve(*length);
  for (int i = 0; i < *length; i++) {
    if (i > 0) ctx.ReadWhitespace(Format().GetSeparator());
    res.push_back(core_constraints_.Elements().Read(ctx));
  }
  return res;
}

template <typename MoriartyElementType>
std::vector<MArray<MoriartyElementType>>
MArray<MoriartyElementType>::ListEdgeCasesImpl(
    librarian::AnalyzeVariableContext ctx) const {
  if (!core_constraints_.LengthConstrained()) {
    throw ConfigurationError(
        "MArray::ListEdgeCases",
        "Attempting to get difficult instances of an Array with no "
        "length parameter given.");
  }
  std::vector<MArray<MoriartyElementType>> cases;
  std::vector<MInteger> length_cases =
      core_constraints_.Length().ListEdgeCases(ctx);

  cases.reserve(length_cases.size());
  for (const auto& c : length_cases) {
    cases.push_back(MArray(Length(c)));
  }
  // TODO(hivini): Add cases for sort and the difficult instances of the
  // variable it holds.
  return cases;
};

template <typename MoriartyElementType>
bool MArray<MoriartyElementType>::CoreConstraints::ElementsConstrained() const {
  return IsSet(Flags::kElements);
}

template <typename MoriartyElementType>
const MoriartyElementType&
MArray<MoriartyElementType>::CoreConstraints::Elements() const {
  return data_->elements;
}

template <typename MoriartyElementType>
bool MArray<MoriartyElementType>::CoreConstraints::LengthConstrained() const {
  return IsSet(Flags::kLength);
}

template <typename MoriartyElementType>
const MInteger& MArray<MoriartyElementType>::CoreConstraints::Length() const {
  return data_->length;
}

template <typename MoriartyElementType>
bool MArray<MoriartyElementType>::CoreConstraints::DistinctElements() const {
  return IsSet(Flags::kDistinctElements);
}

template <typename MoriartyElementType>
bool MArray<MoriartyElementType>::CoreConstraints::ComparatorConstrained()
    const {
  return IsSet(Flags::kComparator);
}

template <typename MoriartyElementType>
const typename MArray<MoriartyElementType>::comparator_type&
MArray<MoriartyElementType>::CoreConstraints::Comparator() const {
  return data_->comparator;
}

template <typename MoriartyElementType>
bool MArray<MoriartyElementType>::CoreConstraints::IsSet(Flags flag) const {
  return (data_->touched & static_cast<std::underlying_type_t<Flags>>(flag)) !=
         0;
}

}  // namespace moriarty

#endif  // MORIARTY_VARIABLES_MARRAY_H_
