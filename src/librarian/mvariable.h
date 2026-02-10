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

#ifndef MORIARTY_LIBRARIAN_MVARIABLE_H_
#define MORIARTY_LIBRARIAN_MVARIABLE_H_

#include <algorithm>
#include <concepts>
#include <cstdint>
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

#include "src/constraints/base_constraints.h"
#include "src/constraints/constraint_violation.h"
#include "src/constraints/custom_constraint.h"
#include "src/context.h"
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
#include "src/librarian/util/ref.h"

namespace moriarty {

// Determines if ConstraintType is a valid constraint to be applied to
// MVariableType.
template <typename MVariableType, typename ConstraintType>
concept ConstraintFor =
    MoriartyVariable<MVariableType> &&
    requires(MVariableType var, ConstraintType c) { var.AddConstraint(c); };

namespace librarian {

template <typename MVariableType, typename ConstraintType>
constexpr void AssertIsConstraintForType() {
  static_assert(ConstraintFor<MVariableType, ConstraintType>,
                "Constraint is not valid for the MVariable.");
}

struct NoValueType {};
template <typename T>
struct MVariableValueTypeTrait {
  using type = NoValueType;
};

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
// * Make sure you have specialized `MVariableValueTypeTrait<VariableType>`;
//   `value_type` is the non-Moriarty type that is generated/validated/analyzed.
//
// E.g.,
//     template <>
//     struct MVariableValueTypeTrait<MInteger> {
//       using type = int64_t;
//     };
//     class MInteger : public MVariable<MInteger> {}
template <typename VariableType>
class MVariable : public moriarty_internal::AbstractVariable {
 public:
  static constexpr bool is_moriarty_variable = true;
  using value_type = MVariableValueTypeTrait<VariableType>::type;

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
      std::string_view name, std::function<bool(const value_type&)> checker);

  // AddCustomConstraint()
  //
  // Adds a constraint that is fully user-defined. This version of
  // `AddCustomConstraint` depends on the value of other variables. (E.g., at
  // least X, larger than the average of the elements in A, etc).
  VariableType& AddCustomConstraint(
      std::string_view name, std::vector<std::string> dependencies,
      std::function<bool(ConstraintContext, const value_type&)> checker);

  // AddConstraint()
  //
  // Adds a constraint that is fully user-defined. See other overloads of
  // `AddCustomConstraint` for simple construction.
  VariableType& AddConstraint(CustomConstraint<VariableType> constraint);

  // AddConstraints()
  //
  // Adds multiple constraints at once.
  template <typename... Constraints>
    requires(sizeof...(Constraints) > 0)
  VariableType& AddConstraints(Constraints&&... constraints);

  // AddConstraints()
  //
  // Adds multiple constraints at once.
  template <typename... Constraints>
  VariableType& AddConstraints(std::tuple<Constraints...> constraints);

  // CheckValue()
  //
  // Determines if `value` satisfies all constraints on this variable.
  ConstraintViolation CheckValue(ConstraintContext ctx,
                                 const value_type& value) const;

  // Generate()
  //
  // Returns a random value that satisfies all constraints on this variable.
  [[nodiscard]] value_type Generate(GenerateVariableContext ctx) const;

  // GetUniqueValue()
  //
  // Determines if there is exactly one value that this variable can be
  // assigned to. If so, return that value. If there is not a unique value
  // (or it is too hard to determine that there is a unique value), returns
  // `std::nullopt`. Returning `std::nullopt` does not guarantee there is
  // not a unique value.
  [[nodiscard]] std::optional<value_type> GetUniqueValue(
      AnalyzeVariableContext ctx) const;

  // Write()
  //
  // Writes `value` into `ctx` using any formatting provided to this
  // variable (as I/O constraints).
  void Write(WriteVariableContext ctx, const value_type& value) const;

  // Read()
  //
  // Reads a value from `ctx` using any formatting provided to this variable
  // (as I/O constraints).
  [[nodiscard]] value_type Read(ReadVariableContext ctx) const;

