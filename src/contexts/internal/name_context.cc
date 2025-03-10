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

#include "src/contexts/internal/name_context.h"

#include <string>
#include <string_view>

namespace moriarty {
namespace moriarty_internal {

std::string NameContext::GetVariableName() const { return name_; }

NameContext::NameContext(std::string_view variable_name)
    : name_(variable_name) {}

}  // namespace moriarty_internal
}  // namespace moriarty
