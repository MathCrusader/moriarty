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
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "src/contexts/internal/mutable_values_context.h"
#include "src/contexts/internal/view_only_context.h"
#include "src/contexts/librarian_context.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/variable_set.h"
#include "src/librarian/constraint_handler.h"
#include "src/librarian/conversions.h"
#include "src/librarian/errors.h"
#include "src/librarian/io_config.h"
#include "src/librarian/one_of_handler.h"
#include "src/librarian/policies.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/constraints/custom_constraint.h"

namespace moriarty {

// Determines if ConstraintType is a valid constraint to be applied to
// MVariableType.
template <typename MVariableType, typename ConstraintType>
concept ConstraintFor =
    MoriartyVariable<MVariableType> &&
    std::derived_from<std::decay_t<ConstraintType>, MConstraint> && requires {
      std::declval<MVariableType>().AddConstraint(
          std::declval<ConstraintType>());
    };

namespace librarian {

// MVariable<>
//
// A class that describes a variable type. An MVariable can be thought of as a
// bundle of constraints. These constraints can be used to generate, validate,
// and analyze values of the variable type.
//
// All Moriarty variable types (e.g., MInteger, MString, MGraph, etc) should
// derive from this class using CRTP (curiously recurring template pattern).
//
// * `VariableType` is the underlying Moriarty variable type
// * `ValueType` is the non-Moriarty type that is generated/validated/analyzed
//   by this MVariable.
//
// E.g.,
//     class MInteger : public MVariable<MInteger, int64_t> {}
template <typename VariableType, typename ValueType>
class MVariable : public moriarty_internal::AbstractVariable {
 public:
  static constexpr bool is_moriarty_variable = true;
  using value_type = ValueType;

  ~MVariable() override = default;

 protected:
  // Only derived classes should make copies of me directly to avoid accidental
  // slicing. Call derived class's constructors instead.
  MVariable();
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
  [[nodiscard]] std::string ToString() const override;

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

  // AddConstraint()
  //
  // Adds a constraint that is fully user-defined. See other overloads of
  // `AddCustomConstraint` for simple construction.
  VariableType& AddConstraint(CustomConstraint<VariableType> constraint);

  // CheckValue()
  //
  // Determines if `value` satisfies all constraints on this variable.
  // Returns `std::nullopt` if `value` satisfies all constraints or a string
  // explaining why it does not.
  [[nodiscard]] std::optional<std::string> CheckValue(
      AnalysisContext ctx, const ValueType& value) const;

  // Generate()
  //
  // Returns a random value that satisfies all constraints on this variable.
  [[nodiscard]] ValueType Generate(ResolverContext ctx) const;

  // GetUniqueValue()
  //
  // Determines if there is exactly one value that this variable can be
  // assigned to. If so, return that value. If there is not a unique value
  // (or it is too hard to determine that there is a unique value), returns
  // `std::nullopt`. Returning `std::nullopt` does not guarantee there is
  // not a unique value.
  [[nodiscard]] std::optional<ValueType> GetUniqueValue(
      AnalysisContext ctx) const;

  // Print()
  //
  // Prints `value` into `ctx` using any formatting provided to this
  // variable (as I/O constraints).
  void Print(PrinterContext ctx, const ValueType& value) const;

  // Read()
  //
  // Reads a value from `ctx` using any formatting provided to this variable
  // (as I/O constraints).
  [[nodiscard]] ValueType Read(ReaderContext ctx) const;

  // ListEdgeCases()
  //
  // Returns a list of MVariables that should give common difficult cases. Some
  // examples of this type of cases include common pitfalls for your data type
  // (edge cases, queries of death, etc).
  [[nodiscard]] std::vector<VariableType> ListEdgeCases(
      AnalysisContext ctx) const;

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
  void MergeFromAnonymous(
      const moriarty_internal::AbstractVariable& other) override;

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

  // InternalAddConstraint() [For Librarians]
  //
  // Adds a constraint to this variable. (E.g., Positive(), Even(), Length(5)).
  // The constraint `c` must be associated with the corresponding MVariable.
  // Librarians must use this to register constraints with the variable. All
  // calls to AddConstraint() in the MVariable should call this function.
  template <typename Constraint>
  VariableType& InternalAddConstraint(Constraint c);

