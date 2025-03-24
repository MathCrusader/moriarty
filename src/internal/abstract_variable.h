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

#ifndef MORIARTY_SRC_INTERNAL_ABSTRACT_VARIABLE_H_
#define MORIARTY_SRC_INTERNAL_ABSTRACT_VARIABLE_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace moriarty::moriarty_internal {
class ValueSet;           // Forward declaration
class VariableSet;        // Forward declaration
class RandomEngine;       // Forward declaration
class GenerationHandler;  // Forward declaration
}  // namespace moriarty::moriarty_internal

namespace moriarty {
enum class WhitespaceStrictness;  // Forward declaration
}

namespace moriarty::librarian {
template <typename V, typename G>
class MVariable;  // Forward declaration
}

// Determines if T is a Moriarty Variable. Examples: MInteger, MString, etc.
template <typename T>
concept MoriartyVariable = requires {
  requires T::is_moriarty_variable;
  typename T::value_type;
};

namespace moriarty {
namespace moriarty_internal {

// AbstractVariable
//
// This class should not be directly derived from. See `MVariable<>`.
// Most users should not need any context about this class's existence.
//
// This is the top of the variable hierarchy. Each instance of AbstractVariable
// is a single variable (it is the `x` in `auto x = 5;`). This variable will
// contain its own constraints as well as have knowledge of other variables.
// This is needed so the variables can interact. For example "I am
// a string S, and my length is 2*N, where N is another AbstractVariable."
class AbstractVariable {
 public:
  virtual ~AbstractVariable() = default;

 protected:
  // Only derived classes should make copies of me directly to avoid accidental
  // slicing. Call derived class's constructors instead.
  AbstractVariable() = default;
  AbstractVariable(const AbstractVariable&) = default;
  AbstractVariable(AbstractVariable&&) = default;
  AbstractVariable& operator=(const AbstractVariable&) = default;
  AbstractVariable& operator=(AbstractVariable&&) = default;

 public:
  // Typename() [pure virtual]
  //
  // Returns a string representing the name of this type (for example,
  // "MInteger"). This is mostly used for debugging/error messages.
  virtual std::string Typename() const = 0;

  // Clone() [pure virtual]
  //
  // Create a copy and return a pointer to the newly created object. Ownership
  // of the returned pointer is passed to the caller.
  virtual std::unique_ptr<AbstractVariable> Clone() const = 0;

  // ToString() [pure virtual]
  //
  // Returns a string representation of the constraints this variable has.
  virtual std::string ToString() const = 0;

  // AssignValue() [pure virtual]
  //
  // Given all current constraints, assigns a specific value to this variable
  // in `ctx`.
  //
  // Note that the variable stored in `ctx` with the same name may or may not be
  // identically `this` variable, but it should be assumed to be equivalent.
  virtual void AssignValue(
      std::string_view variable_name,
      std::reference_wrapper<const VariableSet> variables,
      std::reference_wrapper<ValueSet> values,
      std::reference_wrapper<RandomEngine> engine,
      std::reference_wrapper<GenerationHandler> handler) const = 0;

  // AssignUniqueValue() [pure virtual]
  //
  // Determines if there is exactly one value that this variable can be assigned
  // to. If so, assigns that value.
  //
  // If there is not a unique value (or it is too hard to determine that there
  // is a unique value), this does nothing.
  //
  // Example: MInteger(Between(7, 7)) might be able to determine that its
  // unique value is 7.
  virtual void AssignUniqueValue(
      std::string_view variable_name,
      std::reference_wrapper<const VariableSet> variables,
      std::reference_wrapper<ValueSet> values) const = 0;

  // ReadValue() [pure virtual]
  //
  // Reads a value from `ctx` using the constraints of this variable to
  // determine formatting, etc. Stores the value in `values_ctx`.
  virtual void ReadValue(std::string_view variable_name,
                         std::reference_wrapper<std::istream> is,
                         WhitespaceStrictness whitespace_strictness,
                         std::reference_wrapper<const VariableSet> variables,
                         std::reference_wrapper<ValueSet> values) const = 0;

  // PrintValue() [pure virtual]
  //
  // Prints the value of this variable to `ctx` using the constraints on this
  // variable to determine formatting, etc.
  virtual void PrintValue(
      std::string_view variable_name, std::reference_wrapper<std::ostream> os,
      std::reference_wrapper<const VariableSet> variables,
      std::reference_wrapper<const ValueSet> values) const = 0;

  // MergeFromAnonymous() [pure virtual]
  //
  // Merges my current constraints with the constraints of the `other` variable.
  // The merge should act as an intersection of the two constraints. If one says
  // 1 <= x <= 10 and the other says 5 <= x <= 20, then then merged version
  // should have 5 <= x <= 10.
  virtual void MergeFromAnonymous(const AbstractVariable& other) = 0;

  // IsSatisfiedWithValue() [pure virtual]
  //
  // Determines if the value stored in `ctx` satisfies all constraints for this
  // variable.
  //
  // If a variable does not have a value, this will return false.
  // If a value does not have a variable, this will return true.
  virtual bool IsSatisfiedWithValue(
      std::string_view variable_name,
      std::reference_wrapper<const VariableSet> variables,
      std::reference_wrapper<const ValueSet> values) const = 0;

  // UnsatisfiedWithValueReason() [pure virtual]
  //
  // Determines if the value stored in `ctx` satisfies all constraints for this
  // variable.
  //
  // If a variable does not have a value, this will return not ok.
  // If a value does not have a variable, this will return ok.
  virtual std::string UnsatisfiedWithValueReason(
      std::string_view variable_name,
      std::reference_wrapper<const VariableSet> variables,
      std::reference_wrapper<const ValueSet> values) const = 0;

  // ListAnonymousEdgeCases() [pure virtual]
  //
  // Returns a list of pointers to the edge cases of this variable.
  virtual std::vector<std::unique_ptr<AbstractVariable>> ListAnonymousEdgeCases(
      std::string_view variable_name,
      std::reference_wrapper<const VariableSet> variables,
      std::reference_wrapper<const ValueSet> values) const = 0;

  // GetDependencies() [pure virtual]
  //
  // Returns a list of variable names that this variable depends on.
  virtual std::vector<std::string> GetDependencies() const = 0;
};

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_ABSTRACT_VARIABLE_H_
