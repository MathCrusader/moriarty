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

#include "src/context.h"
#include "src/internal/analysis_bootstrap.h"
#include "src/librarian/errors.h"
#include "src/problem.h"

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

  InputCursor cursor(input_options_->is, input_options_->whitespace_strictness);
  ReadContext ctx(problem_.GetVariables(), cursor);

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

  for (int case_num = 1; const auto& test_case : test_cases) {
    std::vector<DetailedConstraintViolation> failures =
        moriarty_internal::CheckValues(problem_.GetVariables(),
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

}  // namespace moriarty
