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
#include <optional>
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
#include "src/test_case.h"

namespace moriarty {

Moriarty& Moriarty::SetName(std::string_view name) {
  name_ = name;
  return *this;
}

Moriarty& Moriarty::SetNumCases(int num_cases) {
  if (num_cases < 0)
    throw std::invalid_argument("num_cases must be non-negative");

  num_cases_ = num_cases;
  return *this;
}

Moriarty& Moriarty::SetSeed(std::string_view seed) {
  if (seed.size() < kMinimumSeedLength) {
    throw std::invalid_argument(std::format(
        "The seed's length must be at least {}", kMinimumSeedLength));
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
  if (seed_.empty()) throw std::runtime_error("GetSeed called before seed set");

  seed_.back() = index;
  return seed_;
}

std::optional<std::string> Moriarty::ValidateTestCases() {
  if (assigned_test_cases_.empty()) return "No TestCases.";

  int case_num = 1;
  for (const moriarty_internal::ValueSet& test_case : assigned_test_cases_) {
    std::optional<std::string> failure =
        moriarty_internal::AllVariablesSatisfyConstraints(variables_,
                                                          test_case);
    if (failure.has_value())
      return std::format("Case {} invalid: {}", case_num, *failure);
    case_num++;
  }
  return std::nullopt;
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
  moriarty_internal::ValueSet values;
  ExportContext ctx(options.os, variables_, values);
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
  moriarty_internal::ValueSet values;
  moriarty_internal::RandomEngine rng(seed_, "v0.1");
  for (int call = 1; call <= options.num_calls; call++) {
    GenerateContext ctx(variables_, values, rng);
    std::vector<TestCase> test_cases = fn(ctx);

    for (const TestCase& test_case : test_cases) {
      auto [extra_constraints, values] =
          UnsafeExtractTestCaseInternals(test_case);

      assigned_test_cases_.push_back(moriarty_internal::GenerateAllValues(
          variables_, extra_constraints, values, {rng}));
    }
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
