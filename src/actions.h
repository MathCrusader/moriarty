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
// Validates the input for a problem. The InputFormat must be specified in
// `problem`.
//
// Usage:
//
// ValidateInput(problem)
//   .ReadInputUsing({.istream = std::cin})
//   .Run();
class ValidateInputBuilder {
 public:
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
  // Construct a builder using `ValidateInput(problem)`.
  explicit ValidateInputBuilder(Problem problem);
  friend ValidateInputBuilder ValidateInput(Problem problem);

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

// ValidateOutputBuilder
//
// Validates the output for a problem. Both the InputFormat and OutputFormat
// must be specified in `problem`.
//
// FIXME: Currently only works with a single test case. This will be fixed in a
// future release.
//
// Usage:
//
// ValidateOutput(problem)
//   .ReadInputUsing({.istream = std::cin})
//   .ReadOutputUsing({.istream = std::fstream(" ... ")})
//   .Run();
class ValidateOutputBuilder {
 public:
  // Adds options for reading input.
  ValidateOutputBuilder& ReadInputUsing(ReadOptions opts);

  // Adds options for reading output.
  ValidateOutputBuilder& ReadOutputUsing(ReadOptions opts);

  // Runs the validation. If there is an error, this throws an exception. In
  // general, exceptions from Moriarty derive from `GenericMoriartyError`. Other
  // exceptions are likely bugs.
  //
  // FIXME: This comment is a slight lie, as not all exceptions currently
  // derive from GenericMoriartyError. This will be fixed in a future release.
  //
  // Reads the input first, then reads the output. The output will have access
  // to the input variables and their values that were read.
  void Run() const;

 private:
  // Construct a builder using `ValidateOutput(problem)`.
  explicit ValidateOutputBuilder(Problem problem);
  friend ValidateOutputBuilder ValidateOutput(Problem problem);

  Problem problem_;
  std::optional<ReadOptions> input_options_;
  std::optional<ReadOptions> output_options_;
};

// ValidateOutput
//
// Creates a builder to validate output for `problem`.
//
// FIXME: Currently only works with a single test case. This will be fixed in a
// future release.
//
// ValidateOutput(problem)
//   .ReadInputUsing({.istream = std::cin})
//   .ReadOutputUsing({.istream = std::fstream(" ... ")})
//   .Run();
[[nodiscard]] ValidateOutputBuilder ValidateOutput(Problem problem);

// -----------------------------------------------------------------------------

// Valid return types for a MoriartyGenerator.
template <typename T>
concept ValidGeneratorReturnType =
    std::same_as<std::remove_cvref_t<T>, TestCase> ||
    std::same_as<std::remove_cvref_t<T>, MTestCase> ||
    std::same_as<std::remove_cvref_t<T>, std::vector<TestCase>> ||
    std::same_as<std::remove_cvref_t<T>, std::vector<MTestCase>>;

// Determines whether a function is a MoriartyGenerator.
template <typename T>
concept MoriartyGenerator = requires(T t, GenerateContext ctx) {
  { t(ctx) } -> ValidGeneratorReturnType;
};

// GenerateBuilder
//
// Generates test cases for a problem and optionally writes them. The
// InputFormat and OutputFormat are specified in the `problem`.
//
// Usage:
//
// Generate(problem)
//   .Using("MyGenerator", Gen, {.num_runs = 10})
//   .Using("AnotherGenerator", AnotherGen)
//   .WriteInputUsing({ .ostream = std::cout })
//   .WriteOutputUsing({ .ostream = std::cout })
//   .Run();
class GenerateBuilder {
 public:
  // Adds a generator to use. Valid function signatures for `generator` are:
  //
  //  auto foo(GenerateContext ctx) -> TestCase
  //  auto foo(GenerateContext ctx) -> std::vector<TestCase>
  //  auto foo(GenerateContext ctx) -> MTestCase
  //  auto foo(GenerateContext ctx) -> std::vector<MTestCase>
  //
  // * An `MTestCase` will fill in all unspecified variables using the
  //   appropriate random generators.
  // * A `TestCase` will be taken exactly as-is.
  //
  // We recommend that most generators return `MTestCase`(s). You can use
  // `MTestCase` to add more constraints ("e.g., I want a small odd value in
  // this test case"). `TestCase`(s) should be returned only in the case where
  // an *exact* example is needed.
  //
  // We recommend you provide the answer as part of the generator if you can
  // easily compute a special answer *without just calling your actual
  // solution*. Example, if you are writing a "path generator", then it is
  // (sometimes) possible to compute the answer in a different way.
  //
  // Note: `name` is used as the generator's seed if a specific seed is not
  // provided in `options`.
  template <MoriartyGenerator Generator>
  GenerateBuilder& Using(std::string name, Generator generator,
                         GenerateOptions options = {});

  // Specifies where/how to write the inputs of the test cases (optional).
  GenerateBuilder& WriteInputUsing(WriteOptions opts);

  // Specifies where/how to write the outputs of the test cases (optional).
  GenerateBuilder& WriteOutputUsing(WriteOptions opts);

  // Generates the test cases, writes them (if requested), and returns them.
  std::vector<TestCase> Run() const;

