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
#include <type_traits>
#include <variant>
#include <vector>

#include "src/context.h"
#include "src/problem.h"
#include "src/test_case.h"

namespace moriarty {

// ValidateInputBuilder
//
// Validates the input for a problem. The InputFormat is specified in the
// `problem`.
//
// Usage:
//
// ValidateInput(problem)
//   .ReadInputUsing({.istream = std::cin})
//   .Run();
class ValidateInputBuilder {
 public:
  // Creates a builder. The InputFormat from `problem` will be used to read.
  explicit ValidateInputBuilder(Problem problem);

  // Adds options for reading input.
  ValidateInputBuilder& ReadInputUsing(ReadOptions opts);

  // Runs the validation. If there is an error, this throws an exception. In
  // general, exceptions from Moriarty derive from `GenericMoriartyError`. Other
  // exceptions are likely bugs.
  //
  // FIXME: This comment is a slight lie, as not all exceptions currently
  // derive from GenericMoriartyError. This will be fixed in a future release.
  void Run() const;

 private:
  Problem problem_;
  std::optional<ReadOptions> input_options_;
};

// ValidateInput
//
// Creates a builder to validate input for `problem`.
//
// ValidateInput(problem)
//   .ReadInputUsing({.istream = std::cin})
//   .Run();
[[nodiscard]] ValidateInputBuilder ValidateInput(Problem problem);

// -----------------------------------------------------------------------------

// Valid return types for a MoriartyGenerator.
template <typename T>
concept ValidGeneratorReturnType =
    std::same_as<std::decay_t<T>, TestCase> ||
    std::same_as<std::decay_t<T>, MTestCase> ||
    std::same_as<std::decay_t<T>, std::vector<TestCase>> ||
    std::same_as<std::decay_t<T>, std::vector<MTestCase>>;

// Determines whether a function is a MoriartyGenerator.
template <typename T>
concept MoriartyGenerator = requires(T t, GenerateContext ctx) {
  { t(ctx) } -> ValidGeneratorReturnType;
};

// GenerateBuilder
//
// Generates test cases for a problem. The InputFormat and OutputFormat are
// specified in the `problem`.
//
// Usage:
// Generate(problem)
//   .Using("MyGenerator", Gen, {.num_runs = 10})
//   .Using("AnotherGenerator", AnotherGen)
//   .WriteInputUsing({ .ostream = std::cout })
//   .WriteOutputUsing({ .ostream = std::cout })
//   .Run();
class GenerateBuilder {
 public:
  // Creates a builder.
  explicit GenerateBuilder(Problem problem);

  // Adds a generator to use. The generator will be run with the given options.
  // Generators may return either `TestCase`, `MTestCase` (or std::vector<> of
  // either). If `generator` returns an `MTestCase`, then random values will be
  // assigned to all variables, using the extra constraints set in the
  // `MTestCase`.
  //
  // In general, we recommend that generators return `TestCase`(s) directly only
  // if you are making a very specific test case, and should return
  // `MTestCase`(s) for every other case.
  //
  // We recommend you provide the answer as part of the generator if you can
  // easily compute a special answer (without just calling your actual
  // solution). Example, if you are writing a "path generator", then it is
  // (sometimes) possible to compute the answer in a different way.
  template <MoriartyGenerator Generator>
  GenerateBuilder& Using(std::string name, Generator generator,
                         GenerateOptions options = {});

  // Where/how to write the inputs of the test cases.
  GenerateBuilder& WriteInputUsing(WriteOptions opts);

  // Where/how to write the outputs of the test cases (optional).
  GenerateBuilder& WriteOutputUsing(WriteOptions opts);

  // Generates the test cases, writes them (if requested), and returns them.
  std::vector<TestCase> Run() const;

 private:
  Problem problem_;

  struct NamedGenerator {
    using ToTestCase = std::function<std::vector<TestCase>(GenerateContext)>;
    using ToMTestCase = std::function<std::vector<MTestCase>(GenerateContext)>;

    std::string name;
    std::variant<ToTestCase, ToMTestCase> generator;
    GenerateOptions options;
  };
  std::vector<NamedGenerator> generators_;
  std::optional<WriteOptions> input_writer_;
  std::optional<WriteOptions> output_writer_;
};

// -----------------------------------------------------------------------------
//  Template implementations below

template <MoriartyGenerator Generator>
GenerateBuilder& GenerateBuilder::Using(std::string name, Generator generator,
                                        GenerateOptions options) {
  using ReturnType =
      std::decay_t<decltype(generator(std::declval<GenerateContext>()))>;

  generators_.push_back({
      .name = std::move(name),
      .options = std::move(options),
  });
  auto& gen = generators_.back();

  if constexpr (std::same_as<ReturnType, TestCase>) {
    gen.generator = [generator](GenerateContext ctx) -> std::vector<TestCase> {
      return {generator(ctx)};
    };
  } else if constexpr (std::same_as<ReturnType, std::vector<TestCase>>) {
    gen.generator = [generator](GenerateContext ctx) -> std::vector<TestCase> {
      return generator(ctx);
    };
  } else if constexpr (std::same_as<ReturnType, MTestCase>) {
    gen.generator = [generator](GenerateContext ctx) -> std::vector<MTestCase> {
      return {generator(ctx)};
    };
  } else if constexpr (std::same_as<ReturnType, std::vector<MTestCase>>) {
    gen.generator = [generator](GenerateContext ctx) -> std::vector<MTestCase> {
      return generator(ctx);
    };
  } else {
    static_assert(false,
                  "Unhandled return type in MoriartyGenerator. The "
                  "types in the concept don't match the list here.");
  }

  return *this;
}

}  // namespace moriarty

#endif  // MORIARTY_ACTIONS_H_
