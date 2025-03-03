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

#ifndef MORIARTY_SRC_CONTEXTS_LIBRARIAN_ANALYSIS_CONTEXT_H_
#define MORIARTY_SRC_CONTEXTS_LIBRARIAN_ANALYSIS_CONTEXT_H_

#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include "absl/status/statusor.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"

namespace moriarty {
namespace librarian {

// AnalysisContext
//
// Allows you to inspect the current state of the variables and values.
// AnalysisContext is read-only. It does not allow you to modify the variables
// or values.
class AnalysisContext {
 public:
  explicit AnalysisContext(std::string_view variable_name,
                           const moriarty_internal::VariableSet& variables,
                           const moriarty_internal::ValueSet& values)
      : name_(variable_name), variables_(variables), values_(values) {}

  // FIXME: This is a placeholder for testing.

  [[nodiscard]] std::string GetVariableName() const { return name_; }

  template <typename T>
    requires std::derived_from<T, moriarty_internal::AbstractVariable>
  absl::StatusOr<T> GetVariable(std::string_view variable_name) const {
    return variables_.get().GetVariable<T>(variable_name);
  }

  template <typename T>
    requires std::derived_from<T, moriarty_internal::AbstractVariable>
  absl::StatusOr<typename T::value_type> GetValue(
      std::string_view variable_name) const {
    return values_.get().Get<T>(variable_name);
  }

  template <typename T>
    requires std::derived_from<T, moriarty_internal::AbstractVariable>
  std::optional<typename T::value_type> GetUniqueValue(
      std::string_view variable_name) const {
    auto stored_value = GetValueIfKnown<T>(variable_name);
    if (stored_value) return *stored_value;

    auto variable = GetVariable<T>(variable_name);
    if (!variable.ok()) return std::nullopt;
    return variable->GetUniqueValue(*this);
  }

  template <typename T>
    requires std::derived_from<T, moriarty_internal::AbstractVariable>
  std::optional<typename T::value_type> GetValueIfKnown(
      std::string_view variable_name) const {
    absl::StatusOr<typename T::value_type> value =
        values_.get().Get<T>(variable_name);
    if (value.ok()) return *value;
    return std::nullopt;
  }

  bool ValueIsKnown(std::string_view variable_name) const {
    return values_.get().Contains(variable_name);
  }

 private:
  std::string name_;
  std::reference_wrapper<const moriarty_internal::VariableSet> variables_;
  std::reference_wrapper<const moriarty_internal::ValueSet> values_;
};

}  // namespace librarian
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_LIBRARIAN_ANALYSIS_CONTEXT_H_
