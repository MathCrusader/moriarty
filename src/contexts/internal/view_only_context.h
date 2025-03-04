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

#ifndef MORIARTY_SRC_CONTEXTS_INTERNAL_VIEW_ONLY_CONTEXT_H_
#define MORIARTY_SRC_CONTEXTS_INTERNAL_VIEW_ONLY_CONTEXT_H_

#include <functional>
#include <optional>
#include <string_view>

#include "absl/status/statusor.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"

namespace moriarty {
namespace moriarty_internal {

// ViewOnlyContext
//
// Allows you to inspect the current state of the variables and values.
// ViewOnlyContext is read-only. It does not allow you to modify the variables
// or values.
class ViewOnlyContext {
 public:
  explicit ViewOnlyContext(const VariableSet& variables, const ValueSet& values)
      : variables_(variables), values_(values) {}

  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  [[nodiscard]] absl::StatusOr<T> GetVariable(
      std::string_view variable_name) const {
    return variables_.get().GetVariable<T>(variable_name);
  }

  [[nodiscard]] const moriarty_internal::AbstractVariable& GetAnonymousVariable(
      std::string_view variable_name) const {
    auto var = variables_.get().GetAbstractVariable(variable_name);
    if (!var.ok())
      throw std::runtime_error(std::string(var.status().message()));
    return *(var.value());
  }

  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  [[nodiscard]] absl::StatusOr<typename T::value_type> GetValue(
      std::string_view variable_name) const {
    return values_.get().Get<T>(variable_name);
  }

  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  [[nodiscard]] std::optional<typename T::value_type> GetUniqueValue(
      std::string_view variable_name) const {
    auto stored_value = GetValueIfKnown<T>(variable_name);
    if (stored_value) return *stored_value;

    auto variable = GetVariable<T>(variable_name);
    if (!variable.ok()) return std::nullopt;
    return variable->GetUniqueValueFromViewOnly(variable_name, *this);
  }

  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  [[nodiscard]] std::optional<typename T::value_type> GetValueIfKnown(
      std::string_view variable_name) const {
    absl::StatusOr<typename T::value_type> value =
        values_.get().Get<T>(variable_name);
    if (value.ok()) return *value;
    return std::nullopt;
  }

  [[nodiscard]] bool ValueIsKnown(std::string_view variable_name) const {
    return values_.get().Contains(variable_name);
  }

 private:
  std::reference_wrapper<const VariableSet> variables_;
  std::reference_wrapper<const ValueSet> values_;
};

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_INTERNAL_VIEW_ONLY_CONTEXT_H_
