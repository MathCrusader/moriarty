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

#include "src/generators/combinatorial_generator.h"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "src/context.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/combinatorial_coverage.h"
#include "src/test_case.h"

namespace moriarty {

namespace {

using VarPtr = std::unique_ptr<moriarty_internal::AbstractVariable>;

struct InitializeCasesInfo {
  std::vector<std::vector<VarPtr>> cases;
  std::vector<std::string> variable_names;
  std::vector<int> dimension_sizes;
};

InitializeCasesInfo InitializeCases(GenerateContext ctx) {
  InitializeCasesInfo info;
  for (const auto& [name, var_ptr] : ctx.GetAllVariables()) {
    moriarty::librarian::AnalysisContext analysis_ctx(name, ctx);
    std::vector<VarPtr> difficult_vars =
        *var_ptr->GetDifficultAbstractVariables(analysis_ctx);
    info.dimension_sizes.push_back(difficult_vars.size());
    info.cases.push_back(std::move(difficult_vars));
    info.variable_names.push_back(name);
  }
  return info;
}

std::vector<TestCase> CreateTestCases(
    std::span<const CoveringArrayTestCase> covering_array,
    std::span<const std::vector<VarPtr>> cases,
    std::span<const std::string> variable_names) {
  std::vector<TestCase> test_cases;
  for (const CoveringArrayTestCase& x : covering_array) {
    test_cases.push_back(TestCase());
    for (int i = 0; i < x.test_case.size(); i++) {
      test_cases.back().ConstrainAnonymousVariable(
          variable_names[i], *(cases[i][x.test_case[i]].get()));
    }
  }
  return test_cases;
}

}  // namespace

std::vector<TestCase> CombinatorialCoverage(GenerateContext ctx) {
  InitializeCasesInfo cases_info = InitializeCases(ctx);
  auto rand_f = [&ctx](int n) { return ctx.RandomInteger(n); };

  std::vector<CoveringArrayTestCase> covering_array = GenerateCoveringArray(
      cases_info.dimension_sizes, cases_info.dimension_sizes.size(), rand_f);
  return CreateTestCases(covering_array, cases_info.cases,
                         cases_info.variable_names);
}

}  // namespace moriarty
