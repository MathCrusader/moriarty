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

#include "src/constraints/integer_constraints.h"

#include <cstdint>
#include <string>
#include <string_view>

#include "src/constraints/constraint_violation.h"
#include "src/librarian/errors.h"

namespace moriarty {

Mod::Mod(int64_t remainder, int64_t mod)
    : remainder_(std::to_string(remainder)), modulus_(std::to_string(mod)) {
  if (mod <= 0) {
    throw InvalidConstraint("Mod", "Modulus must be positive");
  }
}

Mod::Mod(int64_t remainder, std::string_view mod)
    : remainder_(std::to_string(remainder)), modulus_(mod) {}

Mod::Mod(std::string_view remainder, int64_t mod)
    : remainder_(remainder), modulus_(std::to_string(mod)) {
  if (mod <= 0) {
    throw InvalidConstraint("Mod", "Modulus must be positive");
  }
}

Mod::Mod(std::string_view remainder, std::string_view mod)
    : remainder_(remainder), modulus_(mod) {}

Mod::Equation Mod::GetConstraints() const {
  return Equation{Expression(remainder_), Expression(modulus_)};
}

std::string Mod::ToString() const {
  return std::format("x % ({}) == {}", modulus_.ToString(),
                     remainder_.ToString());
}

ConstraintViolation Mod::CheckValue(ConstraintContext ctx,
                                    int64_t value) const {
  int64_t M = ctx.EvaluateExpression(modulus_);
  if (M <= 0) {
    return ConstraintViolation("Modulus must be positive, but evaluated to " +
                               std::to_string(M));
  }
  int64_t R = ctx.EvaluateExpression(remainder_);
  R %= M;
  if (R < 0) R += M;
  int64_t actual = value % M;
  if (actual < 0) actual += M;
  if (actual != R) {
    return ConstraintViolation(
        std::format("{} is not {} mod {}.", value, R, M));
  }
  return ConstraintViolation::None();
}

std::vector<std::string> Mod::GetDependencies() const {
  std::vector<std::string> deps;
  auto rem_deps = remainder_.GetDependencies();
  deps.insert(deps.end(), rem_deps.begin(), rem_deps.end());
  auto mod_deps = modulus_.GetDependencies();
  deps.insert(deps.end(), mod_deps.begin(), mod_deps.end());
  return deps;
}

}  // namespace moriarty