  // InternalAddExactlyConstraint() [For Librarians]
  //
  // Adds an `Exactly<ValueType>` constraint to this variable. This will
  // call InternalAddConstraint(), so you don't need to.
  VariableType& InternalAddExactlyConstraint(Exactly<ValueType> c);

  // InternalAddOneOfConstraint() [For Librarians]
  //
  // Adds an `OneOf<ValueType>` constraint to this variable. This will
  // call InternalAddConstraint(), so you don't need to.
  VariableType& InternalAddOneOfConstraint(OneOf<ValueType> c);

  // GetOneOf()
  //
  // Returns the OneOfHandler for this variable.
  [[nodiscard]] OneOfHandler<ValueType>& GetOneOf();

  // GetOneOf()
  //
  // Returns the OneOfHandler for this variable.
  [[nodiscard]] const OneOfHandler<ValueType>& GetOneOf() const;

 private:
  ConstraintHandler<VariableType, ValueType> constraints_;
  OneOfHandler<ValueType> one_of_;
  std::vector<std::string> dependencies_;

  // Helper function that casts *this to `VariableType`.
  [[nodiscard]] VariableType& UnderlyingVariableType();
  [[nodiscard]] const VariableType& UnderlyingVariableType() const;
  [[nodiscard]] ValueType GenerateOnce(ResolverContext ctx) const;

  // ---------------------------------------------------------------------------
  //  AbstractVariable overrides
  [[nodiscard]] std::unique_ptr<moriarty_internal::AbstractVariable> Clone()
      const override;
  void AssignValue(
      std::string_view variable_name,
      std::reference_wrapper<const moriarty_internal::VariableSet> variables,
      std::reference_wrapper<moriarty_internal::ValueSet> values,
      std::reference_wrapper<moriarty_internal::RandomEngine> engine,
      std::reference_wrapper<moriarty_internal::GenerationHandler> handler)
      const override;
  void AssignUniqueValue(
      std::string_view variable_name,
      std::reference_wrapper<const moriarty_internal::VariableSet> variables,
      std::reference_wrapper<moriarty_internal::ValueSet> values)
      const override;
  void ReadValue(
      std::string_view variable_name, std::reference_wrapper<InputCursor> input,
      std::reference_wrapper<const moriarty_internal::VariableSet> variables,
      std::reference_wrapper<moriarty_internal::ValueSet> values)
      const override;
  std::unique_ptr<moriarty_internal::PartialReader> GetPartialReader(
      std::string_view variable_name, int N,
      std::reference_wrapper<InputCursor> input,
      std::reference_wrapper<const moriarty_internal::VariableSet> variables,
      std::reference_wrapper<moriarty_internal::ValueSet> values)
      const override;
  void PrintValue(
      std::string_view variable_name, std::reference_wrapper<std::ostream> os,
      std::reference_wrapper<const moriarty_internal::VariableSet> variables,
      std::reference_wrapper<const moriarty_internal::ValueSet> values)
      const override;
  std::optional<std::string> CheckValue(
      std::string_view variable_name,
      std::reference_wrapper<const moriarty_internal::VariableSet> variables,
      std::reference_wrapper<const moriarty_internal::ValueSet> values)
      const override;
  std::vector<std::unique_ptr<AbstractVariable>> ListAnonymousEdgeCases(
      std::string_view variable_name,
      std::reference_wrapper<const moriarty_internal::VariableSet> variables,
      std::reference_wrapper<const moriarty_internal::ValueSet> values)
      const override;
  // ---------------------------------------------------------------------------

  class CustomConstraintWrapper {
   public:
    explicit CustomConstraintWrapper(CustomConstraint<VariableType> constraint);
    bool IsSatisfiedWith(AnalysisContext ctx, const ValueType& value) const;
    std::string UnsatisfiedReason(AnalysisContext ctx,
                                  const ValueType& value) const;
    std::string ToString() const;
    std::vector<std::string> GetDependencies() const;
    void ApplyTo(VariableType& other) const;

   private:
    CustomConstraint<VariableType> constraint_;
  };

