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

#include "src/variables/constraints/string_constraints.h"

#include <bitset>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>

#include "src/internal/simple_pattern.h"
#include "src/librarian/debug_print.h"

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

bool Alphabet::IsSatisfiedWith(std::string_view value) const {
  for (char c : value) {
    if (alphabet_.find(c) == std::string::npos) return false;
  }
  return true;
}

std::string Alphabet::ToString() const {
  return std::format("contains only {}", librarian::DebugString(alphabet_));
}

std::string Alphabet::Explanation(std::string_view value) const {
  for (int idx = -1; char c : value) {
    idx++;
    if (alphabet_.find(c) == std::string::npos) {
      return std::format(
          "character at index {} (which is {}) is not a valid character (valid "
          "characters are {})",
          idx, librarian::DebugString(c), librarian::DebugString(alphabet_));
    }
  }

  throw std::invalid_argument(
      "Alphabet::Explanation called with all valid characters.");
}

// ====== DistinctCharacters ======

bool DistinctCharacters::IsSatisfiedWith(std::string_view value) const {
  std::bitset<sizeof(unsigned char) * 256> seen;
  for (unsigned char c : value) {
    if (seen.test(c)) return false;
    seen.set(c);
  }
  return true;
}

std::string DistinctCharacters::ToString() const {
  return "has distinct characters";
}

std::string DistinctCharacters::Explanation(std::string_view value) const {
  std::bitset<sizeof(unsigned char) * 256> seen;
  for (int idx = -1; unsigned char c : value) {
    idx++;
    if (seen.test(c)) {
      return std::format(
          "character at index {} (which is {}) appears multiple times", idx,
          librarian::DebugString(c));
    }
    seen.set(c);
  }

  throw std::invalid_argument(
      "DistinctCharacters::Explanation called with all distinct characters.");
}

// ====== SimplePattern ======
// TODO: This hides a StatusOr<>. Should throw instead.
SimplePattern::SimplePattern(std::string_view pattern)
    : pattern_(*moriarty_internal::SimplePattern::Create(pattern)) {}

std::string SimplePattern::GetPattern() const { return pattern_.Pattern(); }

moriarty_internal::SimplePattern SimplePattern::GetCompiledPattern() const {
  return pattern_;
}

bool SimplePattern::IsSatisfiedWith(std::string_view value) const {
  return pattern_.Matches(value);
}

std::string SimplePattern::ToString() const {
  return std::format("has a simple pattern of {}",
                     librarian::DebugString(pattern_.Pattern()));
}

std::string SimplePattern::Explanation(std::string_view value) const {
  return std::format("does not follow the simple pattern of {}",
                     librarian::DebugString(pattern_.Pattern()));
}

}  // namespace moriarty
