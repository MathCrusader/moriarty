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

#ifndef MORIARTY_SRC_VARIABLES_MSTRING_H_
#define MORIARTY_SRC_VARIABLES_MSTRING_H_

#include <string>
#include <type_traits>
#include <vector>

#include "src/constraints/base_constraints.h"
#include "src/constraints/container_constraints.h"
#include "src/constraints/size_constraints.h"
#include "src/constraints/string_constraints.h"
#include "src/contexts/librarian_context.h"
#include "src/internal/simple_pattern.h"
#include "src/librarian/mvariable.h"
#include "src/librarian/util/cow_ptr.h"
#include "src/variables/minteger.h"

namespace moriarty {

class MString;
namespace librarian {
template <>
struct MVariableValueTypeTrait<MString> {
  using type = std::string;
};
}  // namespace librarian

// MString
//
// Describes constraints placed on an string. The characters in the string must
// be printable ASCII characters. In general (especially in I/O functions), it
// is assumed that strings do not contain whitespace.
//
// In order to generate, the length and the alphabet must be constrained (via
// some combination of the `Length`, `Alphabet`, and `SimplePattern`
// constraints).
class MString : public librarian::MVariable<MString> {
 public:
  // Create an MString from a set of constraints. Logically equivalent to
  // calling AddConstraint() for each constraint.
  //
  // E.g., MString(Length(10), Alphabet("abc")).
  template <typename... Constraints>
    requires(ConstraintFor<MString, Constraints> && ...)
  explicit MString(Constraints&&... constraints);

  ~MString() override = default;

  using MVariable<MString>::AddConstraint;  // Custom constraints

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

  // MString::CoreConstraints
  //
  // A base set of constraints for `MString` that are used during generation.
  // Note: Returned references/spans are invalidated after any non-const
  // call to this class or the corresponding `MString`.
  class CoreConstraints {
   public:
    bool LengthConstrained() const;
    const MInteger& Length() const;

    bool AlphabetConstrained() const;
    const librarian::OneOfHandler<char>& Alphabet() const;

    bool SimplePatternsConstrained() const;
    std::span<const moriarty_internal::SimplePattern> SimplePatterns() const;

    bool DistinctCharacters() const;

   private:
    friend class MString;
    enum Flags : uint32_t {
      kLength = 1 << 0,
      kAlphabet = 1 << 1,
      kSimplePattern = 1 << 2,

      kDistinctCharacters = 1 << 3,  // Default: false
    };
    struct Data {
      std::underlying_type_t<Flags> touched = 0;

      MInteger length;
      librarian::OneOfHandler<char> alphabet;
      std::vector<moriarty_internal::SimplePattern> simple_patterns;
    };
    librarian::CowPtr<Data> data_;
    bool IsSet(Flags flag) const;
  };
  [[nodiscard]] CoreConstraints GetCoreConstraints() const {
    return core_constraints_;
  }

 private:
  CoreConstraints core_constraints_;

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
