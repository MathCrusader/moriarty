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

#include <format>
#include <stdexcept>
#include <string>
#include <string_view>

namespace moriarty {

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
