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

std::vector<TestCase> ReadTestCases(const ReaderFn& reader, ReadContext ctx) {
  std::vector<TestCase> test_cases = reader(ctx);
  ctx.ReadEof();

  return test_cases;
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

  std::vector<TestCase> test_cases = ReadTestCases(*reader, ctx);
  if (test_cases.empty())
    throw ConfigurationError("ValidateInput::Run", "No Test Cases.");

  ValidateTestCases(problem_, test_cases);
}

ValidateOutputBuilder::ValidateOutputBuilder(Problem problem)
    : problem_(std::move(problem)) {}

ValidateOutputBuilder& ValidateOutputBuilder::ReadInputUsing(ReadOptions opts) {
  input_options_ = std::move(opts);
  return *this;
}

ValidateOutputBuilder& ValidateOutputBuilder::ReadOutputUsing(
    ReadOptions opts) {
  output_options_ = std::move(opts);
  return *this;
}

ValidateOutputBuilder ValidateOutput(Problem problem) {
  return ValidateOutputBuilder(std::move(problem));
}

void ValidateOutputBuilder::Run() const {
  if (!input_options_) {
    throw ConfigurationError("ValidateInput::Run",
                             "std::istream needed for input. Use "
                             "ReadInputUsing() to specify options.");
  }
  if (!output_options_) {
    throw ConfigurationError("ValidateOutput::Run",
                             "std::istream needed for output. Use "
                             "ReadOutputUsing() to specify options.");
  }
  auto input_reader = problem_.GetInputReader();
  if (!input_reader) {
    throw ConfigurationError(
        "ValidateOutput::Run",
        "No InputFormat specified in Problem. Cannot read input.");
  }
  auto output_reader = problem_.GetOutputReader();
  if (!output_reader) {
    throw ConfigurationError(
        "ValidateOutput::Run",
        "No OutputFormat specified in Problem. Cannot read output.");
  }

  InputCursor input_cursor(input_options_->istream,
                           input_options_->whitespace_strictness);
  ReadContext ctx(problem_.UnsafeGetVariables(), input_cursor);

  std::vector<TestCase> test_cases = ReadTestCases(*input_reader, ctx);
  if (test_cases.empty())
    throw ConfigurationError("ValidateOutput::Run", "No Test Cases.");
  ValidateTestCases(problem_, test_cases);

  if (test_cases.size() != 1) {
    throw ConfigurationError(
        "ValidateOutput::Run",
        "ValidateOutput currently only works with exactly 1 test case.");
  }

  InputCursor output_cursor(output_options_->istream,
                            output_options_->whitespace_strictness);
  ReadContext output_ctx(problem_.UnsafeGetVariables(),
                         test_cases[0].UnsafeGetValues(), output_cursor);
  std::vector<TestCase> output_answers =
      ReadTestCases(*output_reader, output_ctx);
  ValidateTestCases(problem_, output_answers);
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
  if (generators_.empty()) {
    throw ConfigurationError("Generate::Run",
                             "No generators specified. Call `Using()` to add "
                             "generators.");
  }

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

AnalyzeBuilder::AnalyzeBuilder(Problem problem)
    : problem_(std::move(problem)) {}

AnalyzeBuilder& AnalyzeBuilder::ReadInputUsing(ReadOptions opts) {
  input_reader_ = std::move(opts);
  return *this;
}

AnalyzeBuilder& AnalyzeBuilder::ReadOutputUsing(ReadOptions opts) {
  output_reader_ = std::move(opts);
  return *this;
}

AnalyzeBuilder Analyze(Problem problem) {
  return AnalyzeBuilder(std::move(problem));
}

void AnalyzeBuilder::Run() const {
  if (!input_reader_) {
    throw ConfigurationError("Analyze::Run",
                             "std::istream needed for input. Use "
                             "ReadInputUsing() to specify options.");
  }
  if (analyzers_.empty()) {
    throw ConfigurationError("Analyze::Run",
                             "No analyzers specified. Call `Using()` to add "
                             "analyzers.");
  }

  InputCursor cursor(input_reader_->istream,
                     input_reader_->whitespace_strictness);
  ReadContext ctx(problem_.UnsafeGetVariables(), cursor);

  auto reader = problem_.GetInputReader();
  if (!reader) {
    throw ConfigurationError(
        "Analyze::Run",
        "No InputFormat specified in Problem. Cannot read input.");
  }

  std::vector<TestCase> test_cases = (*reader)(ctx);
  ctx.ReadEof();

  if (test_cases.empty())
    throw ConfigurationError("Analyze::Run", "No Test Cases read in input.");

  if (output_reader_) {
    InputCursor cursor(output_reader_->istream,
                       output_reader_->whitespace_strictness);
    ReadContext ctx(problem_.UnsafeGetVariables(), cursor);

    auto reader = problem_.GetOutputReader();
    if (!reader) {
      throw ConfigurationError(
          "Analyze::Run",
          "No OutputFormat specified in Problem. Cannot read output.");
    }

    std::vector<TestCase> outputs = (*reader)(ctx);
    ctx.ReadEof();

    if (outputs.size() != test_cases.size()) {
      throw ValidationError(
          "Analyze::Run",
          std::format("Number of output test cases ({}) does not match "
                      "number of input test cases ({}).",
                      outputs.size(), test_cases.size()));
    }

    // TODO: views::zip when we can use C++23
    for (int i = 0; i < test_cases.size(); i++) {
      test_cases[i].UnsafeGetValues().DestructiveMergeFrom(
          outputs[i].UnsafeGetValues());
    }
  }

  for (const auto& [name, analyzer] : analyzers_) {
    moriarty_internal::ValueSet values;
    AnalyzeContext ctx(problem_.UnsafeGetVariables(), values);
    analyzer(ctx, test_cases);
  }
}

}  // namespace moriarty
