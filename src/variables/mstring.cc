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
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "src/constraints/base_constraints.h"
#include "src/constraints/container_constraints.h"
#include "src/constraints/numeric_constraints.h"
#include "src/constraints/size_constraints.h"
#include "src/constraints/string_constraints.h"
#include "src/contexts/librarian_context.h"
#include "src/librarian/errors.h"
#include "src/librarian/policies.h"
#include "src/variables/minteger.h"

namespace moriarty {

MString& MString::AddConstraint(Exactly<std::string> constraint) {
  return InternalAddExactlyConstraint(std::move(constraint));
}

MString& MString::AddConstraint(OneOf<std::string> constraint) {
  return InternalAddOneOfConstraint(std::move(constraint));
}

MString& MString::AddConstraint(Length constraint) {
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kLength;
  constraints.length.MergeFrom(constraint.GetConstraints());
  return InternalAddConstraint(std::move(constraint));
}

MString& MString::AddConstraint(Alphabet constraint) {
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kAlphabet;
  // We later assume that there are no duplicates in the alphabet.
  std::string options = constraint.GetAlphabet();
  std::ranges::sort(options);
  options.erase(std::unique(options.begin(), options.end()), options.end());

  if (!constraints.alphabet.ConstrainOptions(options))
    throw ImpossibleToSatisfy(ToString(), constraint.ToString());
  return InternalAddConstraint(std::move(constraint));
}

MString& MString::AddConstraint(DistinctCharacters constraint) {
  core_constraints_.data_.Mutable().touched |=
      CoreConstraints::Flags::kDistinctCharacters;
  return InternalAddConstraint(std::move(constraint));
}

MString& MString::AddConstraint(SimplePattern constraint) {
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kSimplePattern;
  constraints.simple_patterns.push_back(constraint.GetCompiledPattern());
  return InternalAddConstraint(std::move(constraint));
}

MString& MString::AddConstraint(SizeCategory constraint) {
  return AddConstraint(Length(constraint));
}

bool MString::CoreConstraints::LengthConstrained() const {
  return IsSet(Flags::kLength);
}

const MInteger& MString::CoreConstraints::Length() const {
  return data_->length;
}

bool MString::CoreConstraints::AlphabetConstrained() const {
  return IsSet(Flags::kAlphabet);
}

const librarian::OneOfHandler<char>& MString::CoreConstraints::Alphabet()
    const {
  return data_->alphabet;
}

bool MString::CoreConstraints::DistinctCharacters() const {
  return IsSet(Flags::kDistinctCharacters);
}

bool MString::CoreConstraints::SimplePatternsConstrained() const {
  return IsSet(Flags::kSimplePattern);
}

bool MString::CoreConstraints::IsSet(Flags flag) const {
  return (data_->touched & flag) != 0;
}

std::span<const moriarty_internal::SimplePattern>
MString::CoreConstraints::SimplePatterns() const {
  return data_->simple_patterns;
}

std::vector<MString> MString::ListEdgeCasesImpl(
    librarian::AnalyzeVariableContext ctx) const {
  if (!core_constraints_.LengthConstrained()) {
    throw ConfigurationError(
        "MString::ListEdgeCases",
        "Attempting to get difficult instances of a string with no "
        "length parameter given.");
  }
  std::vector<MInteger> lengthCases =
      core_constraints_.Length().ListEdgeCases(ctx);

  std::vector<MString> values;
  values.reserve(lengthCases.size());
  for (const auto& c : lengthCases) {
    values.push_back(MString(Length(c)));
  }
  // TODO(hivini): Add alphabet cases, pattern cases and integrate
  // SimplePattern.
  // TODO(darcybest): Add Thue-Morse sequence.
  return values;
}

std::string MString::GenerateImpl(
    librarian::GenerateVariableContext ctx) const {
  if (GetOneOf().HasBeenConstrained())
    return GetOneOf().SelectOneOf([&](int n) { return ctx.RandomInteger(n); });

  if (!core_constraints_.SimplePatternsConstrained() &&
      !core_constraints_.AlphabetConstrained()) {
    throw GenerationError(
        ctx.GetVariableName(),
        "Need either Alphabet() or SimplePattern() to generate a string",
        RetryPolicy::kAbort);
  }
  if (!core_constraints_.SimplePatternsConstrained() &&
      !core_constraints_.LengthConstrained()) {
    throw GenerationError(
        ctx.GetVariableName(),
        "Need either Length() or SimplePattern() to generate a string",
        RetryPolicy::kAbort);
  }

  if (core_constraints_.SimplePatternsConstrained())
    return GenerateSimplePattern(ctx);

  MInteger length_local = core_constraints_.Length();

  // Negative string length is impossible.
  length_local.AddConstraint(AtLeast(0));

  if (core_constraints_.DistinctCharacters())
    return GenerateImplWithDistinctCharacters(ctx);

  int length = length_local.Generate(ctx.ForSubVariable("length"));

  std::vector<char> alphabet(core_constraints_.Alphabet().GetOptions());
  std::vector<char> ret = ctx.RandomElementsWithReplacement(alphabet, length);

  return std::string(ret.begin(), ret.end());
}

std::string MString::GenerateSimplePattern(
    librarian::GenerateVariableContext ctx) const {
  ABSL_CHECK(core_constraints_.SimplePatternsConstrained());

  std::optional<std::string> maybe_alphabet =
      [&]() -> std::optional<std::string> {
    if (!core_constraints_.AlphabetConstrained()) return std::nullopt;
    std::vector<char> alphabet(core_constraints_.Alphabet().GetOptions());
    return std::string(alphabet.begin(), alphabet.end());
  }();

  auto lookup = [&](std::string_view name) -> int64_t {
    return ctx.GenerateVariable<MInteger>(name);
  };
  auto rand = [&](int min, int max) { return ctx.RandomInteger(min, max); };
  try {
    // Use the last pattern, since it's probably the most specific. This choice
    // is arbitrary since all patterns must be satisfied.
    return core_constraints_.SimplePatterns().back().GenerateWithRestrictions(
        maybe_alphabet, lookup, rand);
  } catch (const SimplePatternEvaluationError& e) {
    throw GenerationError(
        ctx.GetVariableName(),
        std::format("Failed to generate SimplePattern: ", e.what()),
        RetryPolicy::kAbort);
  }
}

std::string MString::GenerateImplWithDistinctCharacters(
    librarian::GenerateVariableContext ctx) const {
  // Creating a copy in case the alphabet changes in the future, we don't want
  // to limit the length forever.
  MInteger mlength = core_constraints_.Length();
  // Each char appears at most once.
  mlength.AddConstraint(
      AtMost(core_constraints_.Alphabet().GetOptions().size()));
  int length = mlength.Generate(ctx.ForSubVariable("length"));

  std::vector<char> ret = ctx.RandomElementsWithoutReplacement(
      core_constraints_.Alphabet().GetOptions(), length);

  return std::string(ret.begin(), ret.end());
}

std::string MString::ReadImpl(librarian::ReadVariableContext ctx) const {
  return ctx.ReadToken();
}

void MString::WriteImpl(librarian::WriteVariableContext ctx,
                        const std::string& value) const {
  ctx.WriteToken(value);
}

}  // namespace moriarty
