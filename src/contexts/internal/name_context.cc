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

#include <functional>
#include <iostream>
#include <string>
#include <string_view>

namespace moriarty {
namespace moriarty_internal {

NameContext::NameContext(std::string_view variable_name)
    : name_(variable_name) {}

NameContext NameContext::ForSubVariable(
    std::string_view sub_variable_name) const {
  NameContext sub_ctx(sub_variable_name);
  sub_ctx.parent_ = this;
  return sub_ctx;
}

NameContext NameContext::ForIndexedSubVariable(
    std::function<std::string(int index)> indexed_name_fn, int index) const {
  NameContext indexed_ctx("");
  indexed_ctx.indexed_name_fn_ = std::move(indexed_name_fn);
  indexed_ctx.indexed_name_fn_index_ = index;
  indexed_ctx.parent_ = this;
  return indexed_ctx;
}

std::string NameContext::GetVariableName() const {
  if (indexed_name_fn_) return indexed_name_fn_(indexed_name_fn_index_);
  return name_;
}

std::vector<std::string> NameContext::GetVariableStack() const {
  std::vector<std::string> stack;
  const NameContext* current = this;
  while (current != nullptr) {
    std::cout << "N " << current->name_ << std::endl;
    std::cout << "# " << current->indexed_name_fn_index_ << std::endl;
    stack.push_back(current->GetVariableName());
    current = current->parent_;
  }
  std::reverse(stack.begin(), stack.end());
  return stack;
}

}  // namespace moriarty_internal
}  // namespace moriarty
