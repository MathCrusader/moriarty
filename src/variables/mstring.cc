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
#include "src/contexts/librarian/analysis_context.h"
#include "src/contexts/librarian/printer_context.h"
#include "src/contexts/librarian/reader_context.h"
#include "src/contexts/librarian/resolver_context.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/constraints/container_constraints.h"
#include "src/variables/constraints/numeric_constraints.h"
#include "src/variables/constraints/size_constraints.h"
#include "src/variables/constraints/string_constraints.h"
#include "src/variables/minteger.h"

namespace moriarty {

MString& MString::AddConstraint(Exactly<std::string> constraint) {
  return InternalAddExactlyConstraint(std::move(constraint));
}

MString& MString::AddConstraint(OneOf<std::string> constraint) {
  return InternalAddOneOfConstraint(std::move(constraint));
}

MString& MString::AddConstraint(Length constraint) {
  if (!length_) length_ = MInteger();
  length_->MergeFrom(constraint.GetConstraints());
  return InternalAddConstraint(std::move(constraint));
}

MString& MString::AddConstraint(Alphabet constraint) {
  // We later assume that there are no duplicates in the alphabet.
  std::string options = constraint.GetAlphabet();
  std::sort(options.begin(), options.end());
  auto last = std::unique(options.begin(), options.end());
  options.erase(last, options.end());

  if (!alphabet_.ConstrainOptions(options)) {
    throw std::runtime_error(
        std::format("No valid character left after constraining alphabet to {}",
                    constraint.GetAlphabet()));
  }

  return InternalAddConstraint(std::move(constraint));
}

MString& MString::AddConstraint(DistinctCharacters constraint) {
  distinct_characters_ = true;
  return InternalAddConstraint(std::move(constraint));
}

MString& MString::AddConstraint(SimplePattern constraint) {
  simple_patterns_.push_back(constraint.GetCompiledPattern());
  return InternalAddConstraint(std::move(constraint));
}

MString& MString::AddConstraint(SizeCategory constraint) {
  return AddConstraint(Length(constraint));
}

std::vector<MString> MString::ListEdgeCasesImpl(
    librarian::AnalysisContext ctx) const {
  if (!length_) {
    throw std::runtime_error(
        "Attempting to get difficult instances of a string with no "
        "length parameter given.");
  }
  std::vector<MInteger> lengthCases = length_->ListEdgeCases(ctx);

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
  return simple_patterns_.back().GenerateWithRestrictions(
      maybe_alphabet,
      [&](int min, int max) { return ctx.RandomInteger(min, max); });
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

std::string MString::ReadImpl(librarian::ReaderContext ctx) const {
  return ctx.ReadToken();
}

void MString::PrintImpl(librarian::PrinterContext ctx,
                        const std::string& value) const {
  ctx.PrintToken(value);
}

}  // namespace moriarty
