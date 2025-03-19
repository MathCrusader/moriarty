/*
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

#ifndef MORIARTY_SRC_VARIABLES_CONSTRAINTS_IO_CONSTRAINTS_H_
#define MORIARTY_SRC_VARIABLES_CONSTRAINTS_IO_CONSTRAINTS_H_

#include <stdexcept>
#include <string>

#include "src/io_config.h"
#include "src/variables/constraints/base_constraints.h"

namespace moriarty {

// Constraint stating that the container should be separated by this whitespace
// when printed or read.
class IOSeparator : public MConstraint {
 public:
  // The separator to use between elements of a container.
  explicit IOSeparator(Whitespace separator);

  static IOSeparator Space();
  static IOSeparator Newline();
  static IOSeparator Tab();

  // Returns the separator to use between elements of a container.
  [[nodiscard]] Whitespace GetSeparator() const;

  // IOConstraints are always satisfied
  [[nodiscard]] bool IsSatisfiedWith(const auto& value) const { return true; }

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns a string explaining why the value does not satisfy the constraints.
  // It is assumed that IsSatisfiedWith() returned false.
  [[nodiscard]] std::string Explanation(const auto& value) const {
    throw std::runtime_error("IOSeparator is always satisfied");
  }

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const;

 private:
  Whitespace separator_;
};

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_CONSTRAINTS_IO_CONSTRAINTS_H_
