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

#ifndef MORIARTY_SRC_INTERNAL_ANALYSIS_BOOTSTRAP_H_
#define MORIARTY_SRC_INTERNAL_ANALYSIS_BOOTSTRAP_H_

#include <optional>
#include <string>

#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"

namespace moriarty {
namespace moriarty_internal {

// AllVariablesSatisfyConstraints()
//
// Determines if all variable constraints specified here have a corresponding
// value that satisfies the constraints. If ok, returns nullopt. If not ok,
// returns a string describing the issue.
//
// If a variable does not have a value, this will return not ok.
// If a value does not have a variable, this will return ok.
[[nodiscard]] std::optional<std::string> AllVariablesSatisfyConstraints(
    const VariableSet& variables, const ValueSet& values);

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_ANALYSIS_BOOTSTRAP_H_
