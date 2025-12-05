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

#ifndef MORIARTY_SRC_VARIABLES_CONSTRAINTS_CONSTRAINT_VIOLATION_H_
#define MORIARTY_SRC_VARIABLES_CONSTRAINTS_CONSTRAINT_VIOLATION_H_

#include <optional>
#include <ostream>
#include <string>

namespace moriarty {

// A status indicator whether a constraint is violated or not.
class ConstraintViolation {
 public:
  explicit ConstraintViolation(std::string reason) : reason_(reason) {}
  static ConstraintViolation None() { return ConstraintViolation(0); }

  // Implicit conversion that returns true if the constraint is violated, false
  // otherwise.
  operator bool() const { return reason_.has_value(); }

  // Returns true if the constraint is satisfied (i.e., not violated).
  [[nodiscard]] bool IsOk() const { return !reason_.has_value(); }

  // Returns the reason for the constraint violation.
  //
  // NOTE: It is intended that this function is only called if the constraint is
  // violated. If the constraint is not violated, this will return an empty
  // string. But note that an empty string does not mean the constraint is
  // satisfied.
  [[nodiscard]] std::string Reason() const { return reason_.value_or(""); }

  friend std::ostream& operator<<(std::ostream& os,
                                  const ConstraintViolation& violation) {
    if (violation.IsOk()) return os << "Constraint is satisfied";
    return os << "Constraint violation: " << violation.Reason();
  }

 private:
  ConstraintViolation(int) : reason_(std::nullopt) {}
  std::optional<std::string> reason_;
};

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_CONSTRAINTS_CONSTRAINT_VIOLATION_H_
