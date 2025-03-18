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

#ifndef MORIARTY_SRC_LIBRARIAN_MVARIABLE_H_
#define MORIARTY_SRC_LIBRARIAN_MVARIABLE_H_

#include <algorithm>
#include <concepts>
#include <exception>
#include <format>
#include <iterator>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "src/contexts/internal/mutable_values_context.h"
#include "src/contexts/internal/view_only_context.h"
#include "src/contexts/librarian/analysis_context.h"
#include "src/contexts/librarian/assignment_context.h"
#include "src/contexts/librarian/printer_context.h"
#include "src/contexts/librarian/reader_context.h"
#include "src/contexts/librarian/resolver_context.h"
#include "src/errors.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/generation_config.h"
#include "src/librarian/constraint_handler.h"
#include "src/util/status_macro/status_macros.h"
#include "src/variables/constraints/custom_constraint.h"

namespace moriarty {
namespace librarian {

// MVariable<>
//
// All Moriarty variable types (e.g., MInteger, MString, MGraph, etc) should
// derive from this class using CRTP (curiously recurring template pattern).
//
// * `VariableType` is the underlying Moriarty variable type
// * `ValueType` is the non-Moriarty type that is generated/validated/analyzed
//   by this MVariable.
//
// E.g.,
// class MInteger : public librarian::MVariable<MInteger, int64_t> {}
template <typename VariableType, typename ValueType>
class MVariable : public moriarty_internal::AbstractVariable {
 public:
  using variable_type = VariableType;
  using value_type = ValueType;

  ~MVariable() override = default;

 protected:
  // Only derived classes should make copies of me directly to avoid accidental
  // slicing. Call derived class's constructors instead.
  MVariable() {
    static_assert(std::default_initializable<VariableType>,
                  "Moriarty needs to be able to construct default versions of "
                  "your MVariable using its default constructor.");
    static_assert(std::copyable<VariableType>,
                  "Moriarty needs to be able to copy your MVariable.");
    static_assert(
        std::derived_from<
            VariableType,
            MVariable<VariableType, typename VariableType::value_type>>,
        "The first template argument of MVariable<> must be "
        "VariableType. E.g., class MFoo : public MVariable<MFoo, Foo> {}");
  }
  MVariable(const MVariable&) = default;
  MVariable(MVariable&&) = default;
  MVariable& operator=(const MVariable&) = default;
  MVariable& operator=(MVariable&&) = default;

 public:
  // Typename()
  //
  // Returns a string representing the name of this type (for example,
  // "MInteger"). This is mostly used for debugging/error messages.
  [[nodiscard]] std::string Typename() const override = 0;

  // ToString()
  //
  // Returns a string representation of the constraints this MVariable has.
  [[nodiscard]] std::string ToString() const;

  // MergeFrom()
  //
  // Adds all constraints currently in `other` to this variable.
  VariableType& MergeFrom(const VariableType& other);

  // AddCustomConstraint()
  //
  // Adds a constraint that is fully user-defined. This version of
  // `AddCustomConstraint` is agnostic of other variables. (E.g., prime,
  // positive, etc).
  VariableType& AddCustomConstraint(
      std::string_view name, std::function<bool(const ValueType&)> checker);

  // AddCustomConstraint()
  //
  // Adds a constraint that is fully user-defined. This version of
  // `AddCustomConstraint` depends on the value of other variables. (E.g., at
  // least X, larger than the average of the elements in A, etc).
  VariableType& AddCustomConstraint(
      std::string_view name, std::vector<std::string> dependencies,
      std::function<bool(librarian::AnalysisContext, const ValueType&)>
          checker);

  // AddCustomConstraint()
  //
  // Adds a constraint that is fully user-defined. See other overloads of
  // `AddCustomConstraint` for simple construction.
  VariableType& AddCustomConstraint(CustomConstraint<ValueType> constraint);

  // ListEdgeCases()
  //
  // Returns a list of MVariables that should give common difficult cases. Some
  // examples of this type of cases include common pitfalls for your data type
  // (edge cases, queries of death, etc).
  [[nodiscard]] std::vector<VariableType> ListEdgeCases(
      AnalysisContext ctx) const;

  // Print()
  //
  // Prints `value` into `ctx` using any formatting provided to this
  // variable (as I/O constraints).
  void Print(PrinterContext ctx, const ValueType& value) const {
    PrintImpl(ctx, value);
  }

