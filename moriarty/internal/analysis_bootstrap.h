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

#ifndef MORIARTY_INTERNAL_ANALYSIS_BOOTSTRAP_H_
#define MORIARTY_INTERNAL_ANALYSIS_BOOTSTRAP_H_

#include <span>
#include <string>
#include <vector>

#include "moriarty/constraints/constraint_violation.h"
#include "moriarty/context.h"
#include "moriarty/internal/value_set.h"
#include "moriarty/internal/variable_set.h"

namespace moriarty {
namespace moriarty_internal {

// ValidateValues()
//
// Checks that stored values satisfy the constraints of their corresponding
// variables. Returns a list of all problems found, or an empty vector if there
// are none.
//
// `options.validation` determines what to check:
// * kNone             : skips all checks.
// * kOnlySetVariables : checks variables that have a stored value.
// * kOnlySetValues    : also reports stored values with no corresponding
//                       variable.
// * kAllVariables     : checks every variable and reports any missing values.
// * kEverything       : checks every variable and reports both missing values
//                       and stored values with no corresponding variable.
//
// To check only specific names, list them in `options.variables_to_validate`.
// An empty list does not restrict which names are checked. kEverything always
// checks all names, ignoring this list.
//
// `options.constraint_options` controls how each constraint is evaluated.
[[nodiscard]] std::vector<DetailedValidationResult> ValidateValues(
    const VariableSet& variables, const ValueSet& values,
    const ValidateOptions& options);

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_INTERNAL_ANALYSIS_BOOTSTRAP_H_
