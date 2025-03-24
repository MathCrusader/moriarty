/*
 * Copyright 2025 Darcy Best
 * Copyright 2024 Google LLC
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

#ifndef MORIARTY_SRC_INTERNAL_VALUE_SET_H_
#define MORIARTY_SRC_INTERNAL_VALUE_SET_H_

#include <any>
#include <string>
#include <string_view>
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "src/internal/abstract_variable.h"
#include "src/librarian/errors.h"

namespace moriarty {
namespace moriarty_internal {

// ValueSet
//
// A cookie jar that simply stores type-erased values. You can only put things
// in and take them out. Nothing more. Logically equivalent to
//
//   std::map<std::string, std::any>
class ValueSet {
 public:
  // Set()
  //
  // Sets variable_name to `value`. If this was set previously, it will be
  // overwritten.
  template <MoriartyVariable T>
  void Set(std::string_view variable_name, T::value_type value);

  // UnsafeSet()
  //
  // Sets `variable_name` to `value`. If this was set previously, it will be
  // overwritten.
  //
  // NOTE: The type of `value` is expected to be `T::value_type` for the `T`
  // corresponding to `variable_name`. If not, expect undefined behaviour.
  //
  // UnsafeSet does not behave properly with the approximate size. It is always
  // considered to be of size 1.
  void UnsafeSet(std::string_view variable_name, std::any value);

  // Get()
  //
  // Returns the stored value for the variable `variable_name`.
  //
  //  * If `variable_name` is non-existent, throws `ValueNotFound`.
  //  * If the value cannot be converted to T, returns kFailedPrecondition.
  template <MoriartyVariable T>
  [[nodiscard]] T::value_type Get(std::string_view variable_name) const;

  // UnsafeGet()
  //
  // Returns the std::any value for the variable `variable_name`. Should only be
  // used when the value type is unknown.
  //
  // Throws moriarty::ValueNotFound if the variable has not been set.
  [[nodiscard]] std::any UnsafeGet(std::string_view variable_name) const;

  // Contains()
  //
  // Determines if `variable_name` is in this ValueSet.
  [[nodiscard]] bool Contains(std::string_view variable_name) const;

  // Erase()
  //
  // Deletes the stored value for the variable `variable_name`. If
  // `variable_name` is non-existent, this is a no-op.
  void Erase(std::string_view variable_name);

 private:
  absl::flat_hash_map<std::string, std::any> values_;
};

// -----------------------------------------------------------------------------
//  Template implementation below

template <MoriartyVariable T>
T::value_type ValueSet::Get(std::string_view variable_name) const {
  auto it = values_.find(variable_name);
  if (it == values_.end()) throw ValueNotFound(variable_name);

  using TV = typename T::value_type;
  const TV* val = std::any_cast<const TV>(&it->second);
  if (val == nullptr) {
    throw moriarty::ValueTypeMismatch(variable_name, T().Typename());
  }

  return *val;
}
template <MoriartyVariable T>
void ValueSet::Set(std::string_view variable_name, T::value_type value) {
  values_[variable_name] = std::move(value);
}

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_VALUE_SET_H_