  // Read()
  //
  // Reads a value from `ctx` using any formatting provided to this variable
  // (as I/O constraints).
  [[nodiscard]] ValueType Read(ReaderContext ctx) const {
    return ReadImpl(ctx);
  }

  // GetUniqueValue()
  //
  // Determines if there is exactly one value that this variable can be
  // assigned to. If so, return that value. If there is not a unique value
  // (or it is too hard to determine that there is a unique value), returns
  // `std::nullopt`. Returning `std::nullopt` does not guarantee there is
  // not a unique value.
  [[nodiscard]] std::optional<ValueType> GetUniqueValue(
      AnalysisContext ctx) const {
    try {
      // FIXME: Placeholder until Context refactor is done.
      std::optional<ValueType> known =
          ctx.GetValueIfKnown<VariableType>(ctx.GetVariableName());
      if (known) return *known;

      return GetUniqueValueImpl(ctx);
    } catch (const VariableNotFound& e) {
      // Explicitly re-throw an unknown variable. If it's unknown, we will
      // never be able to do anything with it.
      throw;
    } catch (...) {  // FIXME: Only catch Moriarty errors.
      return std::nullopt;
    }
  }

  absl::Status IsSatisfiedWith(AnalysisContext ctx,
                               const ValueType& value) const;
  [[nodiscard]] std::string Explanation(AnalysisContext ctx,
                                        const ValueType& value) const;

  [[nodiscard]] ValueType Generate(ResolverContext ctx) const;

  // GetDependencies()
  //
  // Returns the list of names of the variables that this variable depends
  // on, this includes direct dependencies and the recursive child
  // dependencies.
  [[nodiscard]] std::vector<std::string> GetDependencies() const override;

  // MergeFromAnonymous()
  //
  // Same as `MergeFrom`, but you do not know the type of the variable you
  // are. This should only be used in generic code.
  absl::Status MergeFromAnonymous(
      const moriarty_internal::AbstractVariable& other) {
    MORIARTY_ASSIGN_OR_RETURN(
        VariableType typed_other,
        moriarty_internal::ConvertTo<VariableType>(&other, "MergeFrom"));
    MergeFrom(typed_other);
    return absl::OkStatus();
  }

  // FIXME: Make this the implementation of ToString()
  friend std::ostream& operator<<(std::ostream& os, const VariableType& var) {
    return os << var.constraints_.ToString();
  }

 protected:
  // ---------------------------------------------------------------------------
  //  Virtual extension points
  virtual ValueType GenerateImpl(ResolverContext ctx) const = 0;
  virtual std::optional<ValueType> GetUniqueValueImpl(
      AnalysisContext ctx) const;
  virtual ValueType ReadImpl(ReaderContext ctx) const;
  virtual void PrintImpl(PrinterContext ctx, const ValueType& value) const;
  virtual std::vector<VariableType> ListEdgeCasesImpl(
      AnalysisContext ctx) const;
  // ---------------------------------------------------------------------------

  // To be removed
  virtual std::vector<std::string> GetDependenciesImpl() const;
  virtual absl::Status IsSatisfiedWithImpl(AnalysisContext ctx,
                                           const ValueType& value) const {
    return absl::UnimplementedError("IsSatisfiedWith");
  }
  virtual absl::Status MergeFromImpl(const VariableType& other) {
    return absl::UnimplementedError("MergeFrom");
  }

  // InternalAddConstraint() [For Librarians]
  //
  // Adds a constraint to this variable. (E.g., Positive(), Even(), Length(5)).
  // The constraint `c` must be associated with the corresponding MVariable.
  // Librarians must use this to register constraints with the variable. All
  // calls to AddConstraint() in the MVariable should call this function.
  template <typename Constraint>
  VariableType& InternalAddConstraint(Constraint c);

 private:
  ConstraintHandler<VariableType, ValueType> constraints_;

  // `custom_constraints_deps_` is a list of the needed variables for all
  // the custom constraints defined for this variable.
  std::vector<std::string> custom_constraints_deps_;

  // Helper function that casts *this to `VariableType`.
  [[nodiscard]] VariableType& UnderlyingVariableType();
  [[nodiscard]] const VariableType& UnderlyingVariableType() const;
  [[nodiscard]] ValueType GenerateOnce(ResolverContext ctx) const;

