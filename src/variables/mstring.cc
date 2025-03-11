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

#include "src/variables/mstring.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/substitute.h"
#include "src/contexts/librarian/analysis_context.h"
#include "src/contexts/librarian/printer_context.h"
#include "src/contexts/librarian/reader_context.h"
#include "src/contexts/librarian/resolver_context.h"
#include "src/errors.h"
#include "src/internal/simple_pattern.h"
#include "src/property.h"
#include "src/util/status_macro/status_macros.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/constraints/container_constraints.h"
#include "src/variables/constraints/numeric_constraints.h"
#include "src/variables/constraints/size_constraints.h"
#include "src/variables/constraints/string_constraints.h"
#include "src/variables/minteger.h"

namespace moriarty {

MString& MString::AddConstraint(const Exactly<std::string>& constraint) {
  return Is(constraint.GetValue());
}

MString& MString::AddConstraint(const Length& constraint) {
  if (length_)
    length_->MergeFrom(constraint.GetConstraints());
  else
    length_ = constraint.GetConstraints();
  return *this;
}

MString& MString::AddConstraint(const Alphabet& constraint) {
  return WithAlphabet(constraint.GetAlphabet());
}

MString& MString::AddConstraint(const DistinctCharacters& constraint) {
  distinct_characters_ = true;
  return *this;
}

MString& MString::AddConstraint(const SimplePattern& constraint) {
  return WithSimplePattern(constraint.GetPattern());
}

MString& MString::AddConstraint(const SizeCategory& constraint) {
  return AddConstraint(Length(constraint));
}

MString& MString::OfLength(const MInteger& length) {
  return AddConstraint(Length(length));
}

MString& MString::OfLength(int64_t length) {
  return AddConstraint(Length(length));
}

MString& MString::OfLength(std::string_view length_expression) {
  return AddConstraint(Length(length_expression));
}

MString& MString::OfLength(int64_t min_length, int64_t max_length) {
  return AddConstraint(Length(Between(min_length, max_length)));
}

MString& MString::OfLength(int64_t min_length,
                           std::string_view max_length_expression) {
  return AddConstraint(Length(Between(min_length, max_length_expression)));
}

MString& MString::OfLength(std::string_view min_length_expression,
                           int64_t max_length) {
  return AddConstraint(Length(Between(min_length_expression, max_length)));
}

MString& MString::OfLength(std::string_view min_length_expression,
                           std::string_view max_length_expression) {
  return AddConstraint(
      Length(Between(min_length_expression, max_length_expression)));
}

MString& MString::WithAlphabet(std::string_view valid_characters) {
  // If `valid_characters` isn't sorted or contains multiple occurrences of
  // the same letter, we need to create a new string in order to sort the
  // characters.
  std::string valid_character_str;
  if (!absl::c_is_sorted(valid_characters) ||
      absl::c_adjacent_find(valid_characters) != std::end(valid_characters)) {
    valid_character_str = valid_characters;

    // Sort
    absl::c_sort(valid_character_str);
    // Remove duplicates
    valid_character_str.erase(
        std::unique(valid_character_str.begin(), valid_character_str.end()),
        valid_character_str.end());
    valid_characters = valid_character_str;
  }

  if (!alphabet_) {
    alphabet_ = valid_characters;
  } else {  // Do set intersection of the two alphabets
    alphabet_->erase(absl::c_set_intersection(valid_characters, *alphabet_,
                                              alphabet_->begin()),
                     alphabet_->end());
  }

  return *this;
}

MString& MString::WithDistinctCharacters() {
  return AddConstraint(DistinctCharacters());
}

MString& MString::WithSimplePattern(std::string_view simple_pattern) {
  absl::StatusOr<moriarty_internal::SimplePattern> pattern =
      moriarty_internal::SimplePattern::Create(simple_pattern);
  if (!pattern.ok()) {
    DeclareSelfAsInvalid(
        UnsatisfiedConstraintError(pattern.status().ToString()));
    return *this;
  }

  simple_patterns_.push_back(*std::move(pattern));
  return *this;
}

absl::Status MString::MergeFromImpl(const MString& other) {
  if (other.length_) OfLength(*other.length_);
  if (other.alphabet_) WithAlphabet(*other.alphabet_);
  distinct_characters_ = other.distinct_characters_;
  for (const auto& pattern : other.simple_patterns_)
    simple_patterns_.push_back(pattern);

  return absl::OkStatus();
}

absl::Status MString::IsSatisfiedWithImpl(librarian::AnalysisContext ctx,
                                          const std::string& value) const {
  if (length_) {
    MORIARTY_RETURN_IF_ERROR(
        CheckConstraint(length_->IsSatisfiedWith(ctx, value.length()),
                        "length of string is invalid"));
  }

  if (alphabet_) {
    for (char c : value) {
      MORIARTY_RETURN_IF_ERROR(CheckConstraint(
          absl::c_binary_search(*alphabet_, c),
          absl::Substitute("character '$0' not in alphabet, but in '$1'", c,
                           value)));
    }
  }

  if (distinct_characters_) {
    absl::flat_hash_set<char> seen;
    for (char c : value) {
      auto [it, inserted] = seen.insert(c);
      MORIARTY_RETURN_IF_ERROR(CheckConstraint(
          inserted,
          absl::Substitute(
              "Characters are not distinct. '$0' appears multiple times.", c)));
    }
  }

  for (const moriarty_internal::SimplePattern& pattern : simple_patterns_) {
    MORIARTY_RETURN_IF_ERROR(CheckConstraint(
        pattern.Matches(value),
        absl::Substitute("string '$0' does not match simple pattern '$1'",
                         value, pattern.Pattern())));
  }

  return absl::OkStatus();
}

absl::StatusOr<std::vector<MString>> MString::GetDifficultInstancesImpl(
    librarian::AnalysisContext ctx) const {
  if (!length_) {
    return absl::FailedPreconditionError(
        "Attempting to get difficult instances of a string with no "
        "length parameter given.");
  }
  std::vector<MString> values = {};
  MORIARTY_ASSIGN_OR_RETURN(std::vector<MInteger> lengthCases,
                            length_->GetDifficultInstances(ctx));

  values.reserve(lengthCases.size());
  for (const auto& c : lengthCases) {
    values.push_back(MString().OfLength(c));
  }
  // TODO(hivini): Add alphabet cases, pattern cases and integrate
  // SimplePattern.
  // TODO(darcybest): Add Thue-Morse sequence.
  return values;
}

std::string MString::GenerateImpl(librarian::ResolverContext ctx) const {
  if (simple_patterns_.empty() && (!alphabet_ || alphabet_->empty())) {
    throw std::runtime_error(
        "Cannot generate a string with an empty alphabet and no simple "
        "pattern.");
  }
  if (simple_patterns_.empty() && !length_) {
    throw std::runtime_error(
        "Cannot generate a string with no length parameter or simple "
        "pattern given.");
  }

  if (!simple_patterns_.empty()) return GenerateSimplePattern(ctx);

  MInteger length_local = *length_;

  // Negative string length is impossible.
  length_local.AtLeast(0);

  if (length_size_property_) {
    auto status = length_local.OfSizeProperty(*length_size_property_);
    if (!status.ok()) {
      throw std::runtime_error(std::string(status.message()));
    }
  }

  std::optional<int64_t> generation_limit = ctx.GetSoftGenerationLimit();
  if (generation_limit) length_local.AtMost(*generation_limit);

  if (distinct_characters_) return GenerateImplWithDistinctCharacters(ctx);

  int length = length_local.Generate(ctx.ForSubVariable("length"));

  std::vector<char> alphabet(alphabet_->begin(), alphabet_->end());
  std::vector<char> ret = ctx.RandomElementsWithReplacement(alphabet, length);

  return std::string(ret.begin(), ret.end());
}

std::string MString::GenerateSimplePattern(
    librarian::ResolverContext ctx) const {
  ABSL_CHECK(!simple_patterns_.empty());

  // Use the last pattern, since it's probably the most specific. This choice is
  // arbitrary since all patterns must be satisfied.
  absl::StatusOr<std::string> result =
      simple_patterns_.back().GenerateWithRestrictions(
          alphabet_, ctx.UnsafeGetRandomEngine());
  if (!result.ok()) {
    throw std::runtime_error(result.status().ToString());
  }
  return *result;
}

std::string MString::GenerateImplWithDistinctCharacters(
    librarian::ResolverContext ctx) const {
  // Creating a copy in case the alphabet changes in the future, we don't want
  // to limit the length forever.
  MInteger mlength = *length_;
  mlength.AtMost(alphabet_->size());  // Each char appears at most once.
  int length = mlength.Generate(ctx.ForSubVariable("length"));

  std::vector<char> alphabet(alphabet_->begin(), alphabet_->end());
  std::vector<char> ret =
      ctx.RandomElementsWithoutReplacement(alphabet, length);

  return std::string(ret.begin(), ret.end());
}

absl::Status MString::OfSizeProperty(Property property) {
  length_size_property_ = std::move(property);
  return absl::OkStatus();
}

std::vector<std::string> MString::GetDependenciesImpl() const {
  return length_ ? length_->GetDependencies() : std::vector<std::string>();
}

std::string MString::ReadImpl(librarian::ReaderContext ctx) const {
  return ctx.ReadToken();
}

void MString::PrintImpl(librarian::PrinterContext ctx,
                        const std::string& value) const {
  ctx.PrintToken(value);
}

std::string MString::ToStringImpl() const {
  std::string result;
  if (length_) absl::StrAppend(&result, "length: ", length_->ToString(), "; ");
  if (alphabet_) absl::StrAppend(&result, "alphabet: ", *alphabet_, "; ");
  if (distinct_characters_)
    absl::StrAppend(&result, "Only distinct characters; ");
  for (const moriarty_internal::SimplePattern& pattern : simple_patterns_)
    absl::StrAppend(&result, "simple_pattern: ", pattern.Pattern(), "; ");
  if (length_size_property_.has_value())
    absl::StrAppend(&result, "length: ", length_size_property_->ToString(),
                    "; ");
  return result;
}

absl::StatusOr<std::string> MString::ValueToStringImpl(
    const std::string& value) const {
  return value;
}

}  // namespace moriarty
