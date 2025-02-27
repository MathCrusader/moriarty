/*
 * Copyright 2025 Darcy Best
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

#ifndef MORIARTY_SRC_CONTEXTS_INTERNAL_VARIABLE_NAME_CONTEXT_H_
#define MORIARTY_SRC_CONTEXTS_INTERNAL_VARIABLE_NAME_CONTEXT_H_

#include <functional>
#include <string>

namespace moriarty {
namespace moriarty_internal {

// VariableNameContext
//
// A class to handle the variable name in the current space. This class is
// simply a string-wrapper today, but it is expected to expand in the future to
// handle subvariables nicely.
class VariableNameContext {
 public:
  // TODO: Test that an r-value const& can't be assigned here.
  explicit VariableNameContext(const std::string& variable_name)
      : variable_name_(variable_name) {}

  // GetVariableName()
  //
  // Returns the name of the variable of interest.
  [[nodiscard]] const std::string& GetVariableName() const {
    return variable_name_.get();
  }

 private:
  std::reference_wrapper<const std::string> variable_name_;
};

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_INTERNAL_VARIABLE_NAME_CONTEXT_H_
