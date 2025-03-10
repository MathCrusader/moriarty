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

#ifndef MORIARTY_SRC_CONTEXTS_INTERNAL_MUTABLE_VALUES_CONTEXT_H_
#define MORIARTY_SRC_CONTEXTS_INTERNAL_MUTABLE_VALUES_CONTEXT_H_

#include <functional>
#include <string_view>

#include "src/internal/abstract_variable.h"
#include "src/internal/value_set.h"

namespace moriarty {
namespace moriarty_internal {

// MutableValuesContext
//
// Allows you to update the values currently stored.
class MutableValuesContext {
 public:
  explicit MutableValuesContext(ValueSet& values);

  // SetValue()
  //
  // Sets the value of `variable_name` to be `value`.
  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  void SetValue(std::string_view variable_name, T::value_type value);

  // EraseValue()
  //
  // Removes `variable_name` from the set of known values.
  void EraseValue(std::string_view variable_name);

 private:
  std::reference_wrapper<ValueSet> values_;
};

// -----------------------------------------------------------------------------
//  Template implementation below

template <typename T>
  requires std::derived_from<T, AbstractVariable>
void MutableValuesContext::SetValue(std::string_view variable_name,
                                    T::value_type value) {
  values_.get().Set<T>(variable_name, std::move(value));
}

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_INTERNAL_MUTABLE_VALUES_CONTEXT_H_