  // ---------------------------------------------------------------------------
  //  AbstractVariable overrides
  [[nodiscard]] std::unique_ptr<moriarty_internal::AbstractVariable> Clone()
      const override;
  absl::Status AssignValue(ResolverContext ctx) const override;
  absl::Status AssignUniqueValue(AssignmentContext ctx) const override;
  absl::Status ValueSatisfiesConstraints(AnalysisContext ctx) const override;
  absl::Status MergeFrom(
      const moriarty_internal::AbstractVariable& other) override;
  absl::Status ReadValue(
      ReaderContext ctx,
      moriarty_internal::MutableValuesContext values_ctx) const override;
  absl::Status PrintValue(librarian::PrinterContext ctx) const override;
  std::vector<std::unique_ptr<moriarty_internal::AbstractVariable>>
  ListAnonymousEdgeCases(AnalysisContext ctx) const override;
  // ---------------------------------------------------------------------------

  struct CustomConstraintWrapper {
   public:
    explicit CustomConstraintWrapper(CustomConstraint<ValueType> constraint)
        : constraint_(std::move(constraint)) {};
    bool IsSatisfiedWith(AnalysisContext ctx, const ValueType& value) const {
      return constraint_.IsSatisfiedWith(ctx, value);
    }
    std::string Explanation(AnalysisContext ctx, const ValueType& value) const {
      return constraint_.Explanation(value);
    }
    std::string ToString() const { return constraint_.ToString(); }
    void ApplyTo(MVariable& other) const {
      other.AddCustomConstraint(constraint_);
    }

