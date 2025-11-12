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

#ifndef MORIARTY_SRC_CONTEXTS_INTERNAL_RESOLVE_VALUES_CONTEXT_H_
#define MORIARTY_SRC_CONTEXTS_INTERNAL_RESOLVE_VALUES_CONTEXT_H_

#include <stdexcept>
#include <string_view>

#include "src/internal/abstract_variable.h"
#include "src/internal/generation_handler.h"
#include "src/internal/random_engine.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/util/ref.h"

namespace moriarty {
namespace moriarty_internal {

// ResolveValuesContext
//
// Allows you to update the values currently stored.
class ResolveValuesContext {
 public:
  explicit ResolveValuesContext(
      Ref<const moriarty_internal::VariableSet> variables,
      Ref<moriarty_internal::ValueSet> values,
      Ref<moriarty_internal::RandomEngine> engine,
      Ref<moriarty_internal::GenerationHandler> handler);

  // TODO: Add `AddImpliedConstraint` to add constraints to the context.

  // GenerateVariable()
  //
  // Generates a value for the variable `variable_name` and stores it into
  // `ctx`. If the variable is already known, it will return the known value.
  template <MoriartyVariable T>
  [[nodiscard]] T::value_type GenerateVariable(std::string_view variable_name);

  // GenerateVariable()
  //
  // Generates a value for the variable `variable_name` and stores it into
  // `ctx`. The variable will be merged into `extra_constraints` and generated
  // (in particular, this means these constraints are not permanently added to
  // this variable).
  //
  // If the variable is already known, it will return that known value, and
  // throw if it does not satisfy `extra_constraints`.
  template <MoriartyVariable T>
  [[nodiscard]] T::value_type GenerateVariable(std::string_view variable_name,
                                               T extra_constraints);

  // AssignVariable()
  //
  // Assign a value to the variable named `variable_name`.
  void AssignVariable(std::string_view variable_name);

 private:
  Ref<const moriarty_internal::VariableSet> variables_;
  Ref<moriarty_internal::ValueSet> values_;
  Ref<moriarty_internal::RandomEngine> engine_;
  Ref<moriarty_internal::GenerationHandler> handler_;
};

// -----------------------------------------------------------------------------
//  Template implementation below

template <MoriartyVariable T>
T::value_type ResolveValuesContext::GenerateVariable(
    std::string_view variable_name) {
  if (values_.get().Contains(variable_name))
    return values_.get().Get<T>(variable_name);

  T variable = variables_.get().GetVariable<T>(variable_name);

  auto value = variable.Generate(
      {variable_name, variables_, values_, engine_, handler_});
  values_.get().Set<T>(variable_name, value);
  return value;
}

template <MoriartyVariable T>
T::value_type ResolveValuesContext::GenerateVariable(
    std::string_view variable_name, T extra_constraints) {
  if (values_.get().Contains(variable_name)) {
    if (auto check =
            extra_constraints.CheckValue({variable_name, variables_, values_},
                                         values_.get().Get<T>(variable_name))) {
      throw std::runtime_error(
          std::format("Value for {} is already known, but does not satisfy "
                      "the extra constraints requested: {}",
                      variable_name, check.Reason()));
    }
    return values_.get().Get<T>(variable_name);
  }

  T variable = variables_.get().GetVariable<T>(variable_name);
  extra_constraints.MergeFrom(variable);

  auto value = extra_constraints.Generate(
      {variable_name, variables_, values_, engine_, handler_});
  values_.get().Set<T>(variable_name, value);
  return value;
}

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_INTERNAL_RESOLVE_VALUES_CONTEXT_H_
