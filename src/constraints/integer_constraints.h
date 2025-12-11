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

#ifndef MORIARTY_CONSTRAINTS_INTEGER_CONSTRAINTS_H_
#define MORIARTY_CONSTRAINTS_INTEGER_CONSTRAINTS_H_

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "src/constraints/base_constraints.h"
#include "src/constraints/constraint_violation.h"
#include "src/contexts/librarian_context.h"
#include "src/internal/expressions.h"

namespace moriarty {

// Constrains the modulus of an integer in relation to another integer.
// The modulus must be positive. The remainder will be reduced mod the modulus.
// So Mod(5, 3) is equivalent to Mod(2, 3).
class Mod : public MConstraint {
 public:
  // The integer must satisfy `x % mod == remainder`. E.g., Mod(2, 4)
  explicit Mod(int64_t remainder, int64_t mod);

  // The integer must satisfy `x % mod == remainder`. E.g., Mod(2, "3 * N + 1")
  explicit Mod(int64_t remainder, std::string_view mod_expression);

  // The integer must satisfy `x % mod == remainder`. E.g., Mod("2 * N", 4)
  explicit Mod(std::string_view remainder_expression, int64_t mod);

  // The integer must satisfy `x % mod == remainder`. E.g., Mod("N + 1", "10^9")
  explicit Mod(std::string_view remainder_expression,
               std::string_view mod_expression);

  struct Equation {
    Expression remainder;
    Expression modulus;
  };

  // Returns the constraints.
  [[nodiscard]] Equation GetConstraints() const;

  // Determines if the value has the appropriate remainder.
  ConstraintViolation CheckValue(librarian::AnalysisContext ctx,
                                 int64_t value) const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const;

 private:
  Expression remainder_;
  Expression modulus_;
};

}  // namespace moriarty

#endif  // MORIARTY_CONSTRAINTS_INTEGER_CONSTRAINTS_H_