  // ListEdgeCases()
  //
  // Returns a list of MVariables that should give common difficult cases. Some
  // examples of this type of cases include common pitfalls for your data type
  // (edge cases, queries of death, etc).
  [[nodiscard]] std::vector<VariableType> ListEdgeCases(
      AnalyzeVariableContext ctx) const;

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
  virtual value_type GenerateImpl(GenerateVariableContext ctx) const = 0;
  virtual std::optional<value_type> GetUniqueValueImpl(
      AnalyzeVariableContext ctx) const;
  virtual value_type ReadImpl(ReadVariableContext ctx) const;
  virtual void WriteImpl(WriteVariableContext ctx,
                         const value_type& value) const;
  virtual std::vector<VariableType> ListEdgeCasesImpl(
      AnalyzeVariableContext ctx) const;
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
  // Adds an `Exactly<value_type>` constraint to this variable. This will
  // call InternalAddConstraint(), so you don't need to.
  VariableType& InternalAddExactlyConstraint(Exactly<value_type> c);

  // InternalAddOneOfConstraint() [For Librarians]
  //
  // Adds an `OneOf<value_type>` constraint to this variable. This will
  // call InternalAddConstraint(), so you don't need to.
  VariableType& InternalAddOneOfConstraint(OneOf<value_type> c);

  // GetOneOf()
  //
  // Returns the OneOfHandler for this variable.
  [[nodiscard]] OneOfHandler<value_type>& GetOneOf();

  // GetOneOf()
  //
  // Returns the OneOfHandler for this variable.
  [[nodiscard]] const OneOfHandler<value_type>& GetOneOf() const;

 private:
  ConstraintHandler<VariableType, value_type> constraints_;
  OneOfHandler<value_type> one_of_;
  std::vector<std::string> dependencies_;

  // Helper function that casts *this to `VariableType`.
  [[nodiscard]] VariableType& UnderlyingVariableType();
  [[nodiscard]] const VariableType& UnderlyingVariableType() const;
  [[nodiscard]] value_type GenerateOnce(GenerateVariableContext ctx) const;

  // ---------------------------------------------------------------------------
  //  AbstractVariable overrides
  [[nodiscard]] std::unique_ptr<moriarty_internal::AbstractVariable> Clone()
      const override;
  void AssignValue(
      std::string_view variable_name,
      Ref<const moriarty_internal::VariableSet> variables,
      Ref<moriarty_internal::ValueSet> values,
      Ref<moriarty_internal::RandomEngine> engine,
      Ref<moriarty_internal::GenerationHandler> handler) const override;
  void AssignUniqueValue(
      std::string_view variable_name,
      Ref<const moriarty_internal::VariableSet> variables,
      Ref<moriarty_internal::ValueSet> values) const override;
  std::optional<int64_t> UniqueInteger(
      std::string_view variable_name,
      Ref<const moriarty_internal::VariableSet> variables,
      Ref<const moriarty_internal::ValueSet> values) const override;
  void ReadValue(std::string_view variable_name, Ref<InputCursor> input,
                 Ref<const moriarty_internal::VariableSet> variables,
                 Ref<moriarty_internal::ValueSet> values) const override;
  std::unique_ptr<moriarty_internal::ChunkedReader> GetChunkedReader(
      std::string_view variable_name, int N, Ref<InputCursor> input,
      Ref<const moriarty_internal::VariableSet> variables,
      Ref<moriarty_internal::ValueSet> values) const override;
  void WriteValue(std::string_view variable_name, Ref<std::ostream> os,
                  Ref<const moriarty_internal::VariableSet> variables,
                  Ref<const moriarty_internal::ValueSet> values) const override;
  ConstraintViolation CheckValue(
      std::string_view variable_name,
      Ref<const moriarty_internal::VariableSet> variables,
      Ref<const moriarty_internal::ValueSet> values) const override;
  std::vector<std::unique_ptr<AbstractVariable>> ListAnonymousEdgeCases(
      std::string_view variable_name,
      Ref<const moriarty_internal::VariableSet> variables,
      Ref<const moriarty_internal::ValueSet> values) const override;
  // ---------------------------------------------------------------------------

  class CustomConstraintWrapper {
   public:
    explicit CustomConstraintWrapper(CustomConstraint<VariableType> constraint);
    ConstraintViolation CheckValue(ConstraintContext ctx,
                                   const value_type& value) const;
    std::string ToString() const;
    std::vector<std::string> GetDependencies() const;
    void ApplyTo(VariableType& other) const;

   private:
    CustomConstraint<VariableType> constraint_;
  };

