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

#ifndef MORIARTY_SRC_VARIABLES_CONSTRAINTS_STRING_CONSTRAINTS_H_
#define MORIARTY_SRC_VARIABLES_CONSTRAINTS_STRING_CONSTRAINTS_H_

#include <string>
#include <string_view>

#include "src/constraints/base_constraints.h"
#include "src/constraints/constraint_violation.h"
#include "src/contexts/librarian_context.h"
#include "src/internal/simple_pattern.h"

namespace moriarty {

// Constraint stating that the string must only contain characters from the
// given alphabet.
class Alphabet : public MConstraint {
 public:
  // The string must only contain characters from the given alphabet.
  explicit Alphabet(std::string_view alphabet);

  // TODO(darcybest): Consider having allowing a container of chars as well.

  // The string must only contain English letters (A-Z, a-z).
  static Alphabet Letters();
  // The string must only contain uppercase English letters (A-Z).
  static Alphabet UpperCase();
  // The string must only contain lowercase English letters (a-z).
  static Alphabet LowerCase();
  // The string must only contain numbers (0-9).
  static Alphabet Numbers();
  // The string must only contain alpha-numeric digits (A-Z, a-z, 0-9).
  static Alphabet AlphaNumeric();
  // The string must only contain uppercase alpha-numeric digits (A-Z, 0-9).
  static Alphabet UpperAlphaNumeric();
  // The string must only contain lowercase alpha-numeric digits (a-z, 0-9).
  static Alphabet LowerAlphaNumeric();

  // Returns the alphabet that the string must only contain characters from.
  [[nodiscard]] std::string GetAlphabet() const;

  // Determines if the string has the correct characters.
  [[nodiscard]] ConstraintViolation CheckValue(std::string_view value) const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const;

 private:
  std::string alphabet_;
};

// Constraint stating that the characters in the string must all be distinct.
class DistinctCharacters : public BasicMConstraint {
 public:
  // The characters in the string must all be distinct.
  explicit DistinctCharacters() : BasicMConstraint("has distinct characters") {}

  // Determines if the string has no duplicate characters.
  [[nodiscard]] ConstraintViolation CheckValue(std::string_view value) const;
};

// Constraint stating that the string must match this simple pattern.
//
// A "simple pattern" is a subset of normal regex, but acts in a greedy way
// and only allows very basic regex strings.
//
// See src/internal/simple_pattern.h for more details.
//
// TODO(darcybest): Add more specific/user-friendly documentation for simple
// pattern.
class SimplePattern : public MConstraint {
 public:
  // The string must match this simple pattern.
  explicit SimplePattern(std::string_view pattern);

  // Returns the pattern that the string must match.
  [[nodiscard]] std::string GetPattern() const;

  // Returns the compiled pattern that the string must match.
  [[nodiscard]] moriarty_internal::SimplePattern GetCompiledPattern() const;

  // Determines if the string has the correct characters.
  [[nodiscard]] ConstraintViolation CheckValue(librarian::AnalysisContext ctx,
                                               std::string_view value) const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns all variables that this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const;

 private:
  moriarty_internal::SimplePattern pattern_;
};

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_CONSTRAINTS_STRING_CONSTRAINTS_H_
