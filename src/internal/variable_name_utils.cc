// Copyright 2024 Google LLC
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

#include "src/internal/variable_name_utils.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"

namespace moriarty {
namespace moriarty_internal {

std::string_view BaseVariableName(std::string_view variable_name) {
  return variable_name.substr(0, variable_name.find('.'));
}

std::string ConstructVariableName(std::string_view base_variable_name,
                                  std::string_view subvariable_name) {
  return absl::StrCat(base_variable_name, ".", subvariable_name);
}

std::string ConstructVariableName(
    const VariableNameBreakdown& variable_name_breakdown) {
  if (variable_name_breakdown.subvariable_name.has_value()) {
    return absl::StrCat(variable_name_breakdown.base_variable_name, ".",
                        *variable_name_breakdown.subvariable_name);
  }
  return variable_name_breakdown.base_variable_name;
}

VariableNameBreakdown CreateVariableNameBreakdown(
    std::string_view variable_name) {
  VariableNameBreakdown ret;
  ret.base_variable_name = BaseVariableName(variable_name);
  std::optional<std::string_view> subvariable_name =
      SubvariableName(variable_name);
  if (subvariable_name.has_value()) {
    ret.subvariable_name = *subvariable_name;
  }
  return ret;
}

bool HasSubvariable(std::string_view variable_name) {
  return absl::StrContains(variable_name, '.');
}

std::optional<std::string_view> SubvariableName(
    std::string_view variable_name) {
  auto pos = variable_name.find('.');
  if (pos == std::string_view::npos) {
    return std::nullopt;
  }

  return variable_name.substr(pos + 1);
}

void ValidateVariableName(std::string_view name) {
  if (name.empty()) {
    throw std::invalid_argument("Variable name cannot be empty");
  }
  for (char c : name) {
    if (!absl::ascii_isalnum(c) && c != '_') {
      throw std::invalid_argument(
          "Variable name can only contain 'A-Za-z0-9_'.");
    }
  }

  if (!absl::ascii_isalpha(name[0])) {
    throw std::invalid_argument(
        "Variable name must start with an alphabetic character");
  }
}

}  // namespace moriarty_internal
}  // namespace moriarty
