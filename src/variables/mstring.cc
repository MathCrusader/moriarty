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
#include <stdexcept>
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
  if (!core_constraints_.data_->length)
    core_constraints_.data_.Mutable().length = MInteger();
  core_constraints_.data_.Mutable().length->MergeFrom(
      constraint.GetConstraints());
  return InternalAddConstraint(std::move(constraint));
}

MString& MString::AddConstraint(Alphabet constraint) {
  // We later assume that there are no duplicates in the alphabet.
  std::string options = constraint.GetAlphabet();
  std::sort(options.begin(), options.end());
  auto last = std::unique(options.begin(), options.end());
  options.erase(last, options.end());

  if (!core_constraints_.data_.Mutable().alphabet.ConstrainOptions(options))
    throw ImpossibleToSatisfy(ToString(), constraint.ToString());
  return InternalAddConstraint(std::move(constraint));
}

MString& MString::AddConstraint(DistinctCharacters constraint) {
  core_constraints_.data_.Mutable().distinct_characters = true;
  return InternalAddConstraint(std::move(constraint));
}

MString& MString::AddConstraint(SimplePattern constraint) {
  core_constraints_.data_.Mutable().simple_patterns.push_back(
      constraint.GetCompiledPattern());
  return InternalAddConstraint(std::move(constraint));
}

MString& MString::AddConstraint(SizeCategory constraint) {
  return AddConstraint(Length(constraint));
}

const std::optional<MInteger>& MString::CoreConstraints::Length() const {
  return data_->length;
}

const librarian::OneOfHandler<char>& MString::CoreConstraints::Alphabet()
    const {
  return data_->alphabet;
}

bool MString::CoreConstraints::DistinctCharacters() const {
  return data_->distinct_characters;
}

std::span<const moriarty_internal::SimplePattern>
MString::CoreConstraints::SimplePatterns() const {
  return data_->simple_patterns;
}

std::vector<MString> MString::ListEdgeCasesImpl(
    librarian::AnalysisContext ctx) const {
  if (!core_constraints_.data_->length) {
    throw std::runtime_error(
        "Attempting to get difficult instances of a string with no "
        "length parameter given.");
  }
  std::vector<MInteger> lengthCases =
      core_constraints_.data_->length->ListEdgeCases(ctx);

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

std::string MString::GenerateImpl(librarian::ResolverContext ctx) const {
  if (GetOneOf().HasBeenConstrained())
    return GetOneOf().SelectOneOf([&](int n) { return ctx.RandomInteger(n); });

  if (core_constraints_.SimplePatterns().empty() &&
      !core_constraints_.Alphabet().HasBeenConstrained()) {
    throw GenerationError(
        ctx.GetVariableName(),
        "Need either Alphabet() or SimplePattern() to generate a string",
        RetryPolicy::kAbort);
  }
  if (core_constraints_.SimplePatterns().empty() &&
      !core_constraints_.Length()) {
    throw GenerationError(
        ctx.GetVariableName(),
        "Need either Length() or SimplePattern() to generate a string",
        RetryPolicy::kAbort);
  }

  if (!core_constraints_.SimplePatterns().empty())
    return GenerateSimplePattern(ctx);

  MInteger length_local = *core_constraints_.Length();

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
    librarian::ResolverContext ctx) const {
  ABSL_CHECK(!core_constraints_.SimplePatterns().empty());

  std::optional<std::string> maybe_alphabet =
      [&]() -> std::optional<std::string> {
    if (!core_constraints_.Alphabet().HasBeenConstrained()) return std::nullopt;
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
  } catch (const std::runtime_error& e) {
    throw GenerationError(
        ctx.GetVariableName(),
        std::format("Failed to generate SimplePattern: ", e.what()),
        RetryPolicy::kAbort);
  }
}

std::string MString::GenerateImplWithDistinctCharacters(
    librarian::ResolverContext ctx) const {
  // Creating a copy in case the alphabet changes in the future, we don't want
  // to limit the length forever.
  MInteger mlength = *core_constraints_.Length();
  // Each char appears at most once.
  mlength.AddConstraint(
      AtMost(core_constraints_.Alphabet().GetOptions().size()));
  int length = mlength.Generate(ctx.ForSubVariable("length"));

  std::vector<char> ret = ctx.RandomElementsWithoutReplacement(
      core_constraints_.Alphabet().GetOptions(), length);

  return std::string(ret.begin(), ret.end());
}

std::string MString::ReadImpl(librarian::ReaderContext ctx) const {
  return ctx.ReadToken();
}

void MString::PrintImpl(librarian::PrinterContext ctx,
                        const std::string& value) const {
  ctx.PrintToken(value);
}

}  // namespace moriarty
