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

#include "absl/status/status.h"
#include "src/contexts/librarian/analysis_context.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/util/status_macro/status_macros.h"

namespace moriarty {
namespace moriarty_internal {

absl::Status AllVariablesSatisfyConstraints(const VariableSet& variables,
                                            const ValueSet& values) {
  for (const auto& [var_name, var_ptr] : variables.GetAllVariables()) {
    librarian::AnalysisContext ctx(var_name, variables, values);
    MORIARTY_RETURN_IF_ERROR(var_ptr->ValueSatisfiesConstraints(ctx))
        << "'" << var_name << "' does not satisfy constraints";
  }
  return absl::OkStatus();
}

}  // namespace moriarty_internal
}  // namespace moriarty
