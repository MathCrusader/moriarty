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

#ifndef MORIARTY_VARIABLES_MVARIANT_H_
#define MORIARTY_VARIABLES_MVARIANT_H_

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "absl/strings/str_join.h"
#include "src/constraints/base_constraints.h"
#include "src/constraints/constraint_violation.h"
#include "src/constraints/container_constraints.h"
#include "src/context.h"
#include "src/contexts/librarian_context.h"
#include "src/internal/abstract_variable.h"
#include "src/librarian/errors.h"
#include "src/librarian/io_config.h"
#include "src/librarian/mvariable.h"
#include "src/librarian/util/ref.h"
#include "src/variables/mnone.h"

namespace moriarty {

template <typename... MAlternativeTypes>
class MVariant;
namespace librarian {
template <typename... MAlternativeTypes>
struct MVariableValueTypeTrait<MVariant<MAlternativeTypes...>> {
  using type = std::variant<typename MAlternativeTypes::value_type...>;
};

template <typename T, typename Variant, std::size_t... Is>
constexpr std::size_t VariantIndexImpl(std::index_sequence<Is...>) {
  using U = std::remove_cvref_t<T>;

  constexpr std::size_t count =
      (std::size_t(0) + ... +
       (std::is_same_v<U, std::variant_alternative_t<Is, Variant>> ? 1 : 0));

  static_assert(count != 0, "the type T is not an alternative of Variant");
  static_assert(count == 1, "the type T appears more than once in Variant");

  // Find the index
  constexpr std::size_t index =
      ((std::is_same_v<U, std::variant_alternative_t<Is, Variant>> ? Is : 0) +
       ...);

  return index;
}

template <typename T, typename Variant>
constexpr std::size_t VariantIndex() {
  static_assert(std::variant_size_v<Variant> > 0,
                "Variant must be a std::variant");

  return VariantIndexImpl<T, Variant>(
      std::make_index_sequence<std::variant_size_v<Variant>>{});
}

template <typename T, typename Variant>
concept IsVariantAlternative =
    requires { typename std::variant_size<Variant>; } &&
    ([]<std::size_t... Is>(std::index_sequence<Is...>) {
      using U = std::remove_cvref_t<T>;
      return ((std::is_same_v<U, std::variant_alternative_t<Is, Variant>> ? 1
                                                                          : 0) +
              ... + 0) == 1;
    })(std::make_index_sequence<std::variant_size_v<Variant>>{});

}  // namespace librarian

// MVariantFormat
//
// How to format an MVariant when reading/writing.
class MVariantFormat {
 public:
  // Sets the whitespace to be used between the discriminator and the value.
  // If the type of the discriminator is MNone, the separator will not be
  // read/written since MNone has no value.
  //
  // Default: kSpace
  MVariantFormat& WithSeparator(Whitespace separator);
  // Returns the whitespace separator between the discriminator and the value.
  Whitespace GetSeparator() const;

  // Uses the space separator between the discriminator and the value.
  MVariantFormat& SpaceSeparated();
  // Uses the newline separator between the discriminator and the value.
  MVariantFormat& NewlineSeparated();

  // Sets the discriminator options.
  MVariantFormat& Discriminator(std::vector<std::string> options);
  // Returns the discriminator options.
  const std::vector<std::string>& GetDiscriminatorOptions() const;

  // Take any non-defaults in `other` and apply them to this format.
  void Merge(const MVariantFormat& other);

 private:
  Whitespace separator_ = Whitespace::kSpace;
  std::vector<std::string> discriminator_options_;
};

// MVariant<>
//
// Describes constraints placed on a discriminated union (aka, a type that may
// have multiple different "variants" of values and a discriminator to decide
// which variant is active). For example, you might want a variable that can
// either be an integer between 1 and 10 or a string of length 5. You could
// represent this as `MVariant(MInteger(Between(1, 10)), MString(Length(5)))`.
// The discriminator must always be the first thing read/written.
//
// This can hold as many objects as you'd like. For example:
//
//    MVariant<MInteger, MInteger>
// or
//    MVariant<
//          MArray<MInteger>,
//          MArray<MTuple<MInteger, MInteger, MString>>,
//          MTuple<MInteger, MString>
//         >
//
// Use `MNone` to represent a "null" variant. For example,
//     MVariant<MNone, MInteger>
//
// See MVariantFormat for formatting options (especially about discriminator and
// MNone).
template <typename... MAlternativeTypes>
class MVariant : public librarian::MVariable<MVariant<MAlternativeTypes...>> {
 public:
  using variant_value_type =
      std::variant<typename MAlternativeTypes::value_type...>;

