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
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
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

MString& MString::AddConstraint(Exactly<std::string> constraint) {
  one_of_.ConstrainOptions(std::vector{constraint.GetValue()});
  NewAddConstraint(std::move(constraint));
  return *this;
}

MString& MString::AddConstraint(Length constraint) {
  if (length_)
    length_->MergeFrom(constraint.GetConstraints());
  else
    length_ = constraint.GetConstraints();
  NewAddConstraint(std::move(constraint));
  return *this;
}

MString& MString::AddConstraint(Alphabet constraint) {
  // We later assume that there are no duplicates in the alphabet.
  std::string options = constraint.GetAlphabet();
  std::sort(options.begin(), options.end());
  auto last = std::unique(options.begin(), options.end());
  options.erase(last, options.end());

  alphabet_.ConstrainOptions(options);
  return *this;
}

MString& MString::AddConstraint(DistinctCharacters constraint) {
  distinct_characters_ = true;
  return *this;
}

MString& MString::AddConstraint(SimplePattern constraint) {
  absl::StatusOr<moriarty_internal::SimplePattern> pattern =
      moriarty_internal::SimplePattern::Create(constraint.GetPattern());
  if (!pattern.ok()) {
    throw std::invalid_argument(
        std::format("Invalid simple pattern; {}", pattern.status().ToString()));
  }

  simple_patterns_.push_back(*std::move(pattern));
  return *this;
}

MString& MString::AddConstraint(SizeCategory constraint) {
  return AddConstraint(Length(std::move(constraint)));
}

absl::Status MString::MergeFromImpl(const MString& other) {
  if (other.length_) AddConstraint(Length(*other.length_));
  if (other.alphabet_.HasBeenConstrained())
    alphabet_.ConstrainOptions(other.alphabet_.GetOptions());
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

  if (alphabet_.HasBeenConstrained()) {
    for (char c : value) {
      if (!alphabet_.IsSatisfiedWith(c)) {
        return UnsatisfiedConstraintError(
            absl::Substitute("character '$0' not in alphabet", c));
      }
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
    values.push_back(MString(Length(c)));
  }
  // TODO(hivini): Add alphabet cases, pattern cases and integrate
  // SimplePattern.
  // TODO(darcybest): Add Thue-Morse sequence.
  return values;
}

std::string MString::GenerateImpl(librarian::ResolverContext ctx) const {
  if (simple_patterns_.empty() && !alphabet_.HasBeenConstrained()) {
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
  length_local.AddConstraint(AtLeast(0));

  if (length_size_property_) {
    auto status = length_local.OfSizeProperty(*length_size_property_);
    if (!status.ok()) {
      throw std::runtime_error(std::string(status.message()));
    }
  }

  std::optional<int64_t> generation_limit = ctx.GetSoftGenerationLimit();
  if (generation_limit) length_local.AddConstraint(AtMost(*generation_limit));

  if (distinct_characters_) return GenerateImplWithDistinctCharacters(ctx);

  int length = length_local.Generate(ctx.ForSubVariable("length"));

  std::vector<char> alphabet(alphabet_.GetOptions());
  std::vector<char> ret = ctx.RandomElementsWithReplacement(alphabet, length);

  return std::string(ret.begin(), ret.end());
}

std::string MString::GenerateSimplePattern(
    librarian::ResolverContext ctx) const {
  ABSL_CHECK(!simple_patterns_.empty());

  std::optional<std::string> maybe_alphabet =
      [&]() -> std::optional<std::string> {
    if (!alphabet_.HasBeenConstrained()) return std::nullopt;
    std::vector<char> alphabet(alphabet_.GetOptions());
    return std::string(alphabet.begin(), alphabet.end());
  }();

  // Use the last pattern, since it's probably the most specific. This choice is
  // arbitrary since all patterns must be satisfied.
  absl::StatusOr<std::string> result =
      simple_patterns_.back().GenerateWithRestrictions(
          maybe_alphabet, ctx.UnsafeGetRandomEngine());
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
  // Each char appears at most once.
  mlength.AddConstraint(AtMost(alphabet_.GetOptions().size()));
  int length = mlength.Generate(ctx.ForSubVariable("length"));

  std::vector<char> ret =
      ctx.RandomElementsWithoutReplacement(alphabet_.GetOptions(), length);

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

}  // namespace moriarty
