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

#ifndef MORIARTY_SRC_VARIABLES_MTUPLE_H_
#define MORIARTY_SRC_VARIABLES_MTUPLE_H_

#include <concepts>
#include <cstddef>
#include <format>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "absl/strings/str_join.h"
#include "src/constraints/base_constraints.h"
#include "src/constraints/constraint_violation.h"
#include "src/constraints/container_constraints.h"
#include "src/constraints/io_constraints.h"
#include "src/contexts/librarian_context.h"
#include "src/internal/abstract_variable.h"
#include "src/librarian/errors.h"
#include "src/librarian/io_config.h"
#include "src/librarian/mvariable.h"
#include "src/librarian/util/locked_optional.h"
#include "src/librarian/util/ref.h"

namespace moriarty {

// MTuple<>
//
// Describes constraints placed on an ordered tuple of objects. All objects in
// the tuple must have a corresponding MVariable.
//
// This can hold as many objects as you'd like. For example:
//
//    MTuple<MInteger, MInteger>
// or
//    MTuple<
//          MArray<MInteger>,
//          MArray<MTuple<MInteger, MInteger, MString>>,
//          MTuple<MInteger, MString>
//         >
template <typename... MElementTypes>
class MTuple : public librarian::MVariable<
                   MTuple<MElementTypes...>,
                   std::tuple<typename MElementTypes::value_type...>> {
 public:
  using tuple_value_type = std::tuple<typename MElementTypes::value_type...>;
  class Reader;  // Forward declaration
  using chunked_reader_type = Reader;

  // Create an MTuple from a set of constraints. Logically equivalent to
  // calling AddConstraint() for each constraint.
  //
  // E.g.,
  //     MTuple<MInteger, MInteger>(
  //          Element<0, MInteger>(Between(1, 10)),
  //          Element<1, MString>(Length(15)));
  template <typename... Constraints>
    requires(ConstraintFor<MTuple, Constraints> && ...)
  explicit MTuple(Constraints&&... constraints);

  // Create an MTuple by specifying the MVariables directly.
  //
  // E.g.,
  //     MTuple(MInteger(Between(1, 10)), String(Length(15)));
  explicit MTuple(MElementTypes... values);

  ~MTuple() override = default;

  // Typename()
  //
  // Returns a string representing the name of this type (for example,
  // "MTuple<MInteger, MInteger, MString>"). This is mostly used for
  // debugging/error messages.
  [[nodiscard]] std::string Typename() const override;

  using librarian::MVariable<MTuple,
                             tuple_value_type>::AddConstraint;  // Custom
                                                                // constraints

  MTuple& AddConstraint(Exactly<tuple_value_type> constraint);
  MTuple& AddConstraint(OneOf<tuple_value_type> constraint);

  MTuple& AddConstraint(IOSeparator constraint);

  template <size_t I, typename MElementType>
  MTuple& AddConstraint(Element<I, MElementType> constraint);

  // MTuple::CoreConstraints
  //
  // A base set of constraints for `MTuple` that are used during generation.
  // Note: Returned references are invalidated after any non-const call to this
  // class or the corresponding `MTuple`.
  class CoreConstraints {
   public:
    const std::tuple<MElementTypes...>& Elements() const;

   private:
    friend class MTuple;
    struct Data {  // Must be public since MTuple is a templated class.
      std::tuple<MElementTypes...> elements;
    };
    librarian::CowPtr<Data> data_;
  };
  [[nodiscard]] CoreConstraints GetCoreConstraints() const {
    return core_constraints_;
  }

  // MTuple::Reader
  //
  // An object that can read in an MTuple in chunks. One element at a time.
  // Whitespace must be handled outside of this.
  class Reader {
   public:
    explicit Reader(librarian::ReaderContext ctx, int num_chunks,
                    Ref<const MTuple> variable);
    void ReadNext(librarian::ReaderContext ctx);
    tuple_value_type Finalize() &&;

   private:
    std::size_t current_index_ = 0;
    tuple_value_type values_;
    Ref<const MTuple> variable_;

    template <std::size_t... Is>
    void ReadCurrentIndex(librarian::ReaderContext ctx,
                          std::index_sequence<Is...>);
  };

 private:
  CoreConstraints core_constraints_;
  librarian::LockedOptional<Whitespace> separator_ =
      librarian::LockedOptional<Whitespace>{Whitespace::kSpace};

  // ---------------------------------------------------------------------------
  //  MVariable overrides
  tuple_value_type GenerateImpl(librarian::ResolverContext ctx) const override;
  tuple_value_type ReadImpl(librarian::ReaderContext ctx) const override;
  void PrintImpl(librarian::PrinterContext ctx,
                 const tuple_value_type& value) const override;
  // ---------------------------------------------------------------------------

  template <size_t I, typename MElementType>
  struct ElementConstraintWrapper {
   public:
    explicit ElementConstraintWrapper(Element<I, MElementType> constraint);
    ConstraintViolation CheckValue(librarian::AnalysisContext ctx,
                                   const tuple_value_type& value) const;
    std::string ToString() const;
    std::vector<std::string> GetDependencies() const;
    void ApplyTo(MTuple& other) const;

   private:
    Element<I, MElementType> constraint_;
  };
};

// Class template argument deduction (CTAD). Allows for `Array(MInteger())`
// instead of `Array<MInteger>(MInteger())`. See `NestedArray()` for nesting
// multiple `Arrays` inside one another.
template <typename... MElementTypes>
MTuple(MElementTypes...) -> MTuple<MElementTypes...>;

// -----------------------------------------------------------------------------
//  Template Implementation Below

template <typename... T>
MTuple<T...>::MTuple(T... values) {
  static_assert(
      (MoriartyVariable<T> && ...),
      "MTuple<T1, T2> requires T1 and T2 to be MVariables (E.g., MInteger or "
      "MString).");

  auto apply_one = [&]<std::size_t I>() {
    this->AddConstraint(
        Element<I, typename std::tuple_element_t<I, std::tuple<T...>>>(
            std::get<I>(std::tuple{values...})));
  };

  [&]<size_t... I>(std::index_sequence<I...>) {
    (apply_one.template operator()<I>(), ...);
  }(std::index_sequence_for<T...>{});
}

template <typename... T>
template <typename... Constraints>
  requires(ConstraintFor<MTuple<T...>, Constraints> && ...)
MTuple<T...>::MTuple(Constraints&&... constraints) {
  static_assert(
      (MoriartyVariable<T> && ...),
      "MTuple<T1, T2> requires T1 and T2 to be MVariables (E.g., MInteger or "
      "MString).");
  (AddConstraint(std::forward<Constraints>(constraints)), ...);
}

namespace moriarty_internal {

template <size_t I>
std::string TupleSubVariable() {
  return std::format("<{}>", I);
}

}  // namespace moriarty_internal

template <typename... T>
std::string MTuple<T...>::Typename() const {
  std::tuple typenames = std::apply(
      [](auto&&... elem) { return std::make_tuple(elem.Typename()...); },
      core_constraints_.Elements());
  return std::format("MTuple<{}>", absl::StrJoin(typenames, ", "));
}

template <typename... T>
MTuple<T...>& MTuple<T...>::AddConstraint(
    Exactly<tuple_value_type> constraint) {
  return this->InternalAddExactlyConstraint(std::move(constraint));
}

template <typename... T>
MTuple<T...>& MTuple<T...>::AddConstraint(OneOf<tuple_value_type> constraint) {
  return this->InternalAddOneOfConstraint(std::move(constraint));
}

template <typename... T>
MTuple<T...>& MTuple<T...>::AddConstraint(IOSeparator constraint) {
  if (!separator_.Set(constraint.GetSeparator())) {
    throw ImpossibleToSatisfy(
        "Attempting to set multiple I/O separators for the same MTuple.");
  }
  return this->InternalAddConstraint(std::move(constraint));
}

template <typename... T>
template <size_t I, typename MElementType>
MTuple<T...>& MTuple<T...>::AddConstraint(Element<I, MElementType> constraint) {
  static_assert(
      std::same_as<MElementType,
                   typename std::tuple_element_t<I, std::tuple<T...>>>,
      "Element I in the tuple does not match the type passed in the "
      "constraint");

  std::get<I>(core_constraints_.data_.Mutable().elements)
      .MergeFrom(constraint.GetConstraints());
  return this->InternalAddConstraint(
      ElementConstraintWrapper(std::move(constraint)));
}

template <typename... T>
MTuple<T...>::tuple_value_type MTuple<T...>::GenerateImpl(
    librarian::ResolverContext ctx) const {
  if (this->GetOneOf().HasBeenConstrained())
    return this->GetOneOf().SelectOneOf(
        [&](int n) { return ctx.RandomInteger(n); });

  auto generate_one = [&]<std::size_t I>() {
    return std::get<I>(core_constraints_.Elements())
        .Generate(ctx.ForSubVariable(moriarty_internal::TupleSubVariable<I>()));
  };

  return [&]<size_t... I>(std::index_sequence<I...>) {
    return tuple_value_type { generate_one.template operator()<I>()... };
  }(std::index_sequence_for<T...>{});
}

template <typename... T>
void MTuple<T...>::PrintImpl(librarian::PrinterContext ctx,
                             const tuple_value_type& value) const {
  auto print_one = [&]<std::size_t I>() {
    if (I > 0) ctx.PrintWhitespace(separator_.Get());
    std::get<I>(core_constraints_.Elements()).Print(ctx, std::get<I>(value));
  };

  [&]<size_t... I>(std::index_sequence<I...>) {
    (print_one.template operator()<I>(), ...);
  }(std::index_sequence_for<T...>{});
}

template <typename... T>
MTuple<T...>::tuple_value_type MTuple<T...>::ReadImpl(
    librarian::ReaderContext ctx) const {
  MTuple<T...>::Reader reader = MTuple<T...>::Reader(ctx, sizeof...(T), *this);

  for (size_t i = 0; i < sizeof...(T); ++i) {
    if (i > 0) ctx.ReadWhitespace(separator_.Get());
    reader.ReadNext(ctx);
  }
  return std::move(reader).Finalize();
}

template <typename... T>
template <size_t I, typename MElementType>
MTuple<T...>::ElementConstraintWrapper<I, MElementType>::
    ElementConstraintWrapper(Element<I, MElementType> constraint)
    : constraint_(std::move(constraint)){};

template <typename... T>
template <size_t I, typename MElementType>
ConstraintViolation
MTuple<T...>::ElementConstraintWrapper<I, MElementType>::CheckValue(
    librarian::AnalysisContext ctx, const tuple_value_type& value) const {
  return constraint_.CheckValue(ctx, std::get<I>(value));
}

template <typename... T>
template <size_t I, typename MElementType>
std::string MTuple<T...>::ElementConstraintWrapper<I, MElementType>::ToString()
    const {
  return constraint_.ToString();
}

template <typename... T>
template <size_t I, typename MElementType>
std::vector<std::string> MTuple<T...>::ElementConstraintWrapper<
    I, MElementType>::GetDependencies() const {
  return constraint_.GetDependencies();
}

template <typename... T>
template <size_t I, typename MElementType>
void MTuple<T...>::ElementConstraintWrapper<I, MElementType>::ApplyTo(
    MTuple& other) const {
  other.AddConstraint(constraint_);
}

template <typename... MElementTypes>
const std::tuple<MElementTypes...>&
MTuple<MElementTypes...>::CoreConstraints::Elements() const {
  return data_->elements;
}

template <typename... MElementTypes>
MTuple<MElementTypes...>::Reader::Reader(librarian::ReaderContext ctx,
                                         int num_chunks,
                                         Ref<const MTuple> variable)
    : variable_(std::move(variable)) {
  if (num_chunks != sizeof...(MElementTypes)) {
    throw ConfigurationError(
        "MTuple::Reader",
        std::format(
            "Asked to read {} elements, but there are {} elements in {}.",
            num_chunks, sizeof...(MElementTypes),
            MTuple<MElementTypes...>().Typename()));
  }
}

template <typename... MElementTypes>
void MTuple<MElementTypes...>::Reader::ReadNext(librarian::ReaderContext ctx) {
  if (current_index_ >= sizeof...(MElementTypes)) {
    ctx.ThrowIOError(
        std::format("{}: Attempting to read more elements than exist in tuple.",
                    MTuple<MElementTypes...>().Typename()));
  }
  ReadCurrentIndex(ctx, std::index_sequence_for<MElementTypes...>{});
  current_index_++;
}

template <typename... MElementTypes>
MTuple<MElementTypes...>::tuple_value_type
MTuple<MElementTypes...>::Reader::Finalize() && {
  return std::move(values_);
}

template <typename... MElementTypes>
template <std::size_t... Is>
void MTuple<MElementTypes...>::Reader::ReadCurrentIndex(
    librarian::ReaderContext ctx, std::index_sequence<Is...>) {
  auto read_element = [&]<std::size_t I>() {
    if constexpr (I < sizeof...(Is)) {
      if (current_index_ == I) {
        std::get<I>(values_) =
            std::get<I>(variable_.get().GetCoreConstraints().Elements())
                .Read(ctx);
        return true;
      }
    }
    return false;
  };

  (... || read_element.template operator()<Is>());
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_MTUPLE_H_
