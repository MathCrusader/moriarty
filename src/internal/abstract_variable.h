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

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/substitute.h"

namespace moriarty::librarian {
class AnalysisContext;    // Forward declaring AnalysisContext
class AssignmentContext;  // Forward declaring AssignmentContext
class PrinterContext;     // Forward declaring PrinterContext
class ReaderContext;      // Forward declaring ReaderContext
class ResolverContext;    // Forward declaring ResolverContext
}  // namespace moriarty::librarian

namespace moriarty::moriarty_internal {
class MutableValuesContext;  // Forward declaring MutableValuesContext
}  // namespace moriarty::moriarty_internal

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

  // AssignValue() [pure virtual]
  //
  // Given all current constraints, assigns a specific value to this variable
  // in `ctx`.
  //
  // Note that the variable stored in `ctx` with the same name may or may not be
  // identically `this` variable, but it should be assumed to be equivalent.
  virtual absl::Status AssignValue(librarian::ResolverContext ctx) const = 0;

  // AssignUniqueValue() [pure virtual]
  //
  // Determines if there is exactly one value that this variable can be assigned
  // to. If so, assigns that value.
  //
  // If there is not a unique value (or it is too hard to determine that there
  // is a unique value), this does nothing.
  //
  // Example: MInteger().Between(7, 7) might be able to determine that its
  // unique value is 7.
  virtual absl::Status AssignUniqueValue(
      librarian::AssignmentContext ctx) const = 0;

  // ReadValue() [pure virtual]
  //
  // Reads a value from `ctx` using the constraints of this variable to
  // determine formatting, etc. Stores the value in `values_ctx`.
  virtual absl::Status ReadValue(
      librarian::ReaderContext ctx,
      moriarty_internal::MutableValuesContext values_ctx) const = 0;

  // PrintValue() [pure virtual]
  //
  // Prints the value of this variable to `ctx` using the constraints on this
  // variable to determine formatting, etc.
  virtual absl::Status PrintValue(librarian::PrinterContext ctx) const = 0;

  // MergeFrom() [pure virtual]
  //
  // Merges my current constraints with the constraints of the `other` variable.
  // The merge should act as an intersection of the two constraints. If one says
  // 1 <= x <= 10 and the other says 5 <= x <= 20, then then merged version
  // should have 5 <= x <= 10.
  virtual absl::Status MergeFrom(const AbstractVariable& other) = 0;

  // ValueSatisfiesConstraints() [pure virtual]
  //
  // Determines if the value stored in `ctx` satisfies all constraints for this
  // variable.
  //
  // If a variable does not have a value, this will return not ok.
  // If a value does not have a variable, this will return ok.
  virtual absl::Status ValueSatisfiesConstraints(
      librarian::AnalysisContext ctx) const = 0;

  // GetDifficultAbstractVariables() [pure virtual]
  //
  // Returns a list of smart points to the difficult instances of this abstract
  // variable.
  virtual absl::StatusOr<
      std::vector<std::unique_ptr<moriarty_internal::AbstractVariable>>>
  GetDifficultAbstractVariables(librarian::AnalysisContext ctx) const = 0;

  // GetDependencies() [pure virtual]
  //
  // Returns a list of variable names that this variable depends on.
  virtual std::vector<std::string> GetDependencies() const = 0;
};

}  // namespace moriarty_internal

namespace moriarty_internal {

// ConvertTo<>
//
//  Converts an AbstractVariable to some derived variable type. Returns
//  kInvalidArgument if it is not convertible.
template <typename Type>
absl::StatusOr<Type> ConvertTo(AbstractVariable* var, std::string_view name) {
  Type* typed_var = dynamic_cast<Type*>(var);
  if (typed_var == nullptr)
    return absl::InvalidArgumentError(
        absl::Substitute("Unable to convert `$0` from $1 to $2", name,
                         var->Typename(), Type().Typename()));
  return *typed_var;
}

// ConvertTo<>
//
//  Converts an AbstractVariable to some derived variable type. Returns
//  kInvalidArgument if it is not convertible.
template <typename Type>
absl::StatusOr<Type> ConvertTo(const AbstractVariable* var,
                               std::string_view name) {
  const Type* typed_var = dynamic_cast<const Type*>(var);
  if (typed_var == nullptr)
    return absl::InvalidArgumentError(
        absl::Substitute("Unable to convert `$0` from $1 to $2", name,
                         var->Typename(), Type().Typename()));
  return *typed_var;
}

}  // namespace moriarty_internal

}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_ABSTRACT_VARIABLE_H_