  // Create an MVariant from a set of constraints. Logically equivalent to
  // calling AddConstraint() for each constraint.
  //
  // E.g.,
  //     MVariant<MInteger, MInteger>(
  //          Element<0, MInteger>(Between(1, 10)),
  //          Element<1, MString>(Length(15)));
  template <typename... Constraints>
    requires(sizeof...(MAlternativeTypes) > 0 &&
             (ConstraintFor<MVariant, Constraints> && ...))
  explicit MVariant(Constraints&&... constraints);

  // Create an MVariant by specifying the MVariables directly.
  //
  // E.g.,
  //     MVariant(MInteger(Between(1, 10)), MString(Length(15)));
  explicit MVariant(MAlternativeTypes... values);

  ~MVariant() override = default;

  // Typename()
  //
  // Returns a string representing the name of this type (for example,
  // "MVariant<MInteger, MInteger, MString>"). This is mostly used for
  // debugging/error messages.
  [[nodiscard]] std::string Typename() const override;

  using librarian::MVariable<MVariant>::AddConstraint;  // Custom constraints

  // ---------------------------------------------------------------------------
  //  Constrain the value to a specific set of values

  // The variant must be exactly this value. (Exactly<std::variant<A, B, C>>)
  MVariant& AddConstraint(Exactly<variant_value_type> constraint);
  // The variant must be exactly this value. (Exactly<A>, Exactly<B>, etc.)
  template <typename AlternativeType>
    requires(
        librarian::IsVariantAlternative<AlternativeType, variant_value_type>)
  MVariant& AddConstraint(Exactly<AlternativeType> constraint);
  // The variant must be one of these values. (OneOf<std::variant<A, B, C>>)
  MVariant& AddConstraint(OneOf<variant_value_type> constraint);
  // The variant must be one of these values. (OneOf<A>, OneOf<B>, etc.)
  template <typename AlternativeType>
    requires(
        librarian::IsVariantAlternative<AlternativeType, variant_value_type>)
  MVariant& AddConstraint(OneOf<AlternativeType> constraint);

  // ---------------------------------------------------------------------------
  //  Constrain the alternatives in the variant

  // FIXME: This should be Alternative<I> instead of Element<I>.
  // The I'th alternative in the variant must satisfy these constraints.
  template <size_t I, typename MElementType>
  MVariant& AddConstraint(Element<I, MElementType> constraint);

  // ---------------------------------------------------------------------------
  //  Constrain the I/O format of the variant

  MVariant& AddConstraint(MVariantFormat constraint);

  MVariantFormat& Format();
  MVariantFormat Format() const;

  // MVariant::CoreConstraints
  //
  // A base set of constraints for `MVariant` that are used during generation.
  // Note: Returned references are invalidated after any non-const call to this
  // class or the corresponding `MVariant`.
  class CoreConstraints {
   public:
    bool ElementsConstrained() const;
    const std::tuple<MAlternativeTypes...>& Elements() const;

   private:
    friend class MVariant;
    enum Flags : uint32_t {
      kElements = 1 << 0,
    };
    struct Data {
      std::underlying_type_t<Flags> touched = 0;
      std::tuple<MAlternativeTypes...> elements;
    };
    librarian::CowPtr<Data> data_;
    bool IsSet(Flags flag) const;
  };
  [[nodiscard]] CoreConstraints GetCoreConstraints() const {
    return core_constraints_;
  }

 private:
  CoreConstraints core_constraints_;
  MVariantFormat format_;

  // ---------------------------------------------------------------------------
  //  MVariable overrides
  variant_value_type GenerateImpl(
      librarian::GenerateVariableContext ctx) const override;
  variant_value_type ReadImpl(
      librarian::ReadVariableContext ctx) const override;
  void WriteImpl(librarian::WriteVariableContext ctx,
                 const variant_value_type& value) const override;
  // ---------------------------------------------------------------------------

  template <size_t I, typename MElementType>
  struct ElementConstraintWrapper {
   public:
    explicit ElementConstraintWrapper(Element<I, MElementType> constraint);
    ConstraintViolation CheckValue(ConstraintContext ctx,
                                   const variant_value_type& value) const;
    std::string ToString() const;
    std::vector<std::string> GetDependencies() const;
    void ApplyTo(MVariant& other) const;

