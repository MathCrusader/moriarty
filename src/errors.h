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

#ifndef MORIARTY_SRC_ERRORS_H_
#define MORIARTY_SRC_ERRORS_H_

// Moriarty Error Space
//
// Moriarty uses `absl::Status` as its canonical way to pass errors.
//
// In several locations throughout Moriarty, it is required that a Moriarty
// Error is used instead of a generic `absl::Status`. To create a Moriarty
// Error, you must call an `FooError()` function below. And you can check if a
// status is a specific  error using `IsFooError`. There are googletest matchers
// available in testing/status_test_util.h for unit tests (named `IsFoo`).
//
// To be clear: a Moriarty status is an `absl::Status` with information
// injected into it. You should not depend on any message or implementation
// details of how we are dealing with these. They may (and likely will) change
// in the future.
//
// Types of Errors (this list may grow in the future):
//
//  * NonRetryableGenerationError
//  * RetryableGenerationError
//  * UnsatisfiedConstraintError
//
// Exceptions:
//  * ValueNotFound
//  * VariableNotFound

#include <format>
#include <stdexcept>
#include <string>
#include <string_view>

#include "absl/status/status.h"

namespace moriarty {

// IsMoriartyError()
//
// Returns true if `status` is not `OkStatus()` and was generated via Moriarty.
bool IsMoriartyError(const absl::Status& status);

// -----------------------------------------------------------------------------
//   UnsatisfiedConstraint -- Some constraint on a variable was not satisfied.

// IsUnsatisfiedConstraintError()
//
// Returns true if `status` signals that a constraint on some variable was not
// satisfied.
bool IsUnsatisfiedConstraintError(const absl::Status& status);

// UnsatisfiedConstraintError()
//
// Returns a status that states that this constraint is not satisfied. The
// `constraint_explanation` will be shown to the user if requested.
absl::Status UnsatisfiedConstraintError(
    std::string_view constraint_explanation);

// CheckConstraint()
//
// Convenience function basically equivalent to:
//  if (constraint.ok()) return absl::OkStatus();
//  return UnsatisfiedConstraintError(
//                               constraint_explanation + constraint.message());
//
// Useful when mixed with MORIARTY_RETURN_IF_ERROR.
absl::Status CheckConstraint(const absl::Status& constraint,
                             std::string_view constraint_explanation);

// CheckConstraint()
//
// Convenience function basically equivalent to:
//  if (constraint) return absl::OkStatus();
//  return UnsatisfiedConstraintError(constraint_explanation);
//
// Useful when mixed with MORIARTY_RETURN_IF_ERROR.
absl::Status CheckConstraint(bool constraint,
                             std::string_view constraint_explanation);

// -----------------------------------------------------------------------------
//   Exceptions

// ValueNotFound
//
// Thrown when the user asks about a value that is not known. This does not
// imply anything about if the variable is known.
class ValueNotFound : public std::logic_error {
 public:
  explicit ValueNotFound(std::string_view variable_name)
      : std::logic_error(
            std::format("Value for `{}` not found", variable_name)),
        variable_name_(variable_name) {}

  const std::string& VariableName() const { return variable_name_; }

 private:
  std::string variable_name_;
};

// VariableNotFound
//
// Thrown when the user asks about a variable that is not known. For the most
// part, named variables are created via the `Moriarty` class.
class VariableNotFound : public std::logic_error {
 public:
  explicit VariableNotFound(std::string_view variable_name)
      : std::logic_error(std::format("Variable `{}` not found", variable_name)),
        variable_name_(variable_name) {}

  const std::string& VariableName() const { return variable_name_; }

 private:
  std::string variable_name_;
};

// MVariableTypeMismatch
//
// Thrown when the user attempts to cast an MVariable to one of the wrong type.
class MVariableTypeMismatch : public std::logic_error {
 public:
  explicit MVariableTypeMismatch(std::string_view converting_from,
                                 std::string_view converting_to)
      : std::logic_error(std::format("Cannot convert {} to {}", converting_from,
                                     converting_to)),
        from_(converting_from),
        to_(converting_to) {}

  const std::string& ConvertingFrom() const { return from_; }
  const std::string& ConvertingTo() const { return to_; }

 private:
  std::string from_;
  std::string to_;
};

// ValueTypeMismatch
//
// Thrown when the user attempts to cast a value that has been stored using the
// wrong MVariable type. E.g., attempting to read an std::string using MInteger.
class ValueTypeMismatch : public std::logic_error {
 public:
  explicit ValueTypeMismatch(std::string_view variable_name,
                             std::string_view incompatible_type)
      : std::logic_error(
            std::format("Cannot convert the value of `{}` into {}::value_type",
                        variable_name, incompatible_type)),
        name_(variable_name),
        type_(incompatible_type) {}

  const std::string& Name() const { return name_; }
  const std::string& Type() const { return type_; }

 private:
  std::string name_;
  std::string type_;
};

}  // namespace moriarty

#endif  // MORIARTY_SRC_ERRORS_H_