  // Wrapper class around an MVariable's PartialReader. Expected API is:
  //   void ReadNext(ReaderContext, int idx);  // Read the next value
  //   ValueType Finalize();                   // Return the read value
  template <typename ReaderType>
  class PartialReaderWrapper : public moriarty_internal::PartialReader {
   public:
    PartialReaderWrapper(
        ReaderType reader, ReaderContext ctx,
        std::reference_wrapper<moriarty_internal::ValueSet> values)
        : reader_(std::move(reader)), ctx_(std::move(ctx)), values_(values) {}
    void ReadNext() override { reader_.ReadNext(ctx_, idx_++); }
    void Finalize() override {
      values_.get().Set<VariableType>(ctx_.GetVariableName(),
                                      reader_.Finalize());
    }

   private:
    ReaderType reader_;
    int idx_ = 0;
    ReaderContext ctx_;
    std::reference_wrapper<moriarty_internal::ValueSet> values_;
  };
};

// -----------------------------------------------------------------------------
//  Template implementation below

// In the template implementations below, we use `MVariable<V, G>` instead of
// `MVariable<VariableType, ValueType>` for readability.
//
// V = VariableType
// G = ValueType  (G since it was previously `GeneratedType`).

template <typename V, typename G>
MVariable<V, G>::MVariable() {
  // Simple checks to ensure that the template arguments are correct.
  static_assert(std::default_initializable<V>);
  static_assert(std::copyable<V>);
  static_assert(std::derived_from<V, MVariable<V, typename V::value_type>>);
  static_assert(std::same_as<typename V::value_type, G>);
}

// -----------------------------------------------------------------------------
//  Template implementation for public functions

template <typename V, typename G>
std::string MVariable<V, G>::ToString() const {
  return constraints_.ToString();
}

template <typename V, typename G>
V& MVariable<V, G>::MergeFrom(const V& other) {
  other.constraints_.ApplyAllTo(UnderlyingVariableType());
  return UnderlyingVariableType();
}

template <typename V, typename G>
V& MVariable<V, G>::AddCustomConstraint(std::string_view name,
                                        std::function<bool(const G&)> checker) {
  return AddConstraint(CustomConstraint<V>(name, std::move(checker)));
}

template <typename V, typename G>
V& MVariable<V, G>::AddCustomConstraint(
    std::string_view name, std::vector<std::string> dependencies,
    std::function<bool(librarian::AnalysisContext, const G&)> checker) {
  return AddConstraint(
      CustomConstraint<V>(name, std::move(dependencies), std::move(checker)));
}

template <typename V, typename G>
V& MVariable<V, G>::AddConstraint(CustomConstraint<V> constraint) {
  return InternalAddConstraint(CustomConstraintWrapper(std::move(constraint)));
}

template <typename V, typename G>
std::optional<std::string> MVariable<V, G>::CheckValue(AnalysisContext ctx,
                                                       const G& value) const {
  if (constraints_.IsSatisfiedWith(ctx, value)) return std::nullopt;
  return constraints_.UnsatisfiedReason(ctx, value);
}

template <typename V, typename G>
G MVariable<V, G>::Generate(ResolverContext ctx) const {
  std::string name = ctx.GetVariableName();

  if (auto value = ctx.GetValueIfKnown<V>(name); value.has_value())
    return *value;

  ctx.MarkStartGeneration(name);

  auto report_and_clean = [&](std::string_view failure_reason) {
    auto [should_retry, delete_variables] =
        ctx.ReportGenerationFailure(std::string(failure_reason));
    for (std::string_view variable_name : delete_variables)
      ctx.EraseValue(variable_name);
    return should_retry;
  };

  std::exception_ptr last_exception;
  while (true) {
    try {
      G value = GenerateOnce(ctx);
      ctx.MarkSuccessfulGeneration();
      return value;
    } catch (const GenerationError& e) {
      last_exception = std::current_exception();
      RetryPolicy retry = report_and_clean(e.Message());
      if (retry == RetryPolicy::kAbort) break;
      if (e.IsRetryable() == RetryPolicy::kAbort) break;
    } catch (const std::exception& e) {
      report_and_clean(e.what());
      ctx.MarkAbandonedGeneration();
      throw;  // Re-throw unknown messages.
    }
  }

  ctx.MarkAbandonedGeneration();
  std::rethrow_exception(last_exception);
}

template <typename V, typename G>
std::optional<G> MVariable<V, G>::GetUniqueValue(AnalysisContext ctx) const {
  std::optional<G> known = ctx.GetValueIfKnown<V>(ctx.GetVariableName());
  if (known) return known;
  return GetUniqueValueImpl(ctx);
}

template <typename V, typename G>
void MVariable<V, G>::Print(PrinterContext ctx, const G& value) const {
  PrintImpl(ctx, value);
}

template <typename V, typename G>
G MVariable<V, G>::Read(ReaderContext ctx) const {
  G value = ReadImpl(ctx);
  if (auto reason = CheckValue(ctx, value)) {
    ctx.ThrowIOError(
        std::format("Read value does not satisfy constraints: {}", *reason));
  }
  return value;
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

template <typename V, typename G>
std::vector<std::string> MVariable<V, G>::GetDependencies() const {
  return dependencies_;
}

template <typename V, typename G>
void MVariable<V, G>::MergeFromAnonymous(
    const moriarty_internal::AbstractVariable& other) {
  const V& typed_other = ConvertTo<V>(other);
  MergeFrom(typed_other);
}

template <MoriartyVariable VariableType>
std::ostream& operator<<(std::ostream& os, const VariableType& var) {
  return os << var.ToString();
}

// -----------------------------------------------------------------------------
//  Protected functions' implementations

template <typename V, typename G>
G MVariable<V, G>::ReadImpl(ReaderContext ctx) const {
  throw std::runtime_error(
      std::format("Read() not implemented for {}", Typename()));
}

template <typename V, typename G>
void MVariable<V, G>::PrintImpl(PrinterContext ctx, const G& value) const {
  throw std::runtime_error(
      std::format("Print() not implemented for {}", Typename()));
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
  std::vector<std::string> deps = c.GetDependencies();
  dependencies_.insert(dependencies_.end(),
                       std::make_move_iterator(deps.begin()),
                       std::make_move_iterator(deps.end()));
  std::ranges::sort(dependencies_);
  auto end = std::unique(dependencies_.begin(), dependencies_.end());
  dependencies_.erase(end, dependencies_.end());

  constraints_.AddConstraint(std::move(c));
  return UnderlyingVariableType();
}

template <typename V, typename G>
V& MVariable<V, G>::InternalAddExactlyConstraint(Exactly<G> c) {
  if (!one_of_.ConstrainOptions(std::vector<G>{c.GetValue()}))
    throw ImpossibleToSatisfy(ToString(), c.ToString());
  return InternalAddConstraint(std::move(c));
}

template <typename V, typename G>
V& MVariable<V, G>::InternalAddOneOfConstraint(OneOf<G> c) {
  if (!one_of_.ConstrainOptions(c.GetOptions()))
    throw ImpossibleToSatisfy(ToString(), c.ToString());
  return InternalAddConstraint(std::move(c));
}

template <typename V, typename G>
OneOfHandler<G>& MVariable<V, G>::GetOneOf() {
  return one_of_;
}

template <typename V, typename G>
const OneOfHandler<G>& MVariable<V, G>::GetOneOf() const {
  return one_of_;
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

  // Some dependent variables may not have been generate, but are required to
  // validate. Generate them now.
  for (std::string_view dep : dependencies_) ctx.AssignVariable(dep);

  if (auto reason = CheckValue(ctx, potential_value))
    throw moriarty::GenerationError(
        ctx.GetVariableName(),
        std::format("Generated value does not satisfy constraints: {}",
                    *reason),
        RetryPolicy::kRetry);

  return potential_value;
}

template <typename V, typename G>
void MVariable<V, G>::AssignValue(
    std::string_view variable_name,
    std::reference_wrapper<const moriarty_internal::VariableSet> variables,
    std::reference_wrapper<moriarty_internal::ValueSet> values,
    std::reference_wrapper<moriarty_internal::RandomEngine> engine,
    std::reference_wrapper<moriarty_internal::GenerationHandler> handler)
    const {
  ResolverContext ctx(variable_name, variables, values, engine, handler);

  if (ctx.ValueIsKnown(ctx.GetVariableName())) return;

  G value = Generate(ctx);
  ctx.SetValue<V>(ctx.GetVariableName(), value);
}

template <typename V, typename G>
void MVariable<V, G>::AssignUniqueValue(
    std::string_view variable_name,
    std::reference_wrapper<const moriarty_internal::VariableSet> variables,
    std::reference_wrapper<moriarty_internal::ValueSet> values) const {
  AssignmentContext ctx(variable_name, variables, values);
  if (ctx.ValueIsKnown(ctx.GetVariableName())) return;

  std::optional<G> value = GetUniqueValue(ctx);
  if (value) ctx.SetValue<V>(ctx.GetVariableName(), *value);
}

template <typename V, typename G>
void MVariable<V, G>::ReadValue(
    std::string_view variable_name, std::reference_wrapper<InputCursor> input,
    std::reference_wrapper<const moriarty_internal::VariableSet> variables,
    std::reference_wrapper<moriarty_internal::ValueSet> values) const {
  ReaderContext ctx(variable_name, input, variables, values);
  values.get().Set<V>(ctx.GetVariableName(), Read(ctx));
}

template <typename V, typename G>
std::unique_ptr<moriarty_internal::PartialReader>
MVariable<V, G>::GetPartialReader(
    std::string_view variable_name, int N,
    std::reference_wrapper<InputCursor> input,
    std::reference_wrapper<const moriarty_internal::VariableSet> variables,
    std::reference_wrapper<moriarty_internal::ValueSet> values) const {
  if constexpr (requires { typename V::partial_reader_type; }) {
    ReaderContext ctx(variable_name, input, variables, values);
    return std::make_unique<
        PartialReaderWrapper<typename V::partial_reader_type>>(
        UnderlyingVariableType().CreatePartialReader(N), ctx, values);
  } else {
    throw std::runtime_error(
        std::format("PartialReader not implemented for {}", Typename()));
  }
}

template <typename V, typename G>
void MVariable<V, G>::PrintValue(
    std::string_view variable_name, std::reference_wrapper<std::ostream> os,
    std::reference_wrapper<const moriarty_internal::VariableSet> variables,
    std::reference_wrapper<const moriarty_internal::ValueSet> values) const {
  PrinterContext ctx(variable_name, os, variables, values);
  Print(ctx, ctx.GetValue<V>(ctx.GetVariableName()));
}

template <typename V, typename G>
std::optional<std::string> MVariable<V, G>::CheckValue(
    std::string_view variable_name,
    std::reference_wrapper<const moriarty_internal::VariableSet> variables,
    std::reference_wrapper<const moriarty_internal::ValueSet> values) const {
  AnalysisContext ctx(variable_name, variables, values);
  return CheckValue(ctx, ctx.GetValue<V>(ctx.GetVariableName()));
}

template <typename V, typename G>
std::vector<std::unique_ptr<moriarty_internal::AbstractVariable>>
MVariable<V, G>::ListAnonymousEdgeCases(
    std::string_view variable_name,
    std::reference_wrapper<const moriarty_internal::VariableSet> variables,
    std::reference_wrapper<const moriarty_internal::ValueSet> values) const {
  AnalysisContext ctx(variable_name, variables, values);
  std::vector<V> instances = ListEdgeCases(ctx);
  std::vector<std::unique_ptr<moriarty_internal::AbstractVariable>> new_vec;
  new_vec.reserve(instances.size());
  for (V& instance : instances) {
    new_vec.push_back(std::make_unique<V>(std::move(instance)));
  }
  return new_vec;
}

template <typename V, typename G>
MVariable<V, G>::CustomConstraintWrapper::CustomConstraintWrapper(
    CustomConstraint<V> constraint)
    : constraint_(std::move(constraint)) {}

template <typename V, typename G>
bool MVariable<V, G>::CustomConstraintWrapper::IsSatisfiedWith(
    AnalysisContext ctx, const G& value) const {
  return constraint_.IsSatisfiedWith(ctx, value);
}

template <typename V, typename G>
std::string MVariable<V, G>::CustomConstraintWrapper::UnsatisfiedReason(
    AnalysisContext ctx, const G& value) const {
  return constraint_.UnsatisfiedReason(value);
}

template <typename V, typename G>
std::string MVariable<V, G>::CustomConstraintWrapper::ToString() const {
  return constraint_.ToString();
}

template <typename V, typename G>
std::vector<std::string>
MVariable<V, G>::CustomConstraintWrapper::GetDependencies() const {
  return constraint_.GetDependencies();
}

template <typename V, typename G>
void MVariable<V, G>::CustomConstraintWrapper::ApplyTo(V& other) const {
  other.AddConstraint(constraint_);
}

}  // namespace librarian
}  // namespace moriarty

#endif  // MORIARTY_SRC_LIBRARIAN_MVARIABLE_H_
