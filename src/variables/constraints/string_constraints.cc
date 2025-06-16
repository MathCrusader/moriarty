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

#include "src/variables/constraints/string_constraints.h"

#include <bitset>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>

#include "src/contexts/librarian_context.h"
#include "src/internal/simple_pattern.h"
#include "src/util/debug_string.h"
#include "src/variables/constraints/constraint_violation.h"
#include "src/variables/minteger.h"

namespace moriarty {

namespace {

constexpr static std::string_view kAlphabet =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
constexpr static std::string_view kUpperCase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
constexpr static std::string_view kLowerCase = "abcdefghijklmnopqrstuvwxyz";
constexpr static std::string_view kNumbers = "0123456789";
constexpr static std::string_view kAlphaNumeric =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
constexpr static std::string_view kUpperAlphaNumeric =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
constexpr static std::string_view kLowerAlphaNumeric =
    "abcdefghijklmnopqrstuvwxyz0123456789";

}  // namespace

// ====== Alphabet ======

Alphabet::Alphabet(std::string_view alphabet) : alphabet_(alphabet) {}

std::string Alphabet::GetAlphabet() const { return alphabet_; }

Alphabet Alphabet::Letters() { return Alphabet(kAlphabet); }
Alphabet Alphabet::UpperCase() { return Alphabet(kUpperCase); }
Alphabet Alphabet::LowerCase() { return Alphabet(kLowerCase); }
Alphabet Alphabet::Numbers() { return Alphabet(kNumbers); }
Alphabet Alphabet::AlphaNumeric() { return Alphabet(kAlphaNumeric); }
Alphabet Alphabet::UpperAlphaNumeric() { return Alphabet(kUpperAlphaNumeric); }
Alphabet Alphabet::LowerAlphaNumeric() { return Alphabet(kLowerAlphaNumeric); }

ConstraintViolation Alphabet::CheckValue(std::string_view value) const {
  for (int idx = -1; char c : value) {
    idx++;
    if (alphabet_.find(c) == std::string::npos) {
      return ConstraintViolation(std::format(
          "character at index {} (which is {}) is not a valid character (valid "
          "characters are {})",
          idx, librarian::DebugString(c), librarian::DebugString(alphabet_)));
    }
  }
  return ConstraintViolation::None();
}

std::string Alphabet::ToString() const {
  return std::format("contains only the characters {}",
                     librarian::DebugString(alphabet_));
}

std::vector<std::string> Alphabet::GetDependencies() const { return {}; }

// ====== DistinctCharacters ======

ConstraintViolation DistinctCharacters::CheckValue(
    std::string_view value) const {
  std::bitset<sizeof(unsigned char) * 256> seen;
  for (int idx = -1; unsigned char c : value) {
    idx++;
    if (seen.test(c)) {
      return ConstraintViolation(std::format(
          "character at index {} (which is {}) appears multiple times", idx,
          librarian::DebugString(c)));
    }
    seen.set(c);
  }
  return ConstraintViolation::None();
}

std::string DistinctCharacters::ToString() const {
  return "has distinct characters";
}

std::vector<std::string> DistinctCharacters::GetDependencies() const {
  return {};
}

// ====== SimplePattern ======
SimplePattern::SimplePattern(std::string_view pattern)
    : pattern_(moriarty_internal::SimplePattern(std::string(pattern))) {}

std::string SimplePattern::GetPattern() const { return pattern_.Pattern(); }

moriarty_internal::SimplePattern SimplePattern::GetCompiledPattern() const {
  return pattern_;
}

ConstraintViolation SimplePattern::CheckValue(librarian::AnalysisContext ctx,
                                              std::string_view value) const {
  auto lookup = [&](std::string_view var) -> int64_t {
    return ctx.GetValue<MInteger>(var);
  };
  if (pattern_.Matches(value, lookup)) return ConstraintViolation::None();
  return ConstraintViolation(
      std::format("does not match the simple pattern of {}",
                  librarian::DebugString(pattern_.Pattern())));
}

std::string SimplePattern::ToString() const {
  return std::format("has a simple pattern of {}",
                     librarian::DebugString(pattern_.Pattern()));
}

std::vector<std::string> SimplePattern::GetDependencies() const {
  return pattern_.GetDependencies();
}

}  // namespace moriarty
