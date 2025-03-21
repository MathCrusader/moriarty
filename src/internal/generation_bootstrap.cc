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

#include "src/internal/generation_bootstrap.h"

#include <format>
#include <functional>
#include <queue>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "src/contexts/librarian/analysis_context.h"
#include "src/contexts/librarian/assignment_context.h"
#include "src/contexts/librarian/resolver_context.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/generation_config.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/test_case.h"

namespace moriarty {
namespace moriarty_internal {

namespace {

using DependencyMap = std::unordered_map<std::string, std::vector<std::string>>;
using Heap = std::priority_queue<std::string, std::vector<std::string>,
                                 std::greater<std::string>>;

DependencyMap CreateDependencyMap(const VariableSet& variables) {
  DependencyMap deps;
  for (const auto& [var_name, var_ptr] : variables.ListVariables()) {
    deps[var_name] = var_ptr->GetDependencies();
    for (std::string_view dep : deps[var_name]) {
      if (!variables.Contains(dep)) {
        throw std::runtime_error(std::format(
            "Variable `{}` depends on unknown variable `{}`", var_name, dep));
      }
    }
  }
  return deps;
}

std::vector<std::string> GetGenerationOrder(const VariableSet& variables) {
  DependencyMap deps_map = CreateDependencyMap(variables);
  std::unordered_map<std::string, int> in_degree;
  for (const auto& [var, deps] : deps_map) {
    in_degree.try_emplace(var, 0);
    for (const std::string& dep : deps) in_degree[dep]++;
  }

  Heap pq;
  for (const auto& [v, e] : in_degree)
    if (e == 0) pq.push(v);

  std::vector<std::string> ordered_variables;
  while (!pq.empty()) {
    std::string current = pq.top();
    pq.pop();
    ordered_variables.push_back(current);

    for (const std::string& dep : deps_map.at(current)) {
      if (--in_degree[dep] == 0) pq.push(dep);
    }
  }

  if (ordered_variables.size() != deps_map.size()) {
    throw std::runtime_error(
        "There is a cycle in the MVariable dependency graph.");
  }

  return ordered_variables;
}

}  // namespace

ValueSet GenerateTestCase(TestCase test_case, VariableSet variables,
                          const GenerationOptions& options) {
  auto [extra_constraints, values] = UnsafeExtractTestCaseInternals(test_case);

  for (auto& [name, constraints] : extra_constraints.ListVariables()) {
    variables.AddOrMergeVariable(name, *constraints);
  }

  return GenerateAllValues(variables, values, options);
}

ValueSet GenerateAllValues(VariableSet variables, ValueSet known_values,
                           const GenerationOptions& options) {
  GenerationConfig generation_config;
  std::vector<std::string> order = GetGenerationOrder(variables);

  // First do a quick assignment of all known values. We process these in
  // reverse order so that everyone that a variable depends on is assigned
  // before them.
  for (std::string_view name : std::ranges::reverse_view(order)) {
    AbstractVariable* var = variables.GetAnonymousVariable(name);
    librarian::AssignmentContext ctx(name, variables, known_values);
    var->AssignUniqueValue(ctx);
  }

  // Now do a deep generation.
  for (std::string_view name : order) {
    AbstractVariable* var = variables.GetAnonymousVariable(name);
    librarian::ResolverContext ctx(name, variables, known_values,
                                   options.random_engine, generation_config);
    var->AssignValue(ctx);
  }

  // We may have initially generated invalid values during the
  // AssignUniqueValues(). Let's check for those now...
  // TODO(darcybest): Determine if there's a better way of doing this...
  for (std::string_view name : order) {
    AbstractVariable* var = variables.GetAnonymousVariable(name);
    librarian::AnalysisContext ctx(name, variables, known_values);
    if (!var->IsSatisfiedWithValue(ctx)) {
      throw std::runtime_error(
          std::format("Variable {} does not satisfy its constraints: {}", name,
                      var->UnsatisfiedWithValueReason(ctx)));
    }
  }

  return known_values;
}

}  // namespace moriarty_internal
}  // namespace moriarty
