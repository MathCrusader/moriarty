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

#include <functional>
#include <string_view>

#include "src/contexts/internal/basic_random_context.h"
#include "src/contexts/internal/generation_orchestration_context.h"
#include "src/contexts/internal/mutable_values_context.h"
#include "src/contexts/internal/name_context.h"
#include "src/contexts/internal/view_only_context.h"
#include "src/internal/generation_handler.h"
#include "src/internal/random_engine.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"

namespace moriarty {
namespace librarian {

// ResolverContext
//
// All context that MVariable<>::Generate() has access to.
class ResolverContext
    : public moriarty_internal::NameContext,
      public moriarty_internal::ViewOnlyContext,
      public moriarty_internal::MutableValuesContext,
      public moriarty_internal::BasicRandomContext,
      public moriarty_internal::GenerationOrchestrationContext {
  using NameContext = moriarty_internal::NameContext;
  using ViewOnlyContext = moriarty_internal::ViewOnlyContext;
  using MutableValuesContext = moriarty_internal::MutableValuesContext;
  using BasicRandomContext = moriarty_internal::BasicRandomContext;
  using GenerationOrchestrationContext =
      moriarty_internal::GenerationOrchestrationContext;

 public:
  // Created by Moriarty and passed to you; no need to instantiate.
  // See `src/Moriarty.h` for entry points.
  ResolverContext(std::string_view variable_name,
                  const moriarty_internal::VariableSet& variables,
                  moriarty_internal::ValueSet& values,
                  moriarty_internal::RandomEngine& engine,
                  moriarty_internal::GenerationHandler& handler)
      : NameContext(variable_name),
        ViewOnlyContext(variables, values),
        MutableValuesContext(values),
        BasicRandomContext(engine),
        GenerationOrchestrationContext(handler),
        variables_(variables),
        values_(values) {}

  // ********************************************
  // ** See parent classes for more functions. **
  // ********************************************

  // ForVariable()
  //
  // Creates a copy of this context, except the variable name is replaced with
  // `variable_name`.
  [[nodiscard]] ResolverContext ForVariable(std::string_view new_name) const {
    return ResolverContext(new_name, *this);
  }

  // ForSubVariable()
  //
  // Creates a copy of this context, except the variable name is replaced with
  // the name of a subvariable of the current one. (E.g., if the current
  // variable is A, then ForSubVariable("length") will set the new variable to
  // be "A.length")
  [[nodiscard]] ResolverContext ForSubVariable(
      std::string_view new_name) const {
    return ResolverContext(std::format("{}.{}", GetVariableName(), new_name),
                           *this);
  }

  // TODO: Add `AddImpliedConstraint` to add constraints to the context.

  // TODO: Add version of GenerateVariable that takes extra constraints.
  template <typename T>
    requires std::derived_from<T, moriarty_internal::AbstractVariable>
  [[nodiscard]] T::value_type GenerateVariable(std::string_view variable_name) {
    auto known = GetValueIfKnown<T>(variable_name);
    if (known) return *known;

    T variable = GetVariable<T>(variable_name);

    auto value = variable.Generate(ForVariable(variable_name));
    SetValue<T>(variable_name, value);
    return value;
  }

  void AssignVariable(std::string_view variable_name) {
    const moriarty_internal::AbstractVariable& variable =
        GetAnonymousVariable(variable_name);
    variable.AssignValue(ForVariable(variable_name));
  }

 private:
  std::reference_wrapper<const moriarty_internal::VariableSet> variables_;
  std::reference_wrapper<moriarty_internal::ValueSet> values_;

  ResolverContext(std::string_view new_variable_name,
                  const ResolverContext& other)
      : NameContext(new_variable_name),
        ViewOnlyContext(other),
        MutableValuesContext(other),
        BasicRandomContext(other),
        GenerationOrchestrationContext(other),
        variables_(other.variables_),
        values_(other.values_) {}
};

}  // namespace librarian
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_LIBRARIAN_RESOLVER_CONTEXT_H_
