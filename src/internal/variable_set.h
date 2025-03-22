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

// The base atom Moriarty is a collection of variables, not just a single
// variable.

#ifndef MORIARTY_SRC_INTERNAL_VARIABLE_SET_H_
#define MORIARTY_SRC_INTERNAL_VARIABLE_SET_H_

#include <concepts>
#include <memory>
#include <string>
#include <string_view>

#include "absl/container/flat_hash_map.h"
#include "src/internal/abstract_variable.h"
#include "src/librarian/conversions.h"
#include "src/librarian/errors.h"

namespace moriarty {
namespace moriarty_internal {

// VariableSet
//
// A collection of (possibly interacting) variables. Constraints that reference
// other variables must be in the same `VariableSet` instance.
class VariableSet {
 public:
  // Rule of 5, plus swap
  VariableSet() = default;
  ~VariableSet() = default;
  VariableSet(const VariableSet& other);      // Copy
  VariableSet& operator=(VariableSet other);  // Assignment (both copy and move)
  VariableSet(VariableSet&& other) noexcept = default;  // Move
  void Swap(VariableSet& other);

  // Returns true if the variable exists in the collection.
  [[nodiscard]] bool Contains(std::string_view name) const;

  // Sets the value of a specific variable. Overwrites any existing variable.
  void SetVariable(std::string_view name, const AbstractVariable& variable);

  // Adds an AbstractVariable to the collection, if it doesn't exist, or merges
  // it into the existing variable if it does.
  void AddOrMergeVariable(std::string_view name,
                          const AbstractVariable& variable);

  // GetAnonymousVariable()
  //
  // Returns a pointer to the variable. Ownership of this pointer is *not*
  // transferred to the caller.
  //
  // Throws:
  //  * moriarty::VariableNotFound if the variable does not exist.
  [[nodiscard]] const AbstractVariable* GetAnonymousVariable(
      std::string_view name) const;

  // GetAnonymousVariable()
  //
  // Returns a pointer to the variable. Ownership of this pointer is *not*
  // transferred to the caller.
  //
  // Throws:
  //  * moriarty::VariableNotFound if the variable does not exist.
  [[nodiscard]] AbstractVariable* GetAnonymousVariable(std::string_view name);

  // GetVariable<>()
  //
  // Returns the variable named `name`.
  //
  // Throws:
  //  * moriarty::VariableNotFound if the variable does not exist.
  //
  // Returns: // FIXME: Should throw std::invalid_argument instead of returning
  //  * kInvalidArgument if it is not convertible to `T`
  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  [[nodiscard]] T GetVariable(std::string_view name) const;

  // ListVariables()
  //
  // Returns the map of internal variables.
  [[nodiscard]] const absl::flat_hash_map<std::string,
                                          std::unique_ptr<AbstractVariable>>&
  ListVariables() const;

 private:
  absl::flat_hash_map<std::string, std::unique_ptr<AbstractVariable>>
      variables_;

  // Returns either a pointer to the AbstractVariable or `nullptr` if it doesn't
  // exist.
  const AbstractVariable* GetAnonymousVariableOrNull(
      std::string_view name) const;
  AbstractVariable* GetAnonymousVariableOrNull(std::string_view name);
};

// -----------------------------------------------------------------------------
//  Template implementation below

template <typename T>
  requires std::derived_from<T, AbstractVariable>
T VariableSet::GetVariable(std::string_view name) const {
  const AbstractVariable* var = GetAnonymousVariableOrNull(name);
  if (var == nullptr) throw VariableNotFound(name);
  return librarian::ConvertTo<T>(*var);
}

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_VARIABLE_SET_H_
