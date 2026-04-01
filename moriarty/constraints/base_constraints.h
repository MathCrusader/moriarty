// Copyright 2025 Darcy Best
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

#ifndef MORIARTY_CONSTRAINTS_BASE_CONSTRAINTS_H_
#define MORIARTY_CONSTRAINTS_BASE_CONSTRAINTS_H_

#include <string>
#include <string_view>
#include <vector>

namespace moriarty {

// Base class for all constraints in Moriarty.
class MConstraint {
 protected:
  MConstraint() = default;
  MConstraint(const MConstraint&) = default;
  MConstraint(MConstraint&&) = default;
  MConstraint& operator=(const MConstraint&) = default;
  MConstraint& operator=(MConstraint&&) = default;
};

// A basic constraint denotes a property that you have or you don't have. It
// may not depend on any variables. (e.g., connected, prime, even, etc.)
class BasicMConstraint : public MConstraint {
 public:
  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const { return {}; }

  // Returns a human-readable representation of this constraint.
  [[nodiscard]] std::string ToString() const { return description_; }

 protected:
  explicit BasicMConstraint(std::string_view description)
      : description_(description) {}

 private:
  std::string description_;
};

}  // namespace moriarty

#endif  // MORIARTY_CONSTRAINTS_BASE_CONSTRAINTS_H_
