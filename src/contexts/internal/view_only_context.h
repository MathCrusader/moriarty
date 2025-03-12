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

#ifndef MORIARTY_SRC_CONTEXTS_INTERNAL_VIEW_ONLY_CONTEXT_H_
#define MORIARTY_SRC_CONTEXTS_INTERNAL_VIEW_ONLY_CONTEXT_H_

#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include "absl/status/statusor.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"

namespace moriarty {
namespace moriarty_internal {

// ViewOnlyContext
//
// Allows you to inspect the current state of the variables and values.
// ViewOnlyContext is read-only. It does not allow you to modify the variables
// or values.
class ViewOnlyContext {
 public:
  explicit ViewOnlyContext(std::reference_wrapper<const VariableSet> variables,
                           std::reference_wrapper<const ValueSet> values);

  // GetVariable()
  //
  // Returns the stored variable with the name `variable_name`.
  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  [[nodiscard]] T GetVariable(std::string_view variable_name) const;

  // GetAnonymousVariable()
  //
  // Returns the stored variable with the name `variable_name`. Only use this
  // function if you do not know the type of the variable at the call-site.
  // Prefer `GetVariable()`.
  [[nodiscard]] const AbstractVariable& GetAnonymousVariable(
      std::string_view variable_name) const;

  // GetValue()
  //
  // Returns the stored value for the variable with the name `variable_name`.
  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  [[nodiscard]] T::value_type GetValue(std::string_view variable_name) const;

  // GetValueIfKnown()
  //
  // Returns the stored value for the variable with the name `variable_name` if
  // it has been set previously.
  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  [[nodiscard]] std::optional<typename T::value_type> GetValueIfKnown(
      std::string_view variable_name) const;

  // GetUniqueValue()
  //
  // Determines if there is exactly one value that this variable can be assigned
  // to. If so, returns that value. If there is not a unique value (or it is too
  // hard to determine that there is a unique value), returns `std::nullopt`.
  // Returning `std::nullopt` does not guarantee there is not a unique value,
  // it may just be too hard to determine it.
  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  [[nodiscard]] std::optional<typename T::value_type> GetUniqueValue(
      std::string_view variable_name) const;

  // ValueIsKnown()
  //
  // Returns true if the value for the variable with the name `variable_name` is
  // set to some value.
  [[nodiscard]] bool ValueIsKnown(std::string_view variable_name) const;

  // SatisfiesConstraints()
  //
  // Checks if `value` satisfies the constraints in `variable`. Returns a
  // non-ok status with the reason if not. You may use other known variables in
  // your constraints. Examples:
  //
  //   absl::Status s1 = SatisfiesConstraints(MInteger(AtMost("N")), 25);
  //   absl::Status s2 = SatisfiesConstraints(MString(Length(5)), "hello");
  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  absl::Status SatisfiesConstraints(T variable,
                                    const T::value_type& value) const;

  // GetAllVariables()
  //
  // Returns all variables in the context. Prefer to not use this function. It
  // may be deprecated in the future.
  [[nodiscard]] const absl::flat_hash_map<std::string,
                                          std::unique_ptr<AbstractVariable>>&
  GetAllVariables() const;

 private:
  std::reference_wrapper<const VariableSet> variables_;
  std::reference_wrapper<const ValueSet> values_;
};

}  // namespace moriarty_internal
}  // namespace moriarty

// ----------------------------------------------------------------------------
// Forward declarations

namespace moriarty::librarian {
template <typename V, typename G>
class MVariable;  // Forward declaring MVariable
}

namespace moriarty::moriarty_internal {
class ViewOnlyContext;  // Forward declaring
// Forward declaring GetUniqueValue
template <typename V, typename G>
std::optional<G> GetUniqueValue(const librarian::MVariable<V, G>&,
                                std::string_view, ViewOnlyContext);
// Forward declaring SatisfiesConstraints
template <typename V, typename G>
absl::Status SatisfiesConstraints(const librarian::MVariable<V, G>&,
                                  std::string_view, ViewOnlyContext, const G&);
}  // namespace moriarty::moriarty_internal

namespace moriarty {
namespace moriarty_internal {

template <typename T>
  requires std::derived_from<T, AbstractVariable>
T ViewOnlyContext::GetVariable(std::string_view variable_name) const {
  auto variable = variables_.get().GetVariable<T>(variable_name);
  if (!variable.ok()) throw std::runtime_error(variable.status().ToString());
  return *variable;
}

template <typename T>
  requires std::derived_from<T, AbstractVariable>
T::value_type ViewOnlyContext::GetValue(std::string_view variable_name) const {
  auto value = values_.get().Get<T>(variable_name);
  if (!value.ok()) throw std::runtime_error(value.status().ToString());
  return *value;
}

template <typename T>
  requires std::derived_from<T, AbstractVariable>
std::optional<typename T::value_type> ViewOnlyContext::GetValueIfKnown(
    std::string_view variable_name) const {
  try {
    // FIXME: Create a MaybeGet() function in ValueSet
    absl::StatusOr<typename T::value_type> value =
        values_.get().Get<T>(variable_name);
    if (value.ok()) return *value;
  } catch (const ValueNotFound&) {
    return std::nullopt;
  }

  return std::nullopt;
}

template <typename T>
  requires std::derived_from<T, AbstractVariable>
std::optional<typename T::value_type> ViewOnlyContext::GetUniqueValue(
    std::string_view variable_name) const {
  auto stored_value = GetValueIfKnown<T>(variable_name);
  if (stored_value) return *stored_value;

  return moriarty_internal::GetUniqueValue(GetVariable<T>(variable_name),
                                           variable_name, *this);
}

template <typename T>
  requires std::derived_from<T, AbstractVariable>
absl::Status ViewOnlyContext::SatisfiesConstraints(
    T variable, const T::value_type& value) const {
  return moriarty_internal::SatisfiesConstraints(
      std::move(variable), "Context::SatisfiesConstraints()", *this, value);
}

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_INTERNAL_VIEW_ONLY_CONTEXT_H_
