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

#ifndef MORIARTY_CONTEXTS_INTERNAL_NAME_CONTEXT_H_
#define MORIARTY_CONTEXTS_INTERNAL_NAME_CONTEXT_H_

#include <string>
#include <string_view>

namespace moriarty {
namespace moriarty_internal {

// NameContext
//
// Holds the name of the current variable in question.
class NameContext {
 public:
  explicit NameContext(std::string_view variable_name);

  // GetVariableName()
  //
  // Returns the name of the variable currently being operated on.
  [[nodiscard]] std::string GetVariableName() const;

 private:
  std::string name_;
};

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_CONTEXTS_INTERNAL_NAME_CONTEXT_H_
