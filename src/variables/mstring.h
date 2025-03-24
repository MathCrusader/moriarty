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

#ifndef MORIARTY_SRC_VARIABLES_MSTRING_H_
#define MORIARTY_SRC_VARIABLES_MSTRING_H_

#include <optional>
#include <string>
#include <vector>

#include "src/contexts/librarian/analysis_context.h"
#include "src/contexts/librarian/printer_context.h"
#include "src/contexts/librarian/reader_context.h"
#include "src/contexts/librarian/resolver_context.h"
#include "src/internal/simple_pattern.h"
#include "src/librarian/mvariable.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/constraints/container_constraints.h"
#include "src/variables/constraints/size_constraints.h"
#include "src/variables/constraints/string_constraints.h"
#include "src/variables/minteger.h"

namespace moriarty {

// MString
//
// Describes constraints placed on an string. The characters in the string must
// be printable ASCII characters. In general (especially in I/O functions), it
// is assumed that strings do not contain whitespace.
//
// In order to generate, the length and the alphabet must be constrained (via
// some combination of the `Length`, `Alphabet`, and `SimplePattern`
// constraints).
class MString : public librarian::MVariable<MString, std::string> {
 public:
  // Create an MString from a set of constraints. Logically equivalent to
  // calling AddConstraint() for each constraint.
  //
  // E.g., MString(Length(10), Alphabet("abc")).
  template <typename... Constraints>
    requires(ConstraintFor<MString, Constraints> && ...)
  explicit MString(Constraints&&... constraints);

  ~MString() override = default;

  // ---------------------------------------------------------------------------
  //  Constrain the value to a specific set of values

  // The string must be exactly this value.
  MString& AddConstraint(Exactly<std::string> constraint);
  // The string must be one of these values.
  MString& AddConstraint(OneOf<std::string> constraint);

  // ---------------------------------------------------------------------------
  //  Constrain the length of the string

  // The string's length must satisfy this constraint.
  MString& AddConstraint(Length constraint);
  // The string should be approximately this size.
  MString& AddConstraint(SizeCategory constraint);

  // ---------------------------------------------------------------------------
  //  Constrain the characters in the string

  // The string must be made of these characters.
  MString& AddConstraint(Alphabet constraint);
  // The string must have distinct characters.
  MString& AddConstraint(DistinctCharacters constraint);

  // ---------------------------------------------------------------------------
  //  Constrain the regex pattern of the string

  // The string must match this simple pattern.
  MString& AddConstraint(SimplePattern constraint);

  [[nodiscard]] std::string Typename() const override { return "MString"; }

 private:
  std::optional<MInteger> length_;
  librarian::OneOfHandler<char> alphabet_;
  bool distinct_characters_ = false;
  std::vector<moriarty_internal::SimplePattern> simple_patterns_;

  std::string GenerateSimplePattern(librarian::ResolverContext ctx) const;
  std::string GenerateImplWithDistinctCharacters(
      librarian::ResolverContext ctx) const;

  // ---------------------------------------------------------------------------
  //  MVariable overrides
  std::string GenerateImpl(librarian::ResolverContext ctx) const override;
  std::string ReadImpl(librarian::ReaderContext ctx) const override;
  void PrintImpl(librarian::PrinterContext ctx,
                 const std::string& value) const override;
  std::vector<MString> ListEdgeCasesImpl(
      librarian::AnalysisContext ctx) const override;
  // ---------------------------------------------------------------------------
};

// -----------------------------------------------------------------------------
//  Implementation details
// -----------------------------------------------------------------------------

template <typename... Constraints>
  requires(ConstraintFor<MString, Constraints> && ...)
MString::MString(Constraints&&... constraints) {
  (AddConstraint(std::forward<Constraints>(constraints)), ...);
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_MSTRING_H_
