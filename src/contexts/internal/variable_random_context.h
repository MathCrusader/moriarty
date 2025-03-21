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

#ifndef MORIARTY_SRC_CONTEXTS_INTERNAL_VARIABLE_RANDOM_CONTEXT_H_
#define MORIARTY_SRC_CONTEXTS_INTERNAL_VARIABLE_RANDOM_CONTEXT_H_

#include <concepts>
#include <format>
#include <functional>

#include "src/internal/abstract_variable.h"
#include "src/internal/generation_bootstrap.h"
#include "src/internal/random_engine.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/variables/constraints/size_constraints.h"

namespace moriarty {
namespace moriarty_internal {

// VariableRandomContext
//
// A class to handle Moriarty type-specific randomness.
class VariableRandomContext {
 public:
  explicit VariableRandomContext(
      std::reference_wrapper<const VariableSet> variables,
      std::reference_wrapper<const ValueSet> values,
      std::reference_wrapper<RandomEngine> engine);

  // Random()
  //
  // Returns a random value which satisfies all the constraints specified in
  // `m`.
  //
  // Example:
  //   int64_t x = RandomMVariable(MInteger(Between(1, "N"), Prime()));
  //   std::string S = RandomMVariable(MString(SimplePattern("[a-z]{5}")));
  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  [[nodiscard]] T::value_type Random(T m);

  // RandomValue()
  //
  // Returns a random value for the variable named `variable_name`.
  //
  // Throws if the variable is not known.
  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  [[nodiscard]] T::value_type RandomValue(std::string_view variable_name);

  // RandomValue()
  //
  // Returns a random value for the variable named `variable_name`. Also imposes
  // `extra_constraints` on the variable.
  //
  // Throws if the variable is not known. If a value for the variable is already
  // known, then `extra_constraints` is ignored.
  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  [[nodiscard]] T::value_type RandomValue(std::string_view variable_name,
                                          T extra_constraints);

  // MinValue()
  //
  // Returns the smallest value for `variable_name`.
  //
  // Equivalent to:
  //   RandomValue<MType>(variable_name, MType(SizeCategory::Min()));
  //
  // Example:
  //   int64_t x = MinValue<MInteger>("x");
  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  [[nodiscard]] T::value_type MinValue(std::string_view variable_name);

  // MaxValue()
  //
  // Returns the largest value for `variable_name`.
  //
  // Equivalent to:
  //   RandomValue<MType>(variable_name, MType(SizeCategory::Max()));
  //
  // Example:
  //   int64_t x = MaxValue<MInteger>("x");
  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  [[nodiscard]] T::value_type MaxValue(std::string_view variable_name);

 private:
  std::reference_wrapper<const VariableSet> variables_;
  std::reference_wrapper<const ValueSet> values_;
  std::reference_wrapper<RandomEngine> engine_;
};

// --------------------------------------------------------------------------
//  Template implementation below

template <typename T>
  requires std::derived_from<T, AbstractVariable>
T::value_type VariableRandomContext::Random(T m) {
  std::string name = std::format("Random({})", m.Typename());
  VariableSet variables_copy = variables_.get();
  variables_copy.SetVariable(name, m);

  ValueSet values = GenerateAllValues(std::move(variables_copy), values_.get(),
                                      {.random_engine = engine_.get()});
  return values.Get<T>(name);
}

template <typename T>
  requires std::derived_from<T, AbstractVariable>
T::value_type VariableRandomContext::RandomValue(
    std::string_view variable_name) {
  if (values_.get().Contains(variable_name))
    return values_.get().Get<T>(variable_name);

  return Random<T>(variables_.get().GetVariable<T>(variable_name));
}

template <typename T>
  requires std::derived_from<T, AbstractVariable>
T::value_type VariableRandomContext::RandomValue(std::string_view variable_name,
                                                 T extra_constraints) {
  if (values_.get().Contains(variable_name))
    return values_.get().Get<T>(variable_name);

  T variable = variables_.get().GetVariable<T>(variable_name);
  variable.MergeFrom(std::move(extra_constraints));
  return Random<T>(std::move(variable));
}

template <typename T>
  requires std::derived_from<T, AbstractVariable>
T::value_type VariableRandomContext::MinValue(std::string_view variable_name) {
  return RandomValue<T>(variable_name, T(SizeCategory::Min()));
}

template <typename T>
  requires std::derived_from<T, AbstractVariable>
T::value_type VariableRandomContext::MaxValue(std::string_view variable_name) {
  return RandomValue<T>(variable_name, T(SizeCategory::Max()));
}

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_INTERNAL_VARIABLE_RANDOM_CONTEXT_H_