   private:
    Element<I, MElementType> constraint_;
  };
};

// Class template argument deduction (CTAD). Allows for `MVariant(MInteger(),
// MString())` instead of `MVariant<MInteger, MString>(MInteger(), MString())`.
template <typename... MAlternativeTypes>
MVariant(MAlternativeTypes...) -> MVariant<MAlternativeTypes...>;

// -----------------------------------------------------------------------------
//  Template Implementation Below

template <typename... T>
MVariant<T...>::MVariant(T... values) {
  static_assert(
      (MoriartyVariable<T> && ...),
      "MVariant<T1, T2> requires T1 and T2 to be MVariables (E.g., MInteger or "
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
  requires(sizeof...(T) > 0 &&
           (ConstraintFor<MVariant<T...>, Constraints> && ...))
MVariant<T...>::MVariant(Constraints&&... constraints) {
  static_assert(
      (MoriartyVariable<T> && ...),
      "MVariant<T1, T2> requires T1 and T2 to be MVariables (E.g., MInteger or "
      "MString).");
  (AddConstraint(std::forward<Constraints>(constraints)), ...);
}

namespace moriarty_internal {

template <size_t I>
std::string VariantSubVariable() {
  return std::format("<{}>", I);
}

}  // namespace moriarty_internal

template <typename... T>
std::string MVariant<T...>::Typename() const {
  std::tuple typenames = std::apply(
      [](auto&&... elem) { return std::make_tuple(elem.Typename()...); },
      core_constraints_.Elements());
  return std::format("MVariant<{}>", absl::StrJoin(typenames, ", "));
}

template <typename... T>
MVariant<T...>& MVariant<T...>::AddConstraint(
    Exactly<variant_value_type> constraint) {
  return this->InternalAddExactlyConstraint(std::move(constraint));
}

template <typename... T>
MVariant<T...>& MVariant<T...>::AddConstraint(
    OneOf<variant_value_type> constraint) {
  return this->InternalAddOneOfConstraint(std::move(constraint));
}

template <typename... T>
template <typename AlternativeType>
  requires(librarian::IsVariantAlternative<
           AlternativeType, typename MVariant<T...>::variant_value_type>)
MVariant<T...>& MVariant<T...>::AddConstraint(
    Exactly<AlternativeType> constraint) {
  constexpr std::size_t index =
      librarian::VariantIndex<AlternativeType, variant_value_type>();
  return this->InternalAddExactlyConstraint(Exactly<variant_value_type>(
      variant_value_type{std::in_place_index<index>, constraint.GetValue()}));
}

template <typename... T>
template <typename AlternativeType>
  requires(librarian::IsVariantAlternative<
           AlternativeType, typename MVariant<T...>::variant_value_type>)
MVariant<T...>& MVariant<T...>::AddConstraint(
    OneOf<AlternativeType> constraint) {
  constexpr std::size_t index =
      librarian::VariantIndex<AlternativeType, variant_value_type>();
  std::vector<variant_value_type> options;
  for (const auto& option : constraint.GetOptions()) {
    options.emplace_back(std::in_place_index<index>, option);
  }
  return this->InternalAddOneOfConstraint(
      OneOf<variant_value_type>(std::move(options)));
}

template <typename... T>
MVariant<T...>& MVariant<T...>::AddConstraint(MVariantFormat constraint) {
  format_.Merge(constraint);
  return *this;
}

template <typename... T>
MVariantFormat& MVariant<T...>::Format() {
  return format_;
}

template <typename... T>
MVariantFormat MVariant<T...>::Format() const {
  return format_;
}

template <typename... T>
template <size_t I, typename MAlternativeType>
MVariant<T...>& MVariant<T...>::AddConstraint(
    Element<I, MAlternativeType> constraint) {
  static_assert(
      std::same_as<MAlternativeType,
                   typename std::tuple_element_t<I, std::tuple<T...>>>,
      "Alternative I in the variant does not match the type passed in the "
      "constraint");
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kElements;

  std::get<I>(constraints.elements).MergeFrom(constraint.GetConstraints());
  return this->InternalAddConstraint(
      ElementConstraintWrapper(std::move(constraint)));
}

template <typename... T>
MVariant<T...>::variant_value_type MVariant<T...>::GenerateImpl(
    librarian::GenerateVariableContext ctx) const {
  if (this->GetOneOf().HasBeenConstrained())
    return this->GetOneOf().SelectOneOf(
        [&](int n) { return ctx.RandomInteger(n); });

  int idx = ctx.RandomInteger(sizeof...(T));

  auto generate_one = [&]<std::size_t I>() {
    return std::get<I>(core_constraints_.Elements())
        .Generate(
            ctx.ForSubVariable(moriarty_internal::VariantSubVariable<I>()));
  };

  std::optional<variant_value_type> generated_value;
  [&]<size_t... I>(std::index_sequence<I...>) {
    (
        [&] {
          if (idx == I) {
            generated_value = variant_value_type{
                std::in_place_index<I>, generate_one.template operator()<I>()};
          }
        }(),
        ...);
  }(std::index_sequence_for<T...>{});

  if (!generated_value.has_value()) {
    throw GenerationError(
        ctx.GetVariableName(),
        std::format("Failed to generate a value for {}. This is likely a bug "
                    "in Moriarty. Please report this to the developers.",
                    this->Typename()),
        RetryPolicy::kAbort);
  }
  return *generated_value;
}

template <typename... T>
void MVariant<T...>::WriteImpl(librarian::WriteVariableContext ctx,
                               const variant_value_type& value) const {
  auto idx = value.index();
  if (idx == std::variant_npos) {
    throw WriteError(std::format(
        "Attempting to write an invalid std::variant value for {} ({}).",
        ctx.GetVariableName(), this->Typename()));
  }

  auto write_one = [&]<std::size_t I>() {
    if (I != idx) return;
    ctx.WriteToken(this->Format().GetDiscriminatorOptions()[I]);
    if constexpr (!std::same_as<std::tuple_element_t<I, std::tuple<T...>>,
                                MNone>) {
      ctx.WriteWhitespace(Format().GetSeparator());
    }
    std::get<I>(core_constraints_.Elements()).Write(ctx, std::get<I>(value));
  };

  [&]<size_t... I>(std::index_sequence<I...>) {
    (write_one.template operator()<I>(), ...);
  }(std::index_sequence_for<T...>{});
}

template <typename... T>
MVariant<T...>::variant_value_type MVariant<T...>::ReadImpl(
    librarian::ReadVariableContext ctx) const {
  std::string discriminator = ctx.ReadToken();
  auto it =
      std::ranges::find(Format().GetDiscriminatorOptions(), discriminator);
  if (it == Format().GetDiscriminatorOptions().end()) {
    ctx.ThrowIOError(std::format(
        "Invalid discriminator '{}'. Expected one of: {}.", discriminator,
        absl::StrJoin(Format().GetDiscriminatorOptions(), ", ")));
  }

  size_t idx = std::distance(Format().GetDiscriminatorOptions().begin(), it);
  auto read_one = [&]<std::size_t I>() -> std::optional<variant_value_type> {
    if constexpr (std::same_as<std::tuple_element_t<I, std::tuple<T...>>,
                               MNone>) {
      return variant_value_type{std::in_place_index<I>};
    }
    ctx.ReadWhitespace(Format().GetSeparator());
    return variant_value_type{
        std::in_place_index<I>,
        std::get<I>(core_constraints_.Elements()).Read(ctx)};
  };

  std::optional<variant_value_type> read_value;
  [&]<size_t... I>(std::index_sequence<I...>) {
    (
        [&] {
          if (I == idx) read_value = read_one.template operator()<I>();
        }(),
        ...);
  }(std::index_sequence_for<T...>{});

  if (!read_value.has_value()) {
    ctx.ThrowIOError(
        std::format("Failed to read a value for {} after reading the "
                    "discriminator. This is likely a bug in Moriarty. Please "
                    "report this to the developers.",
                    this->Typename()));
  }

  return *read_value;
}

template <typename... T>
template <size_t I, typename MAlternativeType>
MVariant<T...>::ElementConstraintWrapper<I, MAlternativeType>::
    ElementConstraintWrapper(Element<I, MAlternativeType> constraint)
    : constraint_(std::move(constraint)){};

template <typename... T>
template <size_t I, typename MAlternativeType>
ConstraintViolation
MVariant<T...>::ElementConstraintWrapper<I, MAlternativeType>::CheckValue(
    ConstraintContext ctx, const variant_value_type& value) const {
  if (value.index() != I) return ConstraintViolation::None();
  return constraint_.CheckValue(ctx, std::get<I>(value));
}

template <typename... T>
template <size_t I, typename MAlternativeType>
std::string MVariant<T...>::ElementConstraintWrapper<
    I, MAlternativeType>::ToString() const {
  return constraint_.ToString();
}

template <typename... T>
template <size_t I, typename MAlternativeType>
std::vector<std::string> MVariant<T...>::ElementConstraintWrapper<
    I, MAlternativeType>::GetDependencies() const {
  return constraint_.GetDependencies();
}

template <typename... T>
template <size_t I, typename MAlternativeType>
void MVariant<T...>::ElementConstraintWrapper<I, MAlternativeType>::ApplyTo(
    MVariant& other) const {
  other.AddConstraint(constraint_);
}

template <typename... MAlternativeTypes>
const std::tuple<MAlternativeTypes...>&
MVariant<MAlternativeTypes...>::CoreConstraints::Elements() const {
  return data_->elements;
}

template <typename... MAlternativeTypes>
bool MVariant<MAlternativeTypes...>::CoreConstraints::ElementsConstrained()
    const {
  return IsSet(Flags::kElements);
}

template <typename... MAlternativeTypes>
bool MVariant<MAlternativeTypes...>::CoreConstraints::IsSet(Flags flag) const {
  return (data_->touched & static_cast<uint32_t>(flag)) != 0;
}

}  // namespace moriarty

#endif  // MORIARTY_VARIABLES_MVARIANT_H_
