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

#include <concepts>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "src/contexts/librarian/analysis_context.h"
#include "src/contexts/librarian/printer_context.h"
#include "src/contexts/librarian/reader_context.h"
#include "src/contexts/librarian/resolver_context.h"
#include "src/internal/simple_pattern.h"
#include "src/librarian/mvariable.h"
#include "src/librarian/one_of_handler.h"
#include "src/property.h"
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
    requires(std::derived_from<std::decay_t<Constraints>, MConstraint> && ...)
  explicit MString(Constraints&&... constraints);

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

  // OfSizeProperty()
  //
  // Tells this string to have a specific size/length. `property.category` must
  // be "size". The exact values here are not guaranteed and may change over
  // time. If exact values are required, specify them manually.
  absl::Status OfSizeProperty(Property property);

 private:
  librarian::OneOfHandler<std::string> one_of_;
  std::optional<MInteger> length_;
  librarian::OneOfHandler<char> alphabet_;
  bool distinct_characters_ = false;
  std::vector<moriarty_internal::SimplePattern> simple_patterns_;
  std::optional<Property> length_size_property_;

  std::string GenerateSimplePattern(librarian::ResolverContext ctx) const;
  std::string GenerateImplWithDistinctCharacters(
      librarian::ResolverContext ctx) const;

  // ---------------------------------------------------------------------------
  //  MVariable overrides
  std::string GenerateImpl(librarian::ResolverContext ctx) const override;
  absl::Status IsSatisfiedWithImpl(librarian::AnalysisContext ctx,
                                   const std::string& value) const override;
  absl::Status MergeFromImpl(const MString& other) override;
  std::string ReadImpl(librarian::ReaderContext ctx) const override;
  void PrintImpl(librarian::PrinterContext ctx,
                 const std::string& value) const override;
  std::vector<std::string> GetDependenciesImpl() const override;
  absl::StatusOr<std::vector<MString>> GetDifficultInstancesImpl(
      librarian::AnalysisContext ctx) const override;
  // ---------------------------------------------------------------------------
};

// -----------------------------------------------------------------------------
//  Implementation details
// -----------------------------------------------------------------------------

template <typename... Constraints>
  requires(std::derived_from<std::decay_t<Constraints>, MConstraint> && ...)
MString::MString(Constraints&&... constraints) {
  RegisterKnownProperty("size", &MString::OfSizeProperty);
  (AddConstraint(std::forward<Constraints>(constraints)), ...);
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_MSTRING_H_
