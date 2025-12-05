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

#ifndef MORIARTY_SRC_CONTEXTS_INTERNAL_VIEW_ONLY_CONTEXT_H_
#define MORIARTY_SRC_CONTEXTS_INTERNAL_VIEW_ONLY_CONTEXT_H_

#include <cstdint>
#include <optional>
#include <string_view>

#include "src/internal/abstract_variable.h"
#include "src/internal/expressions.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/util/ref.h"

namespace moriarty {
namespace moriarty_internal {

// ViewOnlyContext
//
// Allows you to inspect the current state of the variables and values.
// ViewOnlyContext is read-only. It does not allow you to modify the variables
// or values.
class ViewOnlyContext {
 public:
  explicit ViewOnlyContext(Ref<const VariableSet> variables,
                           Ref<const ValueSet> values);

  // GetVariable()
  //
  // Returns the stored variable with the name `variable_name`.
  template <MoriartyVariable T>
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
  template <MoriartyVariable T>
  [[nodiscard]] T::value_type GetValue(std::string_view variable_name) const;

  // GetValueIfKnown()
  //
  // Returns the stored value for the variable with the name `variable_name` if
  // it has been set previously.
  template <MoriartyVariable T>
  [[nodiscard]] std::optional<typename T::value_type> GetValueIfKnown(
      std::string_view variable_name) const;

  // GetUniqueValue()
  //
  // Determines if there is exactly one value that this variable can be assigned
  // to. If so, returns that value. If there is not a unique value (or it is too
  // hard to determine that there is a unique value), returns `std::nullopt`.
  // Returning `std::nullopt` does not guarantee there is not a unique value,
  // it may just be too hard to determine it.
  template <MoriartyVariable T>
  [[nodiscard]] std::optional<typename T::value_type> GetUniqueValue(
      std::string_view variable_name) const;

  // GetUniqueInteger()
  //
  // Same as GetUniqueValue(), but only for integer variables. The system does
  // not necessarily know that this variable is an integer.
  [[nodiscard]] std::optional<int64_t> GetUniqueInteger(
      std::string_view variable_name) const;

  // ValueIsKnown()
  //
  // Returns true if the value for the variable with the name `variable_name` is
  // set to some value.
  [[nodiscard]] bool ValueIsKnown(std::string_view variable_name) const;

  // IsSatisfiedWith()
  //
  // Determines if `value` satisfies the constraints in `variable`. You may use
  // other known variables in your constraints. Examples:
  //
  //   bool s1 = IsSatisfiedWith(MInteger(AtMost("N")), 25);
  //   bool s2 = IsSatisfiedWith(MString(Length(5)), "hello");
  template <MoriartyVariable T>
  [[nodiscard]] bool IsSatisfiedWith(T variable,
                                     const T::value_type& value) const;

  // ListVariables()
  //
  // Returns all variables in the context. Prefer to not use this function. It
  // may be deprecated in the future.
  [[nodiscard]] const VariableSet::Map& ListVariables() const;

  // EvaluateExpression()
  //
  // Evaluates the given expression in the current context.
  [[nodiscard]] int64_t EvaluateExpression(const Expression& expr) const;

  // UnsafeGetVariables()
  //
  // Advanced usage only. Normal users should not use this function.
  //
  // Returns a reference to the internal variable set.
  [[nodiscard]] const VariableSet& UnsafeGetVariables() const;

  // UnsafeGetValues()
  //
  // Advanced usage only. Normal users should not use this function.
  //
  // Returns a reference to the internal value set.
  [[nodiscard]] const ValueSet& UnsafeGetValues() const;

 private:
  Ref<const VariableSet> variables_;
  Ref<const ValueSet> values_;
};

template <MoriartyVariable T>
T ViewOnlyContext::GetVariable(std::string_view variable_name) const {
  return variables_.get().GetVariable<T>(variable_name);
}

template <MoriartyVariable T>
T::value_type ViewOnlyContext::GetValue(std::string_view variable_name) const {
  return values_.get().Get<T>(variable_name);
}

template <MoriartyVariable T>
std::optional<typename T::value_type> ViewOnlyContext::GetValueIfKnown(
    std::string_view variable_name) const {
  if (values_.get().Contains(variable_name)) {
    return values_.get().Get<T>(variable_name);
  }
  return std::nullopt;
}

template <MoriartyVariable T>
std::optional<typename T::value_type> ViewOnlyContext::GetUniqueValue(
    std::string_view variable_name) const {
  if (auto stored_value = GetValueIfKnown<T>(variable_name))
    return *stored_value;

  T variable = GetVariable<T>(variable_name);
  return variable.GetUniqueValue({variable_name, variables_, values_});
}

template <MoriartyVariable T>
bool ViewOnlyContext::IsSatisfiedWith(T variable,
                                      const T::value_type& value) const {
  return variable.CheckValue({"IsSatisfiedWith()", variables_, values_}, value)
      .IsOk();
}

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_INTERNAL_VIEW_ONLY_CONTEXT_H_
