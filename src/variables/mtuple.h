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
#include <iterator>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/strings/str_join.h"
#include "src/contexts/librarian/analysis_context.h"
#include "src/contexts/librarian/printer_context.h"
#include "src/contexts/librarian/reader_context.h"
#include "src/contexts/librarian/resolver_context.h"
#include "src/io_config.h"
#include "src/librarian/locked_optional.h"
#include "src/librarian/mvariable.h"
#include "src/librarian/one_of_handler.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/constraints/container_constraints.h"
#include "src/variables/constraints/io_constraints.h"

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

  // Create an MTuple from a set of constraints. Logically equivalent to
  // calling AddConstraint() for each constraint.
  //
  // E.g.,
  //     MTuple<MInteger, MInteger>(
  //          Element<0, MInteger>(Between(1, 10)),
  //          Element<1, MString>(Length(15)));
  template <typename... Constraints>
    requires(std::derived_from<std::decay_t<Constraints>, MConstraint> && ...)
  explicit MTuple(Constraints&&... constraints);

  // Create an MTuple by specifying the MVariables directly.
  //
  // E.g.,
  //     MTuple(MInteger(Between(1, 10)), String(Length(15)));
  explicit MTuple(MElementTypes... values);

  // Typename()
  //
  // Returns a string representing the name of this type (for example,
  // "MTuple<MInteger, MInteger, MString>"). This is mostly used for
  // debugging/error messages.
  [[nodiscard]] std::string Typename() const override;

  MTuple& AddConstraint(Exactly<tuple_value_type> constraint);
  MTuple& AddConstraint(OneOf<tuple_value_type> constraint);

  MTuple& AddConstraint(IOSeparator constraint);

  template <size_t I, typename MElementType>
  MTuple& AddConstraint(Element<I, MElementType> constraint);

 private:
  librarian::OneOfHandler<tuple_value_type> one_of_;
  std::tuple<MElementTypes...> elements_;
  librarian::LockedOptional<Whitespace> separator_ =
      librarian::LockedOptional<Whitespace>{Whitespace::kSpace};

  // ---------------------------------------------------------------------------
  //  MVariable overrides
  tuple_value_type GenerateImpl(librarian::ResolverContext ctx) const override;
  tuple_value_type ReadImpl(librarian::ReaderContext ctx) const override;
  void PrintImpl(librarian::PrinterContext ctx,
                 const tuple_value_type& value) const override;
  std::vector<std::string> GetDependenciesImpl() const override;
  // ---------------------------------------------------------------------------

  template <size_t I, typename MElementType>
  struct ElementConstraintWrapper {
   public:
    explicit ElementConstraintWrapper(Element<I, MElementType> constraint)
        : constraint_(std::move(constraint)) {};

    bool IsSatisfiedWith(librarian::AnalysisContext ctx,
                         const tuple_value_type& value) const {
      return constraint_.IsSatisfiedWith(ctx, std::get<I>(value));
    }
    std::string Explanation(librarian::AnalysisContext ctx,
                            const tuple_value_type& value) const {
      return constraint_.Explanation(ctx, std::get<I>(value));
    }
    std::string ToString() const { return constraint_.ToString(); }
    void ApplyTo(MTuple& other) const { other.AddConstraint(constraint_); }

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
      (std::derived_from<T, librarian::MVariable<T, typename T::value_type>> &&
       ...),
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
  requires(std::derived_from<std::decay_t<Constraints>, MConstraint> && ...)
MTuple<T...>::MTuple(Constraints&&... constraints) {
  static_assert(
      (std::derived_from<T, librarian::MVariable<T, typename T::value_type>> &&
       ...),
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
      elements_);
  return std::format("MTuple<{}>", absl::StrJoin(typenames, ", "));
}

template <typename... T>
MTuple<T...>& MTuple<T...>::AddConstraint(
    Exactly<tuple_value_type> constraint) {
  one_of_.ConstrainOptions(
      std::vector<tuple_value_type>{constraint.GetValue()});
  this->InternalAddConstraint(std::move(constraint));
  return *this;
}

template <typename... T>
MTuple<T...>& MTuple<T...>::AddConstraint(OneOf<tuple_value_type> constraint) {
  one_of_.ConstrainOptions(constraint.GetOptions());
  this->InternalAddConstraint(std::move(constraint));
  return *this;
}

template <typename... T>
MTuple<T...>& MTuple<T...>::AddConstraint(IOSeparator constraint) {
  if (!separator_.Set(constraint.GetSeparator())) {
    throw std::runtime_error(
        "Attempting to set multiple I/O separators for the same MTuple.");
  }
  this->InternalAddConstraint(std::move(constraint));
  return *this;
}

template <typename... T>
template <size_t I, typename MElementType>
MTuple<T...>& MTuple<T...>::AddConstraint(Element<I, MElementType> constraint) {
  static_assert(
      std::same_as<MElementType,
                   typename std::tuple_element_t<I, std::tuple<T...>>>,
      "Element I in the tuple does not match the type passed in the "
      "constraint");

  std::get<I>(elements_).MergeFrom(constraint.GetConstraints());
  this->InternalAddConstraint(ElementConstraintWrapper(std::move(constraint)));
  return *this;
}

template <typename... T>
MTuple<T...>::tuple_value_type MTuple<T...>::GenerateImpl(
    librarian::ResolverContext ctx) const {
  if (one_of_.HasBeenConstrained())
    return one_of_.SelectOneOf([&](int n) { return ctx.RandomInteger(n); });

  auto generate_one = [&]<std::size_t I>() {
    return std::get<I>(elements_).Generate(
        ctx.ForSubVariable(moriarty_internal::TupleSubVariable<I>()));
  };

  return [&]<size_t... I>(std::index_sequence<I...>) {
    return tuple_value_type { generate_one.template operator()<I>()... };
  }(std::index_sequence_for<T...>{});
}

template <typename... T>
std::vector<std::string> MTuple<T...>::GetDependenciesImpl() const {
  std::vector<std::string> dependencies;
  std::apply(
      [&](const auto&... elems) {
        ((absl::c_move(elems.GetDependencies(),
                       std::back_inserter(dependencies))),
         ...);
      },
      elements_);
  return dependencies;
}

template <typename... T>
void MTuple<T...>::PrintImpl(librarian::PrinterContext ctx,
                             const tuple_value_type& value) const {
  auto print_one = [&]<std::size_t I>() {
    if (I > 0) ctx.PrintWhitespace(separator_.Get());
    std::get<I>(elements_).Print(ctx, std::get<I>(value));
  };

  [&]<size_t... I>(std::index_sequence<I...>) {
    (print_one.template operator()<I>(), ...);
  }(std::index_sequence_for<T...>{});
}

template <typename... T>
MTuple<T...>::tuple_value_type MTuple<T...>::ReadImpl(
    librarian::ReaderContext ctx) const {
  auto read_one = [&]<std::size_t I>() {
    if (I > 0) ctx.ReadWhitespace(separator_.Get());
    return std::get<I>(elements_).Read(ctx);
  };

  return [&]<size_t... I>(std::index_sequence<I...>) {
    return tuple_value_type { read_one.template operator()<I>()... };
  }(std::index_sequence_for<T...>{});
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_MTUPLE_H_
