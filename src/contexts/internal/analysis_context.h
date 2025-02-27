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

#ifndef MORIARTY_SRC_CONTEXTS_INTERNAL_ANALYSIS_CONTEXT_H_
#define MORIARTY_SRC_CONTEXTS_INTERNAL_ANALYSIS_CONTEXT_H_

#include <functional>

#include "absl/status/statusor.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"

namespace moriarty {
namespace moriarty_internal {

// AnalysisContext
//
// Allows you to inspect the current state of the variables and values.
// AnalysisContext is read-only. It does not allow you to modify the variables
// or values.
class AnalysisContext {
 public:
  explicit AnalysisContext(const VariableSet& variables, const ValueSet& values)
      : variables_(variables), values_(values) {}

  // FIXME: This is a placeholder for testing.

  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  absl::StatusOr<T> GetVariable(std::string_view variable_name) const {
    return variables_.get().GetVariable<T>(variable_name);
  }

  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  absl::StatusOr<typename T::value_type> GetValue(
      std::string_view variable_name) const {
    return values_.get().Get<T>(variable_name);
  }

 private:
  std::reference_wrapper<const VariableSet> variables_;
  std::reference_wrapper<const ValueSet> values_;
};

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_INTERNAL_ANALYSIS_CONTEXT_H_