   private:
    CustomConstraint<ValueType> constraint_;
  };
};

// -----------------------------------------------------------------------------
//  Template implementation below

// In the template implementations below, we use `MVariable<V, G>` instead of
// `MVariable<VariableType, ValueType>` for readability.
//
// V = VariableType
// G = ValueType  (G since it was previously `GeneratedType`).

// -----------------------------------------------------------------------------
//  Template implementation for public functions

template <typename V, typename G>
std::string MVariable<V, G>::ToString() const {
  std::stringstream ss;
  ss << UnderlyingVariableType();
  return ss.str();
}

template <typename V, typename G>
V& MVariable<V, G>::MergeFrom(const V& other) {
  if (auto status = MergeFromImpl(other);
      !status.ok() && !absl::IsUnimplemented(status)) {
    throw std::runtime_error(status.ToString());
  }
  other.constraints_.ApplyAllTo(UnderlyingVariableType());

  return UnderlyingVariableType();
}

template <typename V, typename G>
V& MVariable<V, G>::AddCustomConstraint(std::string_view name,
                                        std::function<bool(const G&)> checker) {
  return AddCustomConstraint(CustomConstraint<G>(name, std::move(checker)));
}

template <typename V, typename G>
V& MVariable<V, G>::AddCustomConstraint(
    std::string_view name, std::vector<std::string> dependencies,
    std::function<bool(librarian::AnalysisContext, const G&)> checker) {
  return AddCustomConstraint(
      CustomConstraint<G>(name, std::move(dependencies), std::move(checker)));
}

template <typename V, typename G>
V& MVariable<V, G>::AddCustomConstraint(CustomConstraint<G> constraint) {
  std::vector<std::string> deps = constraint.GetDependencies();
  custom_constraints_deps_.insert(custom_constraints_deps_.end(),
                                  std::make_move_iterator(deps.begin()),
                                  std::make_move_iterator(deps.end()));
  // Sort the dependencies so the order of the generation is consistent.
  absl::c_sort(custom_constraints_deps_);
  return InternalAddConstraint(CustomConstraintWrapper(std::move(constraint)));
}

template <typename V, typename G>
std::vector<V> MVariable<V, G>::ListEdgeCases(AnalysisContext ctx) const {
  std::vector<V> instances = ListEdgeCasesImpl(ctx);
  for (V& instance : instances) {
    // TODO(hivini): When merging with a fixed value variable that has the same
    // value a difficult instance, this does not return an error. Determine if
    // this matters or not.
    instance.MergeFrom(UnderlyingVariableType());
  }
  return instances;
}

// -----------------------------------------------------------------------------
//  Protected functions' implementations

template <typename V, typename G>
G MVariable<V, G>::ReadImpl(ReaderContext ctx) const {
  throw std::runtime_error(
      absl::StrCat("Read() not implemented for ", Typename()));
}

template <typename V, typename G>
void MVariable<V, G>::PrintImpl(PrinterContext ctx, const G& value) const {
  throw std::runtime_error(
      absl::StrCat("Print() not implemented for ", Typename()));
}

template <typename V, typename G>
std::vector<std::string> MVariable<V, G>::GetDependenciesImpl() const {
  return std::vector<std::string>();  // By default, return an empty list.
}

template <typename V, typename G>
std::vector<V> MVariable<V, G>::ListEdgeCasesImpl(AnalysisContext ctx) const {
  return std::vector<V>();  // By default, return an empty list.
}

template <typename V, typename G>
std::optional<G> MVariable<V, G>::GetUniqueValueImpl(
    AnalysisContext ctx) const {
  return std::nullopt;  // By default, return no unique value.
}

template <typename V, typename G>
template <typename Constraint>
V& MVariable<V, G>::InternalAddConstraint(Constraint c) {
  constraints_.AddConstraint(std::move(c));
  return UnderlyingVariableType();
}

// -----------------------------------------------------------------------------
//  Template implementation for the Internal Extended API

template <typename V, typename G>
G MVariable<V, G>::Generate(ResolverContext ctx) const {
  std::string name = ctx.GetVariableName();

  if (auto value = ctx.GetValueIfKnown<V>(name); value.has_value())
    return *value;

  ctx.MarkStartGeneration(name);
  using Policy =
      moriarty_internal::GenerationConfig::RetryRecommendation::Policy;

  std::exception_ptr last_exception;
  while (true) {
    try {
      G value = GenerateOnce(ctx);
      ctx.MarkSuccessfulGeneration(name);
      return value;
    } catch (const std::exception& e) {
      last_exception = std::current_exception();
      auto [should_retry, delete_variables] = ctx.AddGenerationFailure(name);
      for (std::string_view variable_name : delete_variables)
        ctx.EraseValue(variable_name);

      if (should_retry == Policy::kAbort) break;
    }
  }

  ctx.MarkAbandonedGeneration(name);

  try {
    std::rethrow_exception(last_exception);
  } catch (const std::exception& e) {
    throw std::runtime_error(
        std::format("Error generating '{}' (even with retries). Last error: {}",
                    name, e.what()));
  }

  throw std::runtime_error(std::format(
      "Error generating '{}' (even with retries). Last error: unknown", name));
}

template <typename V, typename G>
absl::Status MVariable<V, G>::IsSatisfiedWith(AnalysisContext ctx,
                                              const G& value) const {
  if (!constraints_.IsSatisfiedWith(ctx, value)) {
    std::string reason = constraints_.Explanation(ctx, value);
    return UnsatisfiedConstraintError(reason);
  }

  // FIXME: Determine semantics of which errors are propagated.
  if (auto status = IsSatisfiedWithImpl(ctx, value);
      !status.ok() && !absl::IsUnimplemented(status)) {
    return status;
  }

  return absl::OkStatus();
}

template <typename V, typename G>
std::string MVariable<V, G>::Explanation(AnalysisContext ctx,
                                         const G& value) const {
  auto status = IsSatisfiedWith(ctx, value);
  if (status.ok()) throw std::invalid_argument("Value satisfies constraints.");
  return std::string(status.message());
}

template <typename V, typename G>
absl::Status MVariable<V, G>::MergeFrom(
    const moriarty_internal::AbstractVariable& other) {
  const V* const other_derived_class = dynamic_cast<const V* const>(&other);

  if (other_derived_class == nullptr)
    return absl::InvalidArgumentError(
        "In MergeFrom: Cannot convert variable to this variable type.");

  MergeFrom(*other_derived_class);
  return absl::OkStatus();
}

// -----------------------------------------------------------------------------
//  Template implementation for private functions not part of Extended API.

template <typename V, typename G>
V& MVariable<V, G>::UnderlyingVariableType() {
  return static_cast<V&>(*this);
}

template <typename V, typename G>
const V& MVariable<V, G>::UnderlyingVariableType() const {
  return static_cast<const V&>(*this);
}

template <typename V, typename G>
std::unique_ptr<moriarty_internal::AbstractVariable> MVariable<V, G>::Clone()
    const {
  // Use V's copy constructor.
  return std::make_unique<V>(UnderlyingVariableType());
}

template <typename V, typename G>
G MVariable<V, G>::GenerateOnce(ResolverContext ctx) const {
  G potential_value = GenerateImpl(ctx);

  // Generate dependent variables used in custom constraints.
  for (std::string_view dep : custom_constraints_deps_) ctx.AssignVariable(dep);

  absl::Status satisfies = IsSatisfiedWith(ctx, potential_value);
  if (!satisfies.ok())
    throw std::runtime_error(std::string(satisfies.message()));

  return potential_value;
}

template <typename V, typename G>
absl::Status MVariable<V, G>::AssignValue(ResolverContext ctx) const {
  if (ctx.ValueIsKnown(ctx.GetVariableName())) return absl::OkStatus();

  G value = Generate(ctx);
  ctx.SetValue<V>(ctx.GetVariableName(), value);
  return absl::OkStatus();
}

template <typename V, typename G>
absl::Status MVariable<V, G>::AssignUniqueValue(AssignmentContext ctx) const {
  if (ctx.ValueIsKnown(ctx.GetVariableName())) return absl::OkStatus();

  std::optional<G> value = GetUniqueValue(ctx);

  if (!value) return absl::OkStatus();
  ctx.SetValue<V>(ctx.GetVariableName(), *value);
  return absl::OkStatus();
}

template <typename V, typename G>
absl::Status MVariable<V, G>::ValueSatisfiesConstraints(
    AnalysisContext ctx) const {
  return IsSatisfiedWith(ctx, ctx.GetValue<V>(ctx.GetVariableName()));
}

template <typename V, typename G>
absl::Status MVariable<V, G>::ReadValue(
    ReaderContext ctx,
    moriarty_internal::MutableValuesContext values_ctx) const {
  values_ctx.SetValue<V>(ctx.GetVariableName(), Read(ctx));
  return absl::OkStatus();
}

template <typename V, typename G>
absl::Status MVariable<V, G>::PrintValue(PrinterContext ctx) const {
  Print(ctx, ctx.GetValue<V>(ctx.GetVariableName()));
  return absl::OkStatus();
}

template <typename V, typename G>
std::vector<std::string> MVariable<V, G>::GetDependencies() const {
  std::vector<std::string> this_deps = GetDependenciesImpl();
  absl::c_copy(custom_constraints_deps_, std::back_inserter(this_deps));
  return this_deps;
}

template <typename V, typename G>
std::vector<std::unique_ptr<moriarty_internal::AbstractVariable>>
MVariable<V, G>::ListAnonymousEdgeCases(AnalysisContext ctx) const {
  std::vector<V> instances = ListEdgeCases(ctx);
  std::vector<std::unique_ptr<moriarty_internal::AbstractVariable>> new_vec;
  new_vec.reserve(instances.size());
  for (V& instance : instances) {
    new_vec.push_back(std::make_unique<V>(std::move(instance)));
  }
  return new_vec;
}

}  // namespace librarian
}  // namespace moriarty

namespace moriarty::moriarty_internal {
// Convenience functions for internal use only.

template <typename V, typename G>
[[nodiscard]] std::optional<G> GetUniqueValue(
    const librarian::MVariable<V, G>& variable, std::string_view variable_name,
    moriarty::moriarty_internal::ViewOnlyContext ctx) {
  return variable.GetUniqueValue(
      librarian::AnalysisContext(variable_name, ctx));
}

template <typename V, typename G>
absl::Status SatisfiesConstraints(
    const librarian::MVariable<V, G>& variable, std::string_view variable_name,
    moriarty::moriarty_internal::ViewOnlyContext ctx, const G& value) {
  return variable.IsSatisfiedWith(
      librarian::AnalysisContext(variable_name, ctx), value);
}

template <typename... T>
  requires std::constructible_from<librarian::PrinterContext, T...>
absl::Status PrintValue(const AbstractVariable& variable, T... args) {
  return variable.PrintValue(librarian::PrinterContext(args...));
}

}  // namespace moriarty::moriarty_internal

#endif  // MORIARTY_SRC_LIBRARIAN_MVARIABLE_H_
