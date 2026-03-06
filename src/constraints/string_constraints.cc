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

#include "src/constraints/string_constraints.h"

#include <algorithm>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>

#include "src/constraints/constraint_violation.h"
#include "src/context.h"
#include "src/internal/simple_pattern.h"
#include "src/internal/value_printer.h"
#include "src/variables/minteger.h"

namespace moriarty {

namespace {

constexpr static std::string_view kAlphabet =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
constexpr static std::string_view kUppercase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
constexpr static std::string_view kLowercase = "abcdefghijklmnopqrstuvwxyz";
constexpr static std::string_view kNumbers = "0123456789";
constexpr static std::string_view kAlphanumeric =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
constexpr static std::string_view kUpperAlphanumeric =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
constexpr static std::string_view kLowerAlphanumeric =
    "abcdefghijklmnopqrstuvwxyz0123456789";

}  // namespace

// ====== Alphabet ======

Alphabet::Alphabet(std::string_view alphabet) : alphabet_(alphabet) {}

std::string Alphabet::GetAlphabet() const { return alphabet_; }

Alphabet Alphabet::Letters() { return Alphabet(kAlphabet); }
Alphabet Alphabet::Uppercase() { return Alphabet(kUppercase); }
Alphabet Alphabet::Lowercase() { return Alphabet(kLowercase); }
Alphabet Alphabet::Numbers() { return Alphabet(kNumbers); }
Alphabet Alphabet::Alphanumeric() { return Alphabet(kAlphanumeric); }
Alphabet Alphabet::UpperAlphanumeric() { return Alphabet(kUpperAlphanumeric); }
Alphabet Alphabet::LowerAlphanumeric() { return Alphabet(kLowerAlphanumeric); }

ValidationResult Alphabet::Validate(ConstraintContext ctx,
                                    std::string_view value) const {
  for (int idx = -1; char c : value) {
    idx++;
    if (alphabet_.find(c) == std::string::npos) {
      return ctx.Violation(
          value,
          ValidationResult::Violation(
              std::format("index {}", idx), c,
              librarian::Expected("one of {}",
                                  moriarty_internal::ValuePrinter(alphabet_))));
    }
  }
  return ValidationResult::Ok();
}

std::string Alphabet::ToString() const {
  return std::format("contains only the characters {}",
                     moriarty_internal::ValuePrinter(alphabet_));
}

std::vector<std::string> Alphabet::GetDependencies() const { return {}; }

// ====== DistinctCharacters ======

ValidationResult DistinctCharacters::Validate(ConstraintContext ctx,
                                              std::string_view value) const {
  std::array<int, (1 << 8) * sizeof(unsigned char)> seen;
  std::fill(seen.begin(), seen.end(), -1);
  for (int idx = -1; unsigned char c : value) {
    idx++;
    if (seen[static_cast<int>(c)] != -1) {
      return ctx.Violation(
          value,
          ValidationResult::Violation(
              std::format("indices {} and {}", seen[static_cast<int>(c)], idx),
              c, librarian::Expected("distinct characters")));
    }
    seen[static_cast<int>(c)] = idx;
  }
  return ValidationResult::Ok();
}

// ====== SimplePattern ======
SimplePattern::SimplePattern(std::string_view pattern)
    : pattern_(moriarty_internal::SimplePattern(std::string(pattern))) {}

std::string SimplePattern::GetPattern() const { return pattern_.Pattern(); }

moriarty_internal::SimplePattern SimplePattern::GetCompiledPattern() const {
  return pattern_;
}

ValidationResult SimplePattern::Validate(ConstraintContext ctx,
                                         std::string_view value) const {
  auto lookup = [&](std::string_view var) -> int64_t {
    return ctx.GetValue<MInteger>(var);
  };
  if (pattern_.Matches(value, lookup)) return ValidationResult::Ok();
  return ctx.Violation(
      value, {.expected = std::format(
                  "match the simple pattern {}",
                  moriarty_internal::ValuePrinter(pattern_.Pattern()))});
}

std::string SimplePattern::ToString() const {
  return std::format("has a simple pattern of {}",
                     moriarty_internal::ValuePrinter(pattern_.Pattern()));
}

std::vector<std::string> SimplePattern::GetDependencies() const {
  return pattern_.GetDependencies();
}

}  // namespace moriarty