 private:
  // Construct a builder using `Generate(problem)`.
  explicit GenerateBuilder(Problem problem);
  friend GenerateBuilder Generate(Problem problem);

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

// Generate
//
// Generates test cases for a problem and optionally writes them. The
// InputFormat and OutputFormat are specified in the `problem`.
//
// Usage:
//
// Generate(problem)
//   .Using("MyGenerator", Gen, {.num_runs = 10})
//   .Using("AnotherGenerator", AnotherGen)
//   .WriteInputUsing({ .ostream = std::cout })
//   .WriteOutputUsing({ .ostream = std::cout })
//   .Run();
GenerateBuilder Generate(Problem problem);

// -----------------------------------------------------------------------------

// Determines if a function is a MoriartyAnalyzer with a single TestCase.
template <typename T>
concept SingleTestCaseAnalyzer =
    requires(T t, AnalyzeContext ctx, TestCase tc) {
      { t(ctx, tc) } -> std::same_as<void>;
    };

// Determines if a function is a MoriartyAnalyzer with multiple TestCases.
template <typename T>
concept MultiTestCasesAnalyzer =
    requires(T t, AnalyzeContext ctx, std::vector<TestCase> cases) {
      { t(ctx, cases) } -> std::same_as<void>;
    };

// Determines whether a function is a MoriartyAnalyzer.
template <typename T>
concept MoriartyAnalyzer =
    SingleTestCaseAnalyzer<T> || MultiTestCasesAnalyzer<T>;

// AnalyzeBuilder
//
// Analyzes test cases for a problem. Analyzers are most typically used to
// compute statistics on the test cases.
//
// Usage:
//
// Analyze(problem)
//   .Using("MyGenerator", Gen, {.num_runs = 10})
//   .Using("AnotherGenerator", AnotherGen)
//   .ReadInputUsing({ .istream = std::cin })
//   .ReadOutputUsing({ .istream = std::fstream(" ... ") })
//   .Run();
class AnalyzeBuilder {
 public:
  // Adds an analyzer to use. Valid function signatures for `analyzer` are:
  //
  //  auto foo(AnalyzeContext ctx, const TestCase& tc) -> void
  //  auto foo(AnalyzeContext ctx, const std::vector<TestCase>& test_cases) ->
  //  auto foo(AnalyzeContext ctx, std::span<const TestCase> test_cases) -> void
  //
  // If the container version is used, then all test cases are provided at
  // once. If the single test case version is used, then the analyzer is called
  // once per test case.
  template <MoriartyAnalyzer Analyzer>
  AnalyzeBuilder& Using(std::string name, Analyzer analyzer);

  // Reads the input of the test case(s) using the specified options.
  AnalyzeBuilder& ReadInputUsing(ReadOptions opts);

  // Reads the output of the test case(s) using the specified options.
  //
  // FIXME: Right now, output variables do not have access to the values of the
  // input variables. This will be fixed in a future release.
  AnalyzeBuilder& ReadOutputUsing(ReadOptions opts);

  // Runs each test case through each analyzer.
  void Run() const;

 private:
  // Construct a builder using `Generate(problem)`.
  explicit AnalyzeBuilder(Problem problem);
  friend AnalyzeBuilder Analyze(Problem problem);

  Problem problem_;

  struct NamedAnalyzer {
    std::string name;
    std::function<void(AnalyzeContext, std::span<const TestCase>)> analyzer;
  };
  std::vector<NamedAnalyzer> analyzers_;
  std::optional<ReadOptions> input_reader_;
  std::optional<ReadOptions> output_reader_;
};

// Analyze
//
// Analyzes test cases for a problem.
//
// Usage:
//
// Analyze(problem)
//   .Using("Find extremes", FindExtremes)
//   .Using("Determine connectivity", DetermineConnectivity)
//   .ReadInputUsing({ .istream = std::cin })
//   .ReadOutputUsing({ .istream = std::cin })
//   .Run();
AnalyzeBuilder Analyze(Problem problem);

// -----------------------------------------------------------------------------
//  Template implementations below

template <MoriartyGenerator Generator>
GenerateBuilder& GenerateBuilder::Using(std::string name, Generator generator,
                                        GenerateOptions options) {
  using ReturnType =
      std::remove_cvref_t<std::invoke_result_t<Generator, GenerateContext>>;

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

template <MoriartyAnalyzer Analyzer>
AnalyzeBuilder& AnalyzeBuilder::Using(std::string name, Analyzer analyzer) {
  analyzers_.push_back({
      .name = std::move(name),
  });
  auto& a = analyzers_.back();

  if constexpr (SingleTestCaseAnalyzer<Analyzer>) {
    a.analyzer = [analyzer](AnalyzeContext ctx,
                            std::span<const TestCase> cases) {
      for (const TestCase& tc : cases) analyzer(ctx, tc);
    };
  } else if constexpr (MultiTestCasesAnalyzer<Analyzer>) {
    a.analyzer = [analyzer](AnalyzeContext ctx,
                            std::span<const TestCase> cases) {
      analyzer(ctx, cases);
    };
  } else {
    static_assert(false,
                  "Unhandled function signature in MoriartyAnalyzer. The "
                  "types in the concept don't match the list here.");
  }

  return *this;
}

}  // namespace moriarty

#endif  // MORIARTY_ACTIONS_H_
