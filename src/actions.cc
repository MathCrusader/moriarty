// Copyright 2025 Darcy Best
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

#include "src/actions.h"

#include <iterator>
#include <vector>

#include "src/context.h"
#include "src/internal/analysis_bootstrap.h"
#include "src/internal/generation_bootstrap.h"
#include "src/internal/random_engine.h"
#include "src/librarian/errors.h"
#include "src/problem.h"
#include "src/test_case.h"

namespace moriarty {

ValidateInputBuilder::ValidateInputBuilder(Problem problem)
    : problem_(std::move(problem)) {}

ValidateInputBuilder& ValidateInputBuilder::ReadInputUsing(ReadOptions opts) {
  input_options_ = std::move(opts);
  return *this;
}

ValidateInputBuilder ValidateInput(Problem problem) {
  return ValidateInputBuilder(std::move(problem));
}

namespace {

std::string FailuresToString(
    const std::vector<DetailedConstraintViolation>& failures) {
  std::string result;

  for (const auto& [var_name, reason] : failures) {
    if (!result.empty()) result += "\n";
    result += std::format(" - Variable `{}` failed constraint: {}\n", var_name,
                          reason.Reason());
  }
  return result;
}

void ValidateTestCases(const Problem& problem,
                       const std::vector<TestCase>& test_cases) {
  if (test_cases.empty()) {
    throw ValidationError("ValidateTestCases", "No Test Cases.");
  }

  for (int case_num = 1; const auto& test_case : test_cases) {
    std::vector<DetailedConstraintViolation> failures =
        moriarty_internal::CheckValues(problem.UnsafeGetVariables(),
                                       test_case.UnsafeGetValues(), {},
                                       ValidationStyle::kOnlySetVariables);
    if (!failures.empty()) {
      if (test_cases.size() == 1) {
        throw ValidationError(FailuresToString(failures));
      }
      throw ValidationError(std::format("Case #{} invalid:\n{}", case_num,
                                        FailuresToString(failures)));
    }
    case_num++;
  }
}

}  // namespace

void ValidateInputBuilder::Run() const {
  if (!input_options_) {
    throw ConfigurationError("ValidateInput::Run",
                             "std::istream needed for input. Use "
                             "ReadInputUsing() to specify options.");
  }
  // FIXME: Remove validation styles from options.
  // if (input_options_->validation != ValidationStyle::kOnlySetVariables) {
  //   throw ConfigurationError("ValidateInput::Run",
  //                            "Only ValidationStyle::kOnlySetVariables is "
  //                            "supported for input validation.");
  // }

  InputCursor cursor(input_options_->istream,
                     input_options_->whitespace_strictness);
  ReadContext ctx(problem_.UnsafeGetVariables(), cursor);

  auto reader = problem_.GetInputReader();
  if (!reader) {
    throw ConfigurationError(
        "ValidateInput::Run",
        "No InputFormat specified in Problem. Cannot read input.");
  }

  std::vector<TestCase> test_cases;
  test_cases = (*reader)(ctx);
  ctx.ReadEof();

  if (test_cases.empty())
    throw ConfigurationError("ValidateInput::Run", "No Test Cases.");

  ValidateTestCases(problem_, test_cases);
}

GenerateBuilder Generate(Problem problem) {
  return GenerateBuilder(std::move(problem));
}

GenerateBuilder::GenerateBuilder(Problem problem)
    : problem_(std::move(problem)) {}

GenerateBuilder& GenerateBuilder::WriteInputUsing(WriteOptions opts) {
  input_writer_ = std::move(opts);
  return *this;
}

GenerateBuilder& GenerateBuilder::WriteOutputUsing(WriteOptions opts) {
  output_writer_ = std::move(opts);
  return *this;
}

std::vector<TestCase> GenerateBuilder::Run() const {
  std::vector<TestCase> all_test_cases;

  for (const auto& [name, generator, options] : generators_) {
    std::vector<int64_t> seed =
        problem_.BaseSeedForGenerator(options.seed.value_or(name));
    seed.push_back(0);  // Placeholder for call number

    for (int call = 1; call <= options.num_calls; call++) {
      seed.back() = call;

      moriarty_internal::ValueSet values;
      moriarty_internal::RandomEngine rng(seed, "v0.1");
      GenerateContext ctx(problem_.UnsafeGetVariables(), values, rng);

      std::vector<TestCase> generated_cases;
      if (std::holds_alternative<NamedGenerator::ToTestCase>(generator)) {
        const auto& gen = std::get<NamedGenerator::ToTestCase>(generator);
        generated_cases = gen(ctx);
      } else {
        const auto& gen = std::get<NamedGenerator::ToMTestCase>(generator);
        for (const MTestCase& test_case : gen(ctx)) {
          generated_cases.push_back(
              TestCase(moriarty_internal::GenerateAllValues(
                  problem_.UnsafeGetVariables(), test_case.UnsafeGetVariables(),
                  test_case.UnsafeGetValues(),
                  {.random_engine = rng,
                   .variables_to_generate =
                       problem_.GetInputDependencies().value_or(
                           std::vector<std::string>{})})));
        }
      }

      if (generated_cases.empty()) {
        throw ValidationError(
            "Generate::Run",
            std::format("Generator '{}' produced no test cases.", name));
      }

      all_test_cases.insert(all_test_cases.end(),
                            std::make_move_iterator(generated_cases.begin()),
                            std::make_move_iterator(generated_cases.end()));
    }
  }

  ValidateTestCases(problem_, all_test_cases);
  if (input_writer_) {
    if (!problem_.GetInputWriter()) {
      throw ConfigurationError(
          "Generate::Run",
          "No InputFormat specified in Problem. Cannot write input.");
    }
    auto values = moriarty_internal::ValueSet{};
    WriteContext ctx(input_writer_->ostream, problem_.UnsafeGetVariables(),
                     values);

    std::invoke(*problem_.GetInputWriter(), ctx, all_test_cases);
  }
  if (output_writer_) {
    if (!problem_.GetOutputWriter()) {
      throw ConfigurationError(
          "Generate::Run",
          "No OutputFormat specified in Problem. Cannot write output.");
    }
    auto values = moriarty_internal::ValueSet{};
    WriteContext ctx(output_writer_->ostream, problem_.UnsafeGetVariables(),
                     values);

    std::invoke(*problem_.GetOutputWriter(), ctx, all_test_cases);
  }

  return all_test_cases;
}

}  // namespace moriarty
