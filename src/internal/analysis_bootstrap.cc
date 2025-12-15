// Copyright 2025 Darcy Best
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

#include "src/internal/analysis_bootstrap.h"

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "src/constraints/constraint_violation.h"
#include "src/context.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"

namespace moriarty {
namespace moriarty_internal {

namespace {

bool ShouldValidateVariable(ValidationStyle validation,
                            std::span<const std::string> variables_to_validate,
                            std::string_view name) {
  if (validation == ValidationStyle::kEverything) return true;
  if (variables_to_validate.empty()) return true;
  return std::ranges::find(variables_to_validate, name) !=
         variables_to_validate.end();
}

}  // namespace

std::vector<DetailedConstraintViolation> CheckValues(
    const VariableSet& variables, const ValueSet& values,
    std::span<const std::string> variables_to_validate,
    ValidationStyle validation) {
  if (validation == ValidationStyle::kNone) return {};

  std::vector<DetailedConstraintViolation> violations;
  for (const auto& [name, var] : variables.ListVariables()) {
    if (!ShouldValidateVariable(validation, variables_to_validate, name))
      continue;

    if (!values.Contains(name)) {
      if (validation == ValidationStyle::kAllVariables ||
          validation == ValidationStyle::kEverything) {
        violations.push_back(
            {name, ConstraintViolation(std::format(
                       "No value assigned to variable `{}`", name))});
      }
      continue;
    }
    if (auto reason = var->CheckValue(name, variables, values)) {
      violations.push_back({name, reason});
    }
  }

  if (validation == ValidationStyle::kOnlySetValues ||
      validation == ValidationStyle::kEverything) {
    for (const auto& name : values.ListValues()) {
      if (variables.Contains(name)) continue;  // Handled above
      if (!ShouldValidateVariable(validation, variables_to_validate, name))
        continue;

      violations.push_back(
          {name, ConstraintViolation(std::format(
                     "No variable found for `{}`, but a value "
                     "was set for it (use `Moriarty::AddVariable()`)",
                     name))});
    }
  }

  return violations;
}

}  // namespace moriarty_internal
}  // namespace moriarty
