// Copyright 2026 Darcy Best
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

#include "src/constraints/constraint_violation.h"

#include <format>
#include <iostream>
#include <memory>
#include <string>
#include <variant>

namespace moriarty {
namespace moriarty_internal {

/*
Example message:

└─ A
   │ which is: [...]
└─ element 5
   │ which is: "abcdef..."
└─ length
   │ which is: 14
Value must be between 1 and 3 * N + 1 (1..13)
*/
std::string PrettyPrintValidationResult(const ValidationResultNode& node) {
  using Dets = moriarty_internal::ValidationResultNode::Details;
  using Node = std::unique_ptr<moriarty_internal::ValidationResultNode>;

  std::string result = std::format("└─ {}\n", node.name);

  if (std::holds_alternative<Dets>(node.details)) {
    const auto& d = std::get<Dets>(node.details);
    result += std::format("   │ expected: {}\n", d.expected.value);
    if (d.evaluated && d.evaluated->value != d.expected.value)
      result += std::format("   │           = {}\n", d.evaluated->value);
    result += std::format("   │      got: {}\n", node.value);
    if (d.details)
      result += std::format("   │  details: {}\n", d.details->value);
  } else {
    result += std::format("   │    value: {}\n", node.value);
    result += PrettyPrintValidationResult(*std::get<Node>(node.details));
  }
  return result;
}

}  // namespace moriarty_internal

ValidationResult::ValidationResult(
    InternalConstructorTag, bool ok,
    std::unique_ptr<moriarty_internal::ValidationResultNode> details)
    : ok_(ok), details_(std::move(details)) {
  if (details_ != nullptr && ok_) {
    throw std::logic_error(
        "ValidationResult cannot be Ok and have details at the same time");
  }
}

ValidationResult ValidationResult::Ok() {
  return ValidationResult(ValidationResult::InternalConstructorTag{}, true,
                          nullptr);
}

ValidationResult ValidationResult::Violation(
    std::string name, librarian::Expected expected,
    std::optional<librarian::Evaluated> evaluated,
    std::optional<librarian::Details> details) {
  return ValidationResult(
      ValidationResult::InternalConstructorTag{}, false,
      std::make_unique<moriarty_internal::ValidationResultNode>(
          moriarty_internal::ValidationResultNode{
              std::move(name), "(none)",
              moriarty_internal::ValidationResultNode::Details{
                  .expected = std::move(expected),
                  .evaluated = std::move(evaluated),
                  .details = std::move(details)}}));
}

bool ValidationResult::IsOk() const { return ok_; }

std::string ValidationResult::PrettyReason() const {
  if (ok_ || details_ == nullptr) return "";
  return moriarty_internal::PrettyPrintValidationResult(*details_);
}

std::ostream& operator<<(std::ostream& os, const ValidationResult& validation) {
  if (validation.IsOk()) return os << "Constraint is satisfied";
  return os << validation.PrettyReason();
}

}  // namespace moriarty
