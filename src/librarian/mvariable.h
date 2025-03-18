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
#include <functional>
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
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/substitute.h"
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
#include "src/internal/status_utils.h"
#include "src/librarian/constraint_handler.h"
#include "src/util/status_macro/status_macros.h"

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

  // Is()
  //
  // Declares that this variable must be exactly `value`. For example,
  //
  //   M.AddVariable("N", MInteger().Is(5));
  //
  // states that this variable must be 5. Logically equivalent to
  // `IsOneOf({value});`
  VariableType& Is(ValueType value);

  // IsOneOf()
  //
  // Declares that this variable must be exactly one of the elements in
  // `values`. For example,
  //
  //   M.AddVariable("N", MInteger().IsOneOf({5, 10, 15}));
  //   M.AddVariable("S", MString().IsOneOf({"POSSIBLE", "IMPOSSIBLE"});
  //
  // states that the variable "N" must be either 5, 10, or 15, and that the
  // variable "S" must be either the string "POSSIBLE" or "IMPOSSIBLE".
  VariableType& IsOneOf(std::vector<ValueType> values);

  // MergeFrom()
  //
  // Merges my current constraints with the constraints of the `other` variable.
  // The merge should act as an intersection of the two constraints. If one says
  // 1 <= x <= 10 and the other says 5 <= x <= 20, then then merged version
  // should have 5 <= x <= 10.
  //
  // Crashes on failure. See `TryMergeFrom()` for non-crashing version.
  VariableType& MergeFrom(const VariableType& other);

  // TryMergeFrom()
  //
  // Merges my current constraints with the constraints of the `other` variable.
  // The merge should act as an intersection of the two constraints. If one says
  // 1 <= x <= 10 and the other says 5 <= x <= 20, then then merged version
  // should have 5 <= x <= 10.
  //
  // Returns status on failure. See `MergeFrom()` for builder-like version.
  absl::Status TryMergeFrom(const VariableType& other);

  // AddConstraint() // FIXME: Change to AddConstraint
  //
  // Adds a constraint to this variable. (E.g., Positive(), Even(), Length(5)).
  // The constraint `c` must be associated with the corresponding MVariable.
  template <typename Constraint>
  VariableType& NewAddConstraint(Constraint c) {
    constraints_.AddConstraint(std::move(c));
    return UnderlyingVariableType();
  }

  // AddCustomConstraint()
  //
  // Adds a constraint to this variable. `checker` must take exactly one
  // parameter of type `ValueType`. `checker` should return true if the value
  // satisfies this constraint.
  VariableType& AddCustomConstraint(
      std::string_view constraint_name,
      std::function<bool(const ValueType&)> checker);

  // AddCustomConstraint()
  // FIXME: Fix comments
  //
  // Adds a constraint to this variable. `checker` must take exactly two
  // parameters of type `ValueType` and `ConstraintValues`. `checker` should
  // return true if the value satisfies this constraint.
  //
  // `deps` is a list of variables that this constraint depends on. If a
  // variable's name is in this list, you may use ConstraintValues::GetValue<>()
  // to access that variable's value.
  VariableType& AddCustomConstraint(
      std::string_view constraint_name, std::vector<std::string> deps,
      std::function<bool(const ValueType&, AnalysisContext ctx)> checker);

  // ListEdgeCases()
  //
  // Returns a list of MVariables that where merged with your variable. This
  // should give common difficult cases. Some examples of this type of cases
  // include common pitfalls for your data type (edge cases, queries of death,
  // etc).
  [[nodiscard]] std::vector<VariableType> ListEdgeCases(
      AnalysisContext ctx) const;

  // Print()
  //
  // Prints `value` into `ctx` using any formatting provided to this variable
  // (as I/O constraints).
  void Print(PrinterContext ctx, const ValueType& value) const {
    PrintImpl(ctx, value);
  }

  // Read()
  //
  // Reads a value from `ctx` using any formatting provided to this variable (as
  // I/O constraints).
  [[nodiscard]] ValueType Read(ReaderContext ctx) const {
    return ReadImpl(ctx);
  }

  // GetUniqueValue()
  //
  // Determines if there is exactly one value that this variable can be assigned
  // to. If so, return that value. If there is not a unique value (or it is too
  // hard to determine that there is a unique value), returns `std::nullopt`.
  // Returning `std::nullopt` does not guarantee there is not a unique value,
  // just that it is too hard to determine it.
  [[nodiscard]] std::optional<ValueType> GetUniqueValue(
      AnalysisContext ctx) const {
    try {
      // FIXME: Placeholder until Context refactor is done.
      std::optional<ValueType> known =
          ctx.GetValueIfKnown<VariableType>(ctx.GetVariableName());
      if (known) return *known;

      if (!overall_status_.ok()) return std::nullopt;
      if (is_one_of_) {
        // TODO: Check if all constraints are satisfied?
        if (is_one_of_->size() == 1) return is_one_of_->at(0);
        return std::nullopt;  // Not sure which one is correct.
      }

      return GetUniqueValueImpl(ctx);
    } catch (const VariableNotFound& e) {
      // Explicitly re-throw an unknown variable. If it's unknown, we will never
      // be able to do anything with it.
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
  // Returns the list of names of the variables that this variable depends on,
  // this includes direct dependencies and the recursive child dependencies.
  [[nodiscard]] std::vector<std::string> GetDependencies() const override;

  // MergeFromAnonymous()
  //
  // Same as `MergeFrom`, but you do not know the type of the variable you are.
  // This should only be used in generic code.
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
  //  Functions that need to be overridden by all `MVariable`s.

  // GenerateImpl() [virtual]
  //
  // Users should not call this directly. Call `Random(MVariable)` in the
  // specific Moriarty component (Generator, Importer, etc).
  //
  // Returns a random value based on all current constraints on this MVariable.
  //
  // This function may be called several times and should not update the
  // variable's internal state.
  //
  // TODO(darcybest): Consider if this function could/should be `const`.
  // Librarians are strongly encouraged to treat this function as if it were
  // const so you are future compatible.
  //
  // GenerateImpl() will only be called if Is()/IsOneOf() have not been called.
  virtual ValueType GenerateImpl(ResolverContext ctx) const = 0;

  // IsSatisfiedWithImpl() [virtual]
  //
  // Users should not call this directly. Call `IsSatisfiedWith()` instead.
  //
  // Determines if `value` satisfies all of the constraints specified by this
  // variable.
  //
  // FIXME: Fix this comment.
  // This function should return an `UnsatisfiedConstraintError()`. Almost every
  // other status returned will be treated as an internal error. The exceptions
  // are `VariableNotFound` and `ValueNotFound`, which will be
  // converted into an `UnsatisfiedConstraintError` before returning to the
  // user. You may find the `CheckConstraint()` helpers useful.
  virtual absl::Status IsSatisfiedWithImpl(AnalysisContext ctx,
                                           const ValueType& value) const = 0;

  // MergeFromImpl() [virtual]
  //
  // Users should not call this directly. Call `MergeFrom()` instead.
  //
  // Merges my current constraints with the constraints of the `other` variable.
  // The merge should act as an intersection of the two constraints. If one says
  // 1 <= x <= 10 and the other says 5 <= x <= 20, then then merged version
  // should have 5 <= x <= 10.
  virtual absl::Status MergeFromImpl(const VariableType& other) = 0;

  // ReadImpl() [virtual/optional]
  //
  // Users should not call this directly. Call `Read()` instead.
  //
  // Reads a value from `ctx` using any formatting provided to this variable.
  virtual ValueType ReadImpl(ReaderContext ctx) const;

  // PrintImpl() [virtual/optional]
  //
  // Users should not call this directly. Call `Print()` instead.
  //
  // Print `value` into `ctx` using any configuration provided by your
  // VariableType (whitespace separators, etc).
  virtual void PrintImpl(PrinterContext ctx, const ValueType& value) const;

  // GetDependenciesImpl() [virtual/optional]
  //
  // Users should not call this directly. Call `GetDependencies()` instead.
  //
  // Returns a list of the variable names that this variable depends on.
  //
  // Default: No dependencies.
  virtual std::vector<std::string> GetDependenciesImpl() const;

  // ListEdgeCasesImpl() [virtual/optional]
  //
  // Users should not call this directly. Call `ListEdgeCases()`
  // instead.
  //
  // Returns a list of complicated instances to merge from.
  // TODO(hivini): Improve comment with examples.
  virtual std::vector<VariableType> ListEdgeCasesImpl(
      AnalysisContext ctx) const;

  // GetUniqueValueImpl() [virtual/optional]
  //
  // Users should not call this directly. Call `GetUniqueValue()` instead.
  //
  // Determines if there is exactly one value that this variable can be assigned
  // to. If so, return that value. If there is not a unique value (or it is too
  // hard to determine that there is a unique value), returns `std::nullopt`.
  // Returning `std::nullopt` does not guarantee there is not a unique value,
  // just that it is too hard to determine it.
  //
  // By default, this returns `std::nullopt`.
  virtual std::optional<ValueType> GetUniqueValueImpl(
      AnalysisContext ctx) const;

  // DeclareSelfAsInvalid() [Helper for Librarians]
  //
  // Sets the status of this variable to `status`. This must be a non-ok
  // Moriarty Status. Almost surely an `UnsatisfiedConstraintError()`. All
  // future calls to FooImpl() that return an absl::Status will be intercepted
  // and return this instead. (E.g., GenerateImpl(), PrintImpl(), etc.)
  //
  // FIXME: PrintImpl (and others with Context) ignores this flag.
  void DeclareSelfAsInvalid(absl::Status status);

 private:
  ConstraintHandler<VariableType, ValueType> constraints_;

  // `is_one_of_` is a list of values that Generate() should produce. If the
  // optional is set and the list is empty, then there are no viable values.
  std::optional<std::vector<ValueType>> is_one_of_;

  // `custom_constraints_deps_` is a list of the needed variables for all the
  // custom constraints defined for this variable.
  std::vector<std::string> custom_constraints_deps_;

  // `custom_constraints_` is a list of constraints, in the form of functions,
  // to check when `SatisfiesConstraints()` is called.
  struct CustomConstraint {
    std::string name;
    std::function<bool(const ValueType&, AnalysisContext)> checker;
  };
  std::vector<CustomConstraint> custom_constraints_;

  // The overall status of this variable. If this is not ok, then all FooImpl()
  // that return an absl::Status will be intercepted and return this instead.
  // (E.g., GenerateImpl(), PrintImpl(), etc.)
  absl::Status overall_status_ = absl::OkStatus();

  // Helper function that casts *this to `VariableType`.
  [[nodiscard]] VariableType& UnderlyingVariableType();
  [[nodiscard]] const VariableType& UnderlyingVariableType() const;

  // Clones the object, similar to a copy-constructor.
  [[nodiscard]] std::unique_ptr<moriarty_internal::AbstractVariable> Clone()
      const override;

  // Try to generate exactly once, without any retries.
  ValueType GenerateOnce(ResolverContext ctx) const;

  // AssignValue()
  //
  // Assigns an explicit value to this variable. The value is set into `ctx` and
  // `ctx` gives all necessary information to set the value. If `ctx` already
  // contains a value for this variable, this is a no-op.
  absl::Status AssignValue(ResolverContext ctx) const override;

  // AssignUniqueValue()
  //
  // Determines if there is exactly one value that this variable can be
  // assigned to. If so, assigns that value.
  //
  // If there is not a unique value (or it is too hard to determine that there
  // is a unique value), this does nothing.
  //
  // Example: MInteger().Between(7, 7) might be able to determine that its
  // unique value is 7.
  absl::Status AssignUniqueValue(AssignmentContext ctx) const override;

  // ValueSatisfiesConstraints()
  //
  // Determines if the value stored in `ctx` satisfies all constraints for this
  // variable.
  //
  // If a variable does not have a value, this will return not ok.
  // If a value does not have a variable, this will return ok.
  absl::Status ValueSatisfiesConstraints(AnalysisContext ctx) const override;

  // MergeFrom()
  //
  // Merges my current constraints with the constraints of the `other`
  // variable. The merge should act as an intersection of the two constraints.
  // If one says 1 <= x <= 10 and the other says 5 <= x <= 20, then then
  // merged version should have 5 <= x <= 10.
  absl::Status MergeFrom(
      const moriarty_internal::AbstractVariable& other) override;

  // ReadValue()
  //
  // Reads a value from `ctx` using the constraints of this variable to
  // determine formatting, etc.
  absl::Status ReadValue(
      ReaderContext ctx,
      moriarty_internal::MutableValuesContext values_ctx) const override;

  // PrintValue()
  //
  // Prints the value of this variable to `ctx` using the constraints on this
  // variable to determine formatting, etc.
  absl::Status PrintValue(librarian::PrinterContext ctx) const override;

  // ListAnonymousEdgeCases()
  //
  // Returns the list of difficult abstract variables defined by the
  // implementation of this abstract variable.
  std::vector<std::unique_ptr<moriarty_internal::AbstractVariable>>
  ListAnonymousEdgeCases(AnalysisContext ctx) const override;
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
V& MVariable<V, G>::Is(G value) {
  return IsOneOf({std::move(value)});
}

template <typename V, typename G>
V& MVariable<V, G>::IsOneOf(std::vector<G> values) {
  std::sort(values.begin(), values.end());

  if (!is_one_of_) {
    is_one_of_ = std::move(values);
    return UnderlyingVariableType();
  }

  is_one_of_->erase(
      std::set_intersection(std::begin(values), std::end(values),
                            std::begin(*is_one_of_), std::end(*is_one_of_),
                            std::begin(*is_one_of_)),
      std::end(*is_one_of_));
  return UnderlyingVariableType();
}

template <typename V, typename G>
V& MVariable<V, G>::MergeFrom(const V& other) {
  moriarty_internal::TryFunctionOrCrash(
      [this, &other]() { return this->TryMergeFrom(other); }, "MergeFrom");
  return UnderlyingVariableType();
}

template <typename V, typename G>
absl::Status MVariable<V, G>::TryMergeFrom(const V& other) {
  MORIARTY_RETURN_IF_ERROR(overall_status_);
  if (!other.overall_status_.ok()) overall_status_ = other.overall_status_;
  MORIARTY_RETURN_IF_ERROR(overall_status_);

  if (other.is_one_of_) IsOneOf(*other.is_one_of_);
  MORIARTY_RETURN_IF_ERROR(MergeFromImpl(other));

  other.constraints_.ApplyAllTo(UnderlyingVariableType());

  // The merge may have caused a new error to occur.
  return overall_status_;
}

template <typename V, typename G>
V& MVariable<V, G>::AddCustomConstraint(std::string_view constraint_name,
                                        std::function<bool(const G&)> checker) {
  return AddCustomConstraint(
      constraint_name, {},
      [checker](const G& v, AnalysisContext ctx) { return checker(v); });
}

template <typename V, typename G>
V& MVariable<V, G>::AddCustomConstraint(
    std::string_view constraint_name, std::vector<std::string> deps,
    std::function<bool(const G&, AnalysisContext ctx)> checker) {
  CustomConstraint c;
  c.name = constraint_name;
  c.checker = checker;
  custom_constraints_deps_.insert(custom_constraints_deps_.end(),
                                  std::make_move_iterator(deps.begin()),
                                  std::make_move_iterator(deps.end()));
  // Sort the dependencies so the order of the generation is consistent.
  absl::c_sort(custom_constraints_deps_);
  custom_constraints_.push_back(c);
  return UnderlyingVariableType();
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
void MVariable<V, G>::DeclareSelfAsInvalid(absl::Status status) {
  ABSL_CHECK(!status.ok());
  ABSL_CHECK(IsMoriartyError(status));
  overall_status_ = std::move(status);
}

// -----------------------------------------------------------------------------
//  Template implementation for the Internal Extended API

template <typename V, typename G>
G MVariable<V, G>::Generate(ResolverContext ctx) const {
  std::string name = ctx.GetVariableName();

  if (auto value = ctx.GetValueIfKnown<V>(name); value.has_value())
    return *value;

  if (is_one_of_ && is_one_of_->empty())
    throw std::runtime_error("Is/IsOneOf used, but no viable value found.");

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
  MORIARTY_RETURN_IF_ERROR(overall_status_);
  if (is_one_of_) {
    MORIARTY_RETURN_IF_ERROR(CheckConstraint(
        absl::c_binary_search(*is_one_of_, value),
        "`value` must be one of the options in Is() and IsOneOf()"));
  }

  if (!constraints_.IsSatisfiedWith(ctx, value)) {
    std::string reason = constraints_.Explanation(ctx, value);
    return UnsatisfiedConstraintError(reason);
  }

  // FIXME: Determine semantics of which errors are propagated.
  absl::Status status = IsSatisfiedWithImpl(ctx, value);
  if (!status.ok()) return status;

  for (const auto& [checker_name, checker] : custom_constraints_) {
    MORIARTY_RETURN_IF_ERROR(CheckConstraint(
        checker(value, ctx),
        absl::Substitute("Custom constraint '$0' not satisfied.",
                         checker_name)));
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

  return TryMergeFrom(*other_derived_class);
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
  G potential_value =
      is_one_of_ ? ctx.RandomElement(*is_one_of_) : GenerateImpl(ctx);

  // Generate dependent variables used in custom constraints.
  for (std::string_view dep : custom_constraints_deps_) ctx.AssignVariable(dep);

  absl::Status satisfies = IsSatisfiedWith(ctx, potential_value);
  if (!satisfies.ok())
    throw std::runtime_error(std::string(satisfies.message()));

  return potential_value;
}

template <typename V, typename G>
absl::Status MVariable<V, G>::AssignValue(ResolverContext ctx) const {
  MORIARTY_RETURN_IF_ERROR(overall_status_);

  if (ctx.ValueIsKnown(ctx.GetVariableName())) return absl::OkStatus();

  G value = Generate(ctx);
  ctx.SetValue<V>(ctx.GetVariableName(), value);
  return absl::OkStatus();
}

template <typename V, typename G>
absl::Status MVariable<V, G>::AssignUniqueValue(AssignmentContext ctx) const {
  MORIARTY_RETURN_IF_ERROR(overall_status_);

  if (ctx.ValueIsKnown(ctx.GetVariableName())) return absl::OkStatus();

  std::optional<G> value = GetUniqueValue(ctx);

  if (!value) return absl::OkStatus();
  ctx.SetValue<V>(ctx.GetVariableName(), *value);
  return absl::OkStatus();
}

template <typename V, typename G>
absl::Status MVariable<V, G>::ValueSatisfiesConstraints(
    AnalysisContext ctx) const {
  MORIARTY_RETURN_IF_ERROR(overall_status_);
  return IsSatisfiedWith(ctx, ctx.GetValue<V>(ctx.GetVariableName()));
}

template <typename V, typename G>
absl::Status MVariable<V, G>::ReadValue(
    ReaderContext ctx,
    moriarty_internal::MutableValuesContext values_ctx) const {
  MORIARTY_RETURN_IF_ERROR(overall_status_);
  values_ctx.SetValue<V>(ctx.GetVariableName(), Read(ctx));
  return absl::OkStatus();
}

template <typename V, typename G>
absl::Status MVariable<V, G>::PrintValue(PrinterContext ctx) const {
  MORIARTY_RETURN_IF_ERROR(overall_status_);
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
