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
#include "absl/status/status.h"
#include "absl/strings/str_join.h"
#include "src/contexts/librarian/analysis_context.h"
#include "src/contexts/librarian/printer_context.h"
#include "src/contexts/librarian/reader_context.h"
#include "src/contexts/librarian/resolver_context.h"
#include "src/io_config.h"
#include "src/librarian/locked_optional.h"
#include "src/librarian/mvariable.h"

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

  MTuple();
  explicit MTuple(MElementTypes... values);

  // Typename()
  //
  // Returns a string representing the name of this type (for example,
  // "MTuple<MInteger, MInteger, MString>"). This is mostly used for
  // debugging/error messages.
  [[nodiscard]] std::string Typename() const override;

  // Of()
  //
  // Sets the argument matching `index` in the tuple.
  //
  // Example:
  //   Of<0>(MInteger().Between(1, 10));
  //
  // TODO(b/208296530): We should be taking the name as a parameter instead of
  // the index as a template argument.
  // TODO(b/208296530): `Of()` is a pretty bad name... We should change it to
  // something more meaningful.
  template <int index, typename T>
  MTuple& Of(T variable);

  // WithSeparator()
  //
  // Sets the whitespace separator to be used between different elements
  // when reading/writing. Default = kSpace.
  MTuple& WithSeparator(Whitespace separator);

 private:
  std::tuple<MElementTypes...> elements_;
  librarian::LockedOptional<Whitespace> separator_ =
      librarian::LockedOptional<Whitespace>{Whitespace::kSpace};

  // ---------------------------------------------------------------------------
  //  MVariable overrides
  tuple_value_type GenerateImpl(librarian::ResolverContext ctx) const override;
  absl::Status IsSatisfiedWithImpl(
      librarian::AnalysisContext ctx,
      const tuple_value_type& value) const override;
  absl::Status MergeFromImpl(const MTuple& other) override;
  tuple_value_type ReadImpl(librarian::ReaderContext ctx) const override;
  void PrintImpl(librarian::PrinterContext ctx,
                 const tuple_value_type& value) const override;
  std::vector<std::string> GetDependenciesImpl() const override;
  // ---------------------------------------------------------------------------
};

// Class template argument deduction (CTAD). Allows for `Array(MInteger())`
// instead of `Array<MInteger>(MInteger())`. See `NestedArray()` for nesting
// multiple `Arrays` inside one another.
template <typename... MElementTypes>
MTuple(MElementTypes...) -> MTuple<MElementTypes...>;

// -----------------------------------------------------------------------------
//  Template Implementation Below

template <typename... T>
MTuple<T...>::MTuple() {
  static_assert(
      (std::derived_from<T, librarian::MVariable<T, typename T::value_type>> &&
       ...),
      "The T1, T2, etc used in MTuple<T1, T2> must be a Moriarty variable. "
      "For example, MTuple<MInteger, MString>.");
}

template <typename... T>
MTuple<T...>::MTuple(T... values) : elements_(values...) {
  static_assert(
      (std::derived_from<T, librarian::MVariable<T, typename T::value_type>> &&
       ...),
      "The T1, T2, etc used in MTuple<T1, T2> must be a Moriarty variable. "
      "For example, MTuple<MInteger, MString>.");
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
MTuple<T...>& MTuple<T...>::WithSeparator(Whitespace separator) {
  if (!separator_.Set(separator)) {
    throw std::runtime_error(
        "Attempting to set multiple I/O separators for the same MTuple.");
  }
  return *this;
}

template <typename... T>
template <int index, typename U>
MTuple<T...>& MTuple<T...>::Of(U variable) {
  static_assert(
      std::same_as<U, typename std::tuple_element_t<index, std::tuple<T...>>>,
      "Parameter passed to MTuple::Of<> is the wrong type.");
  std::get<index>(elements_).MergeFrom(std::move(variable));
  return *this;
}

template <typename... T>
absl::Status MTuple<T...>::MergeFromImpl(const MTuple<T...>& other) {
  auto merge_one = [&]<std::size_t I>() {
    std::get<I>(elements_).MergeFrom(std::get<I>(other.elements_));
  };
  if (other.separator_.IsSet()) separator_.Set(other.separator_.Get());

  [&]<size_t... I>(std::index_sequence<I...>) {
    (merge_one.template operator()<I>(), ...);
  }(std::index_sequence_for<T...>{});

  return absl::OkStatus();
}

template <typename... T>
MTuple<T...>::tuple_value_type MTuple<T...>::GenerateImpl(
    librarian::ResolverContext ctx) const {
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
absl::Status MTuple<T...>::IsSatisfiedWithImpl(
    librarian::AnalysisContext ctx, const tuple_value_type& value) const {
  absl::Status status;
  auto check_one = [&]<std::size_t I>() {
    if (!status.ok()) return;
    status = std::get<I>(elements_).IsSatisfiedWith(ctx, std::get<I>(value));
  };

  [&]<size_t... I>(std::index_sequence<I...>) {
    (check_one.template operator()<I>(), ...);
  }(std::index_sequence_for<T...>{});

  return status;
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
