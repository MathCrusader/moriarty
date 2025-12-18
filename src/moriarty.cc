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
#include <format>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "src/context.h"
#include "src/internal/analysis_bootstrap.h"
#include "src/internal/generation_bootstrap.h"
#include "src/internal/random_engine.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_name_utils.h"
#include "src/internal/variable_set.h"
#include "src/librarian/errors.h"
#include "src/librarian/io_config.h"
#include "src/test_case.h"

namespace moriarty {

Moriarty& Moriarty::SetName(std::string_view name) {
  name_ = name;
  return *this;
}

Moriarty& Moriarty::SetNumCases(int num_cases) {
  if (num_cases < 0)
    throw ConfigurationError("Moriarty::SetNumCases",
                             "num_cases must be non-negative");

  num_cases_ = num_cases;
  return *this;
}

Moriarty& Moriarty::SetSeed(std::string_view seed) {
  if (seed.size() < kMinimumSeedLength) {
    throw ConfigurationError(
        "Moriarty::SetSeed",
        std::format("The seed's length must be at least {}",
                    kMinimumSeedLength));
  }

  // Each generator receives a random engine whose seed only differs in the
  // final index. This copies in the first seed.size() here, then
  // GetSeedForGenerator() deals with the final value that changes for each
  // generator.
  seed_.resize(seed.size() + 1);
  for (int i = 0; i < seed.size(); i++) seed_[i] = seed[i];
  return *this;
}

std::span<const int64_t> Moriarty::GetSeedForGenerator(int index) {
  if (seed_.empty())
    throw ConfigurationError("Moriarty::GetSeed",
                             "Seed not set before generation started.");

  seed_.back() = index;
  return seed_;
}

namespace {

std::string FailureToString(
    const std::vector<DetailedConstraintViolation>& failures) {
  std::string result;

  for (const auto& [var_name, reason] : failures) {
    if (!result.empty()) result += "\n";
    result += std::format(" - Variable `{}` failed constraint: {}\n", var_name,
                          reason.Reason());
  }
  return result;
}

}  // namespace

ValidationResults Moriarty::ValidateTestCases(ValidateOptions options) const {
  if (options.validation == ValidationStyle::kNone) return ValidationResults();

  ValidationResults res;
  if (test_cases_.empty()) {
    res.AddFailure(0, "No Test Cases.");
    return res;
  }

  for (int case_num = 1; const auto& test_case : test_cases_) {
    std::vector<DetailedConstraintViolation> failures =
        moriarty_internal::CheckValues(variables_, test_case.UnsafeGetValues(),
                                       options.variables_to_validate,
                                       options.validation);
    if (!failures.empty()) {
      res.AddFailure(test_cases_.size() == 1 ? 0 : case_num,
                     FailureToString(failures));
    }
    case_num++;
  }
  return res;
}

void Moriarty::ReadTestCases(ReaderFn fn, ReadOptions options) {
  InputCursor cursor(options.is, options.whitespace_strictness);
  ReadContext ctx(variables_, cursor);

  std::vector<TestCase> test_cases = fn(ctx);
  test_cases_.insert(test_cases_.end(), test_cases.begin(), test_cases.end());

  ValidationResults results =
      ValidateTestCases(ValidateOptions{.validation = options.validation});
  if (!results.IsValid()) {
    throw ValidationError("Moriarty::ReadTestCases",
                          results.DescribeFailures());
  }
}

void Moriarty::WriteTestCases(WriterFn fn, WriteOptions options) const {
  moriarty_internal::ValueSet values;
  WriteContext ctx(options.os, variables_, values);
  fn(ctx, test_cases_);
}

void Moriarty::GenerateTestCases(GenerateFn fn, GenerateOptions options) {
  // FIXME: Seed is wrong. (add test)
  // FIXME: name isn't used. (add test)
  moriarty_internal::ValueSet values;
  moriarty_internal::RandomEngine rng(seed_, "v0.1");
  for (int call = 1; call <= options.num_calls; call++) {
    GenerateContext ctx(variables_, values, rng);
    std::vector<MTestCase> test_cases = fn(ctx);

    for (const MTestCase& test_case : test_cases) {
      test_cases_.emplace_back(moriarty_internal::GenerateAllValues(
          variables_, test_case.UnsafeGetVariables(),
          test_case.UnsafeGetValues(), {rng, options.variables_to_generate}));
    }
  }

  ValidationResults results =
      ValidateTestCases(ValidateOptions{.validation = options.validation});
  if (!results.IsValid()) {
    throw ValidationError("Moriarty::GenerateTestCases",
                          results.DescribeFailures());
  }
}

Moriarty& Moriarty::AddAnonymousVariable(
    std::string_view name,
    const moriarty_internal::AbstractVariable& variable) {
  moriarty_internal::ValidateVariableName(name);
  if (variables_.Contains(name)) {
    throw std::invalid_argument(
        std::format("Adding the variable `{}` multiple times", name));
  }
  variables_.SetVariable(name, variable);
  return *this;
}

}  // namespace moriarty
