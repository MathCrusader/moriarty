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

#ifndef MORIARTY_SRC_CONTEXTS_LIBRARIAN_ASSIGNMENT_CONTEXT_H_
#define MORIARTY_SRC_CONTEXTS_LIBRARIAN_ASSIGNMENT_CONTEXT_H_

#include <string_view>

#include "src/contexts/internal/mutable_values_context.h"
#include "src/contexts/librarian/analysis_context.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"

namespace moriarty {
namespace librarian {

// AssignmentContext
//
// Allows you to inspect the current state of the variables and set values.
class AssignmentContext : public librarian::AnalysisContext,
                          public moriarty_internal::MutableValuesContext {
  using AnalysisBase = AnalysisContext;
  using ValuesBase = moriarty_internal::MutableValuesContext;

 public:
  explicit AssignmentContext(std::string_view variable_name,
                             const moriarty_internal::VariableSet& variables,
                             moriarty_internal::ValueSet& values)
      : AnalysisBase(variable_name, variables, values), ValuesBase(values) {}

  using AnalysisContext::GetUniqueValue;
  using AnalysisContext::GetValueIfKnown;
  using AnalysisContext::GetVariable;
  using AnalysisContext::GetVariableName;
  using AnalysisContext::ValueIsKnown;
  using ValuesBase::EraseValue;
  using ValuesBase::SetValue;
};

}  // namespace librarian
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_LIBRARIAN_ASSIGNMENT_CONTEXT_H_
