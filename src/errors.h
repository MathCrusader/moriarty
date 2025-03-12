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
//  * ValueNotFoundError
//
// Exceptions:
//  * VariableNotFound

#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include "absl/status/status.h"

namespace moriarty {

// IsMoriartyError()
//
// Returns true if `status` is not `OkStatus()` and was generated via Moriarty.
bool IsMoriartyError(const absl::Status& status);

// GetUnknownVariableName()
//
// If `status` is a `ValueNotFoundError`, returns the
// name of the variable that wasn't found (if known). If the status is not an
// error listed above or the name is unknown, returns `std::nullopt`.
std::optional<std::string> GetUnknownVariableName(const absl::Status& status);

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
//   ValueNotFound -- when a variable is known, but no value is known for it.

// IsValueNotFoundError()
//
// Returns true if `status` signals that the value of a variable is not known.
bool IsValueNotFoundError(const absl::Status& status);

// ValueNotFoundError()
//
// Returns a status specifying that a specific value for `variable_name` is not
// known. This normally implies that the MVariable with the same name is known,
// but its value is not.
//
// For example, this error indicates that we know an MInteger exists with the
// name "N", but we don't know an integer value for "N".
//
// It is not typical for users to need to use this function.
absl::Status ValueNotFoundError(std::string_view variable_name);

// -----------------------------------------------------------------------------
//   Exceptions

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

}  // namespace moriarty

#endif  // MORIARTY_SRC_ERRORS_H_
