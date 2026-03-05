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

#include <functional>
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

  // ForSubVariable()
  //
  // Returns a new NameContext for a sub-variable of the current variable.
  //
  // NOTE: The current NameContext must outlive the returned NameContext.
  [[nodiscard]] NameContext ForSubVariable(
      std::string_view sub_variable_name) const;

  // ForIndexedSubVariable()
  //
  // Returns a new NameContext for an indexed sub-variable of the current
  // variable. The indexed_name_fn should take an index and return the name
  // of the indexed sub-variable at that index.
  //
  // This function will be lazily evaluated, meaning that the indexed_name_fn
  // will only be called when GetVariableName() or GetVariableStack() is called
  // on the returned NameContext.
  //
  // NOTE: The current NameContext must outlive the returned NameContext.
  [[nodiscard]] NameContext ForIndexedSubVariable(
      std::function<std::string(int index)> indexed_name_fn, int index) const;

  // GetLocalVariableName()
  //
  // Returns the name of the variable currently being operated on. This does not
  // include the names of parent variables. For example, if the current variable
  // is "X[2].Y", this would return "Y".
  [[nodiscard]] std::string GetLocalVariableName() const;

  // GetVariableStack()
  //
  // Returns a vector of variable names representing the stack of variables
  // leading to the current variable. For example, if the current variable is
  // "X[2].Y", this might return ["X", "index 2", "Y"].
  [[nodiscard]] std::vector<std::string> GetVariableStack() const;

  // GetVariableNameHash()
  //
  // Returns the hash of the full variable name stack. This is used for caching
  // purposes.
  [[nodiscard]] std::string GetVariableNameHash() const;

 private:
  std::string name_;
  std::function<std::string(int)>
      indexed_name_fn_;            // Will be used if non-null.
  int indexed_name_fn_index_ = 0;  // The index to pass to indexed_name_fn_.
  const NameContext* parent_ = nullptr;  // The parent context, if any.
};

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_CONTEXTS_INTERNAL_NAME_CONTEXT_H_
