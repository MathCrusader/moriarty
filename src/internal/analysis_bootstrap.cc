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

#include <format>
#include <optional>

#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"

namespace moriarty {
namespace moriarty_internal {

std::optional<std::string> AllVariablesSatisfyConstraints(
    const VariableSet& variables, const ValueSet& values) {
  for (const auto& [name, var] : variables.ListVariables()) {
    if (auto reason = var->CheckValue(name, variables, values))
      return std::format("Variable {} does not satisfy its constraints: {}",
                         name, reason.Reason());
  }
  return std::nullopt;
}

}  // namespace moriarty_internal
}  // namespace moriarty
