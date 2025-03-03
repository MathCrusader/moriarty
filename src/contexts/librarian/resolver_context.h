/*
 * Copyright 2025 Darcy Best
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

#ifndef MORIARTY_SRC_CONTEXTS_LIBRARIAN_RESOLVER_CONTEXT_H_
#define MORIARTY_SRC_CONTEXTS_LIBRARIAN_RESOLVER_CONTEXT_H_

#include <string>
#include <string_view>

#include "src/contexts/internal/basic_random_context.h"
#include "src/contexts/internal/generation_orchestration_context.h"
#include "src/contexts/internal/mutable_values_context.h"
#include "src/contexts/librarian/analysis_context.h"
#include "src/internal/generation_config.h"
#include "src/internal/random_engine.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"

namespace moriarty {
namespace librarian {

// ResolverContext
//
// All context that MVariable<>::Generate() has access to.
class ResolverContext
    : public AnalysisContext,
      public moriarty_internal::MutableValuesContext,
      public moriarty_internal::BasicRandomContext,
      public moriarty_internal::GenerationOrchestrationContext {
  using AnalysisBase = AnalysisContext;
  using ValuesBase = moriarty_internal::MutableValuesContext;
  using RandomBase = moriarty_internal::BasicRandomContext;
  using OrchestrationBase = moriarty_internal::GenerationOrchestrationContext;

 public:
  // Note: Users should not need to create this object. This object will be
  // created by Moriarty and passed to you. See `src/Moriarty.h` for
  // entry-points.
  ResolverContext(std::string_view variable_name,
                  const moriarty_internal::VariableSet& variables,
                  moriarty_internal::ValueSet& values,
                  moriarty_internal::RandomEngine& engine,
                  moriarty_internal::GenerationConfig& config)
      : AnalysisBase(variable_name, variables, values),
        ValuesBase(values),
        RandomBase(engine),
        OrchestrationBase(config),
        name_(variable_name),
        variables_(variables),
        values_(values) {}

  // WithVariable()
  //
  // Creates a copy of this context, except the variable name is replaced with
  // `variable_name`.
  [[nodiscard]] ResolverContext WithVariable(std::string_view new_name) const {
    return ResolverContext(new_name, *this);
  }

  // WithSubVariable()
  //
  // Creates a copy of this context, except the variable name is replaced with
  // the name of a subvariable of the current one. (E.g., if the current
  // variable is A, then WithSubVariable("length") will set the new variable to
  // be "A.length")
  [[nodiscard]] ResolverContext WithSubVariable(
      std::string_view new_name) const {
    return ResolverContext(std::format("{}.{}", name_, new_name), *this);
  }

  // GetVariableName()
  //
  // Returns the name of the variable being generated.
  [[nodiscard]] std::string GetVariableName() const { return name_; }

  // GetValue()
  //
  // Returns the value of the variable `variable_name`.
  using AnalysisBase::GetValue;

  // GetVariable()
  //
  // Returns the variable `variable_name`.
  using AnalysisBase::GetVariable;

  // TODO: Add `AddImpliedConstraint` to add constraints to the context.

  // TODO: Add version of GenerateVariable that takes extra constraints.
  template <typename T>
    requires std::derived_from<T, moriarty_internal::AbstractVariable>
  [[nodiscard]] T::value_type GenerateVariable(std::string_view variable_name) {
    std::optional<typename T::value_type> known =
        GetValueIfKnown<T>(variable_name);
    if (known) return *known;

    absl::StatusOr<T> variable = variables_.get().GetVariable<T>(variable_name);
    if (!variable.ok())
      throw std::runtime_error(std::string(variable.status().message()));

    typename T::value_type value =
        variable->Generate(WithVariable(variable_name));
    values_.get().Set<T>(variable_name, value);
    return value;
  }

  void AssignVariable(std::string_view variable_name) {
    auto variable = variables_.get().GetAbstractVariable(variable_name);
    if (!variable.ok())
      throw std::runtime_error(std::string(variable.status().message()));
    auto status = (*variable)->AssignValue(WithVariable(variable_name));
    if (!status.ok()) throw std::runtime_error(std::string(status.message()));
  }

 private:
  std::string name_;
  std::reference_wrapper<const moriarty_internal::VariableSet> variables_;
  std::reference_wrapper<moriarty_internal::ValueSet> values_;

  ResolverContext(std::string_view new_variable_name,
                  const ResolverContext& other)
      : AnalysisBase(new_variable_name, other.variables_, other.values_),
        ValuesBase(other.values_),
        RandomBase(other),
        OrchestrationBase(other),
        name_(new_variable_name),
        variables_(other.variables_),
        values_(other.values_) {}
};

}  // namespace librarian
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_LIBRARIAN_RESOLVER_CONTEXT_H_
