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

#include "src/contexts/internal/mutable_values_context.h"

#include "src/internal/value_set.h"
#include "src/librarian/util/ref.h"

namespace moriarty {
namespace moriarty_internal {

MutableValuesContext::MutableValuesContext(Ref<ValueSet> values)
    : values_(values) {}

void MutableValuesContext::EraseValue(std::string_view variable_name) {
  values_.get().Erase(variable_name);
}

}  // namespace moriarty_internal
}  // namespace moriarty