  // Wrapper class around an MVariable's ChunkedReader. Expected API is:
  //   void ReadNext(ReadVariableContext);  // Read the next value
  //   value_type Finalize() &&;       // Return the read value
  template <typename ReaderType>
  class ChunkedReaderWrapper : public moriarty_internal::ChunkedReader {
   public:
    ChunkedReaderWrapper(ReaderType reader, ReadVariableContext ctx,
                         Ref<moriarty_internal::ValueSet> values)
        : reader_(std::move(reader)), ctx_(std::move(ctx)), values_(values) {}
    void ReadNext() override { reader_.ReadNext(ctx_); }
    void Finalize() && override {
      values_.get().Set<VariableType>(ctx_.GetVariableName(),
                                      std::move(reader_).Finalize());
    }

   private:
    ReaderType reader_;
    ReadVariableContext ctx_;
    Ref<moriarty_internal::ValueSet> values_;
  };
};

// -----------------------------------------------------------------------------
//  Template implementation below

template <typename V>
MVariable<V>::MVariable() {
  // Simple checks to ensure that the template arguments are correct.
  static_assert(std::default_initializable<V>);
  static_assert(std::copyable<V>);
  static_assert(std::derived_from<V, MVariable<V>>);
  static_assert(
      !std::same_as<typename MVariableValueTypeTrait<V>::type, NoValueType>,
      "No specialization of MVariableValueTypeTrait<> for this type");
}

// -----------------------------------------------------------------------------
//  Template implementation for public functions

template <typename V>
std::string MVariable<V>::ToString() const {
  return constraints_.ToString();
}

template <typename V>
V& MVariable<V>::MergeFrom(const V& other) {
  other.constraints_.ApplyAllTo(UnderlyingVariableType());
  return UnderlyingVariableType();
}

template <typename V>
V& MVariable<V>::AddCustomConstraint(
    std::string_view name, std::function<bool(const value_type&)> checker) {
  return AddConstraint(CustomConstraint<V>(name, std::move(checker)));
}

template <typename V>
V& MVariable<V>::AddCustomConstraint(
    std::string_view name, std::vector<std::string> dependencies,
    std::function<bool(ConstraintContext, const value_type&)> checker) {
  return AddConstraint(
      CustomConstraint<V>(name, std::move(dependencies), std::move(checker)));
}

template <typename V>
V& MVariable<V>::AddConstraint(CustomConstraint<V> constraint) {
  return InternalAddConstraint(CustomConstraintWrapper(std::move(constraint)));
}

template <typename V>
template <typename... Constraints>
V& MVariable<V>::AddConstraints(std::tuple<Constraints...> constraints) {
  std::apply(
      [this](auto&&... cs) {
        (UnderlyingVariableType().AddConstraint(std::forward<decltype(cs)>(cs)),
         ...);
      },
      std::move(constraints));
  return UnderlyingVariableType();
}

template <typename V>
ConstraintViolation MVariable<V>::CheckValue(ConstraintContext ctx,
                                             const value_type& value) const {
  return constraints_.CheckValue(ctx, value);
}

template <typename V>
auto MVariable<V>::Generate(GenerateVariableContext ctx) const -> value_type {
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
      value_type value = GenerateOnce(ctx);
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

template <typename V>
auto MVariable<V>::GetUniqueValue(AnalyzeVariableContext ctx) const
    -> std::optional<value_type> {
  std::optional<value_type> known =
      ctx.GetValueIfKnown<V>(ctx.GetVariableName());
  if (known) return known;
  if (one_of_.GetUniqueValue()) return one_of_.GetUniqueValue();
  return GetUniqueValueImpl(ctx);
}

template <typename V>
void MVariable<V>::Write(WriteVariableContext ctx,
                         const value_type& value) const {
  WriteImpl(ctx, value);
}

template <typename V>
auto MVariable<V>::Read(ReadVariableContext ctx) const -> value_type {
  value_type value = ReadImpl(ctx);
  if (auto reason = CheckValue(ConstraintContext(ctx), value)) {
    ctx.ThrowIOError("Read value does not satisfy constraints: {}",
                     reason.Reason());
  }
  return value;
}

template <typename V>
auto MVariable<V>::ListEdgeCases(AnalyzeVariableContext ctx) const
    -> std::vector<V> {
  std::vector<V> instances = ListEdgeCasesImpl(ctx);
  for (V& instance : instances) {
    // TODO(hivini): When merging with a fixed value variable that has the same
    // value a difficult instance, this does not return an error. Determine if
    // this matters or not.
    instance.MergeFrom(UnderlyingVariableType());
  }
  return instances;
}

template <typename V>
std::vector<std::string> MVariable<V>::GetDependencies() const {
  return dependencies_;
}

template <typename V>
void MVariable<V>::MergeFromAnonymous(
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

template <typename V>
auto MVariable<V>::ReadImpl(ReadVariableContext ctx) const -> value_type {
  throw std::runtime_error(
      std::format("Read() not implemented for {}", Typename()));
}

template <typename V>
void MVariable<V>::WriteImpl(WriteVariableContext ctx,
                             const value_type& value) const {
  throw std::runtime_error(
      std::format("Write() not implemented for {}", Typename()));
}

template <typename V>
std::vector<V> MVariable<V>::ListEdgeCasesImpl(
    AnalyzeVariableContext ctx) const {
  return std::vector<V>();  // By default, return an empty list.
}

template <typename V>
auto MVariable<V>::GetUniqueValueImpl(AnalyzeVariableContext ctx) const
    -> std::optional<value_type> {
  return std::nullopt;  // By default, return no unique value.
}

template <typename V>
template <typename Constraint>
V& MVariable<V>::InternalAddConstraint(Constraint c) {
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

template <typename V>
V& MVariable<V>::InternalAddExactlyConstraint(Exactly<value_type> c) {
  if (!one_of_.ConstrainOptions(std::vector<value_type>{c.GetValue()}))
    throw ImpossibleToSatisfy(ToString(), c.ToString());
  return InternalAddConstraint(std::move(c));
}

template <typename V>
V& MVariable<V>::InternalAddOneOfConstraint(OneOf<value_type> c) {
  if (!one_of_.ConstrainOptions(c.GetOptions()))
    throw ImpossibleToSatisfy(ToString(), c.ToString());
  return InternalAddConstraint(std::move(c));
}

template <typename V>
auto MVariable<V>::GetOneOf() -> OneOfHandler<value_type>& {
  return one_of_;
}

template <typename V>
auto MVariable<V>::GetOneOf() const -> const OneOfHandler<value_type>& {
  return one_of_;
}

// -----------------------------------------------------------------------------
//  Template implementation for private functions not part of Extended API.

template <typename V>
V& MVariable<V>::UnderlyingVariableType() {
  return static_cast<V&>(*this);
}

template <typename V>
const V& MVariable<V>::UnderlyingVariableType() const {
  return static_cast<const V&>(*this);
}

template <typename V>
std::unique_ptr<moriarty_internal::AbstractVariable> MVariable<V>::Clone()
    const {
  // Use V's copy constructor.
  return std::make_unique<V>(UnderlyingVariableType());
}

template <typename V>
typename MVariable<V>::value_type MVariable<V>::GenerateOnce(
    GenerateVariableContext ctx) const {
  value_type potential_value = GenerateImpl(ctx);

  // Some dependent variables may not have been generate, but are required to
  // validate. Generate them now.
  for (std::string_view dep : dependencies_) ctx.AssignVariable(dep);

  if (auto reason = CheckValue(ConstraintContext(ctx), potential_value))
    throw moriarty::GenerationError(
        ctx.GetVariableName(),
        std::format("Generated value does not satisfy constraints: {}",
                    reason.Reason()),
        RetryPolicy::kRetry);

  return potential_value;
}

template <typename V>
void MVariable<V>::AssignValue(
    std::string_view variable_name,
    Ref<const moriarty_internal::VariableSet> variables,
    Ref<moriarty_internal::ValueSet> values,
    Ref<moriarty_internal::RandomEngine> engine,
    Ref<moriarty_internal::GenerationHandler> handler) const {
  GenerateVariableContext ctx(variable_name, variables, values, engine,
                              handler);

  if (ctx.ValueIsKnown(ctx.GetVariableName())) return;
  ctx.SetValue<V>(ctx.GetVariableName(), Generate(ctx));
}

template <typename V>
void MVariable<V>::AssignUniqueValue(
    std::string_view variable_name,
    Ref<const moriarty_internal::VariableSet> variables,
    Ref<moriarty_internal::ValueSet> values) const {
  AssignVariableContext ctx(variable_name, variables, values);
  if (ctx.ValueIsKnown(ctx.GetVariableName())) return;

  std::optional<value_type> value = GetUniqueValue(ctx);
  if (value) ctx.SetValue<V>(ctx.GetVariableName(), *value);
}

template <typename V>
std::optional<int64_t> MVariable<V>::UniqueInteger(
    std::string_view variable_name,
    Ref<const moriarty_internal::VariableSet> variables,
    Ref<const moriarty_internal::ValueSet> values) const {
  if constexpr (std::same_as<value_type, int64_t>) {
    AnalyzeVariableContext ctx(variable_name, variables, values);
    if (ctx.ValueIsKnown(ctx.GetVariableName())) {
      return ctx.GetValue<V>(ctx.GetVariableName());
    }

    return GetUniqueValue(ctx);
  }
  return std::nullopt;
}

template <typename V>
void MVariable<V>::ReadValue(
    std::string_view variable_name, Ref<InputCursor> input,
    Ref<const moriarty_internal::VariableSet> variables,
    Ref<moriarty_internal::ValueSet> values) const {
  ReadVariableContext ctx(variable_name, input, variables, values);
  values.get().Set<V>(ctx.GetVariableName(), Read(ctx));
}

template <typename V>
std::unique_ptr<moriarty_internal::ChunkedReader>
MVariable<V>::GetChunkedReader(
    std::string_view variable_name, int N, Ref<InputCursor> input,
    Ref<const moriarty_internal::VariableSet> variables,
    Ref<moriarty_internal::ValueSet> values) const {
  if constexpr (requires { typename V::chunked_reader_type; }) {
    ReadVariableContext ctx(variable_name, input, variables, values);
    return std::make_unique<
        ChunkedReaderWrapper<typename V::chunked_reader_type>>(
        typename V::chunked_reader_type(ctx, N, UnderlyingVariableType()), ctx,
        values);
  } else {
    throw ConfigurationError(
        this->Typename(),
        std::format("Unable to read {} in independent chunks.", Typename()));
  }
}

template <typename V>
void MVariable<V>::WriteValue(
    std::string_view variable_name, Ref<std::ostream> os,
    Ref<const moriarty_internal::VariableSet> variables,
    Ref<const moriarty_internal::ValueSet> values) const {
  WriteVariableContext ctx(variable_name, os, variables, values);
  Write(ctx, ctx.GetValue<V>(ctx.GetVariableName()));
}

template <typename V>
ConstraintViolation MVariable<V>::CheckValue(
    std::string_view variable_name,
    Ref<const moriarty_internal::VariableSet> variables,
    Ref<const moriarty_internal::ValueSet> values) const {
  ConstraintContext ctx(variable_name, variables, values);
  return CheckValue(ctx, ctx.GetValue<V>(ctx.GetVariableName()));
}

template <typename V>
std::vector<std::unique_ptr<moriarty_internal::AbstractVariable>>
MVariable<V>::ListAnonymousEdgeCases(
    std::string_view variable_name,
    Ref<const moriarty_internal::VariableSet> variables,
    Ref<const moriarty_internal::ValueSet> values) const {
  AnalyzeVariableContext ctx(variable_name, variables, values);
  std::vector<V> instances = ListEdgeCases(ctx);
  std::vector<std::unique_ptr<moriarty_internal::AbstractVariable>> new_vec;
  new_vec.reserve(instances.size());
  for (V& instance : instances) {
    new_vec.push_back(std::make_unique<V>(std::move(instance)));
  }
  return new_vec;
}

template <typename V>
MVariable<V>::CustomConstraintWrapper::CustomConstraintWrapper(
    CustomConstraint<V> constraint)
    : constraint_(std::move(constraint)) {}

template <typename V>
ConstraintViolation MVariable<V>::CustomConstraintWrapper::CheckValue(
    ConstraintContext ctx, const value_type& value) const {
  return constraint_.CheckValue(ctx, value);
}

template <typename V>
std::string MVariable<V>::CustomConstraintWrapper::ToString() const {
  return constraint_.ToString();
}

template <typename V>
std::vector<std::string>
MVariable<V>::CustomConstraintWrapper::GetDependencies() const {
  return constraint_.GetDependencies();
}

template <typename V>
void MVariable<V>::CustomConstraintWrapper::ApplyTo(V& other) const {
  other.AddConstraint(constraint_);
}

}  // namespace librarian
}  // namespace moriarty

#endif  // MORIARTY_LIBRARIAN_MVARIABLE_H_
