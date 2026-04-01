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

#include "moriarty/contexts/internal/variable_random_context.h"

#include "moriarty/internal/random_engine.h"
#include "moriarty/internal/value_set.h"
#include "moriarty/internal/variable_set.h"
#include "moriarty/librarian/util/ref.h"

namespace moriarty {
namespace moriarty_internal {

VariableRandomContext::VariableRandomContext(Ref<const VariableSet> variables,
                                             Ref<const ValueSet> values,
                                             Ref<RandomEngine> engine)
    : variables_(variables), values_(values), engine_(engine) {}

}  // namespace moriarty_internal
}  // namespace moriarty
