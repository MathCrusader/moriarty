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

#ifndef MORIARTY_ACTIONS_H_
#define MORIARTY_ACTIONS_H_

#include <optional>
#include <string>

#include "src/context.h"
#include "src/problem.h"

namespace moriarty {

// ValidateInputBuilder
//
// Validates the input for a problem. The InputFormat is specified in the
// `problem`.
class ValidateInputBuilder {
 public:
  // Creates a builder. The InputFormat from `problem` will be used to read.
  explicit ValidateInputBuilder(Problem problem);

  // Adds options for reading input.
  ValidateInputBuilder& ReadInputUsing(ReadOptions opts);

  // Runs the validation. If there is an error, this will throw an exception. If
  // you want to catch the exception, use `RunReturningError()`, which will
  // catch and return an error as a string.
  void Run() const;

  // Runs the validation. If there is an error, it will be returned as a string.
  // Note that this only catches exceptions directly related to Moriarty, not
  // all exceptions.
  std::optional<std::string> RunReturningError() const;

 private:
  Problem problem_;
  std::optional<ReadOptions> input_options_;
};

// ValidateInput
//
// Creates a builder to validate input for `problem`.
//
// ValidateInput(problem)
//       .ReadInputUsing({.istream = std::cin})
//       .Run();
[[nodiscard]] ValidateInputBuilder ValidateInput(Problem problem);

}  // namespace moriarty

#endif  // MORIARTY_ACTIONS_H_
