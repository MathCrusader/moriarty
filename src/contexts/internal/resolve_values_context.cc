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

#include "src/contexts/internal/resolve_values_context.h"

#include <functional>
#include <string_view>

#include "src/internal/abstract_variable.h"
#include "src/internal/generation_handler.h"
#include "src/internal/random_engine.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"

namespace moriarty {
namespace moriarty_internal {

ResolveValuesContext::ResolveValuesContext(
    std::reference_wrapper<const moriarty_internal::VariableSet> variables,
    std::reference_wrapper<moriarty_internal::ValueSet> values,
    std::reference_wrapper<moriarty_internal::RandomEngine> engine,
    std::reference_wrapper<moriarty_internal::GenerationHandler> handler)
    : variables_(variables),
      values_(values),
      engine_(engine),
      handler_(handler) {}

void ResolveValuesContext::AssignVariable(std::string_view variable_name) {
  const moriarty_internal::AbstractVariable* variable =
      variables_.get().GetAnonymousVariable(variable_name);
  variable->AssignValue(variable_name, variables_, values_, engine_, handler_);
}
}  // namespace moriarty_internal
}  // namespace moriarty
