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

#include "src/moriarty.h"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/types/span.h"
#include "src/context.h"
#include "src/internal/analysis_bootstrap.h"
#include "src/internal/generation_bootstrap.h"
#include "src/internal/random_engine.h"
#include "src/internal/status_utils.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/test_case.h"
#include "src/util/status_macro/status_macros.h"

namespace moriarty {

Moriarty& Moriarty::SetName(absl::string_view name) {
  name_ = name;
  return *this;
}

Moriarty& Moriarty::SetNumCases(int num_cases) {
  moriarty_internal::TryFunctionOrCrash(
      [&]() { return TrySetNumCases(num_cases); }, "SetNumCases");
  return *this;
}

absl::Status Moriarty::TrySetNumCases(int num_cases) {
  if (num_cases < 0)
    return absl::InvalidArgumentError("num_cases must be non-negative");

  num_cases_ = num_cases;
  return absl::OkStatus();
}

Moriarty& Moriarty::SetSeed(absl::string_view seed) {
  moriarty_internal::TryFunctionOrCrash([&]() { return TrySetSeed(seed); },
                                        "SetSeed");
  return *this;
}

absl::Status Moriarty::TrySetSeed(absl::string_view seed) {
  if (seed.size() < kMinimumSeedLength) {
    return absl::InvalidArgumentError(absl::Substitute(
        "The seed's length must be at least $0", kMinimumSeedLength));
  }

  // Each generator receives a random engine whose seed only differs in the
  // final index. This copies in the first seed.size() here, then
  // GetSeedForGenerator() deals with the final value that changes for each
  // generator.
  seed_.resize(seed.size() + 1);
  for (int i = 0; i < seed.size(); i++) seed_[i] = seed[i];
  return absl::OkStatus();
}

absl::StatusOr<absl::Span<const int64_t>> Moriarty::GetSeedForGenerator(
    int index) {
  if (seed_.empty())
    return absl::FailedPreconditionError(
        "GetSeedForGenerator should only be called after a valid seed has been "
        "set.");
  seed_.back() = index;
  return seed_;
}

absl::Status Moriarty::TryValidateTestCases() {
  if (assigned_test_cases_.empty()) {
    return absl::FailedPreconditionError("No TestCases to validate.");
  }

  int case_num = 1;
  for (const moriarty_internal::ValueSet& test_case : assigned_test_cases_) {
    MORIARTY_RETURN_IF_ERROR(moriarty_internal::AllVariablesSatisfyConstraints(
        variables_, test_case))
        << "Case " << case_num << " invalid";
    case_num++;
  }
  return absl::OkStatus();
}

absl::Status Moriarty::ValidateVariableName(absl::string_view name) {
  if (name.empty())
    return absl::InvalidArgumentError("Variable name cannot be empty");
  for (char c : name)
    if (!absl::ascii_isalnum(c) && c != '_')
      return absl::InvalidArgumentError(
          "Variable name can only contain 'A-Za-z0-9_'.");
  // TODO(darcybest): Re-enable this test once our users update their workflow.
  // if (!absl::ascii_isalpha(name[0]))
  //   return absl::InvalidArgumentError(
  //       "Variable name must start with an alphabetic character");
  return absl::OkStatus();
}

void Moriarty::ImportTestCases(ImportFn fn, ImportOptions options) {
  ImportContext ctx(variables_, options.is, options.whitespace_strictness);

  std::vector<ConcreteTestCase> test_cases = fn(ctx);
  for (const ConcreteTestCase& test_case : test_cases) {
    assigned_test_cases_.push_back(
        UnsafeExtractConcreteTestCaseInternals(test_case));
  }
}

void Moriarty::ExportTestCases(ExportFn fn, ExportOptions options) const {
  ExportContext ctx(options.os, variables_, {});
  std::vector<ConcreteTestCase> test_cases;
  for (const moriarty_internal::ValueSet& values : assigned_test_cases_) {
    test_cases.push_back(ConcreteTestCase());
    UnsafeSetConcreteTestCaseInternals(test_cases.back(), values);
  }

  fn(ctx, test_cases);
}

void Moriarty::GenerateTestCases(GenerateFn fn, GenerateOptions options) {
  // FIXME: Seed is wrong. (add test)
  // FIXME: name isn't used. (add test)
  moriarty_internal::RandomEngine rng(seed_, "v0.1");
  for (int call = 1; call <= options.num_calls; call++) {
    GenerateContext ctx(variables_, {}, rng);
    std::vector<TestCase> test_cases = fn(ctx);

    for (const TestCase& test_case : test_cases) {
      assigned_test_cases_.push_back(moriarty_internal::GenerateTestCase(
          test_case, variables_, {rng, std::nullopt}));
    }
  }
}

}  // namespace moriarty
