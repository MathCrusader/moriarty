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

#ifndef MORIARTY_CONSTRAINTS_CONSTRAINT_VIOLATION_H_
#define MORIARTY_CONSTRAINTS_CONSTRAINT_VIOLATION_H_

#include <memory>
#include <optional>
#include <ostream>
#include <string>

#include "src/internal/value_printer.h"

namespace moriarty {

namespace librarian {
struct Expected {
  std::string value;
  explicit Expected(std::string s) : value(std::move(s)) {}
  template <typename... Args>
  explicit Expected(std::format_string<Args...> fmt, Args&&... args)
      : value(std::format(fmt, std::forward<Args>(args)...)) {}
};

struct Evaluated {
  std::string value;
  explicit Evaluated(std::string s) : value(std::move(s)) {}
  template <typename... Args>
  explicit Evaluated(std::format_string<Args...> fmt, Args&&... args)
      : value(std::format(fmt, std::forward<Args>(args)...)) {}
};

struct Details {
  std::string value;
  explicit Details(std::string s) : value(std::move(s)) {}
  template <typename... Args>
  explicit Details(std::format_string<Args...> fmt, Args&&... args)
      : value(std::format(fmt, std::forward<Args>(args)...)) {}
};

}  // namespace librarian

namespace moriarty_internal {
struct ValidationResultNode {
  struct Details {
    ::moriarty::librarian::Expected expected;
    std::optional<::moriarty::librarian::Evaluated> evaluated;
    std::optional<::moriarty::librarian::Details> details;
  };
  // Either the variable or sub-variable name ("length, "element 5", "A")
  std::string name;
  // A stringified value that caused the violation, for debugging purposes.
  std::string value;
  // Either a string expected/violation or a nested violation node.
  std::variant<Details, std::unique_ptr<ValidationResultNode>> details;
};

std::string PrettyPrintValidationResult(const ValidationResultNode& node);

}  // namespace moriarty_internal

// A status indicator whether a constraint is violated or not.
class [[nodiscard]] ValidationResult {
 public:
  // There is no constraint violation.
  static ValidationResult Ok();
  // There is a direct constraint violation on this variable.
  template <typename T>
  static ValidationResult Violation(
      std::string name, const T& value, librarian::Expected expected,
      std::optional<librarian::Evaluated> evaluated = std::nullopt,
      std::optional<librarian::Details> details = std::nullopt);
  // There is a direct constraint violation on this variable, but it doesn't
  // have a value.
  static ValidationResult Violation(
      std::string name, librarian::Expected expected,
      std::optional<librarian::Evaluated> evaluated = std::nullopt,
      std::optional<librarian::Details> details = std::nullopt);
  // There is an indirect constraint violation on either this variable or the
  // sub-variable. The details from `details` will be nested under `name` and
  // `value` in the message.
  template <typename T>
  static ValidationResult Violation(std::string name, const T& value,
                                    ValidationResult&& details);

  // Returns true if the constraint is satisfied (i.e., not violated).
  [[nodiscard]] bool IsOk() const;

  // Returns the reason for the constraint violation.
  //
  // NOTE: If a reason wasn't provided or the constraint is satisfied, this will
  // return an empty string (empty string does not necessarily mean the
  // constraint is satisfied).
  [[nodiscard]] std::string PrettyReason() const;

  friend std::ostream& operator<<(std::ostream& os,
                                  const ValidationResult& validation);

 private:
  struct InternalConstructorTag {};
  ValidationResult(
      InternalConstructorTag, bool ok,
      std::unique_ptr<moriarty_internal::ValidationResultNode> details);
  bool ok_;
  std::unique_ptr<moriarty_internal::ValidationResultNode> details_;
};

std::ostream& operator<<(std::ostream& os, const ValidationResult& validation);

// A detailed constraint validation contains the violation, as well as the
// variable that violated it.
struct DetailedValidationResult {
  std::string variable_name;
  ValidationResult violation;
};

// ----------------------------------------------------------------------------
//  Template implementations below

template <typename T>
ValidationResult ValidationResult::Violation(
    std::string name, const T& value, librarian::Expected expected,
    std::optional<librarian::Evaluated> evaluated,
    std::optional<librarian::Details> details) {
  return ValidationResult(
      InternalConstructorTag{}, false,
      std::make_unique<moriarty_internal::ValidationResultNode>(
          moriarty_internal::ValidationResultNode{
              std::move(name), moriarty_internal::ValuePrinter(value),
              moriarty_internal::ValidationResultNode::Details{
                  .expected = std::move(expected),
                  .evaluated = std::move(evaluated),
                  .details = std::move(details)}}));
}

template <typename T>
ValidationResult ValidationResult::Violation(std::string name, const T& value,
                                             ValidationResult&& details) {
  return ValidationResult(
      InternalConstructorTag{}, false,
      std::make_unique<moriarty_internal::ValidationResultNode>(
          moriarty_internal::ValidationResultNode{
              std::move(name), moriarty_internal::ValuePrinter(value),
              std::move(details.details_)}));
}

}  // namespace moriarty

#endif  // MORIARTY_CONSTRAINTS_CONSTRAINT_VIOLATION_H_
