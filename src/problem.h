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

#ifndef MORIARTY_PROBLEM_H_
#define MORIARTY_PROBLEM_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "src/context.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/variable_set.h"

namespace moriarty {

// Title
//
// The title of a problem.
class Title {
 public:
  explicit Title(std::string_view title);
  explicit operator std::string() const;

 private:
  std::string title_;
};

// Seed
//
// The seed for a problem's random generation. Note that both problems and
// generators need seeds. They will be combined into a single seed for
// generation.
class Seed {
 public:
  explicit Seed(std::string_view seed);
  explicit operator std::string() const;

 private:
  std::string seed_;
};

template <typename T>
concept MoriartyFormat = requires(T t) {
  { t.Reader() } -> std::same_as<ReaderFn>;
  { t.Writer() } -> std::same_as<WriterFn>;
  { t.GetDependencies() } -> std::same_as<std::vector<std::string>>;
};

namespace moriarty_internal {

class Format {
 public:
  Format(ReaderFn reader, WriterFn writer,
         std::vector<std::string> dependencies)
      : reader_(std::move(reader)),
        writer_(std::move(writer)),
        dependencies_(std::move(dependencies)) {}

  ReaderFn Reader() const { return reader_; }
  WriterFn Writer() const { return writer_; }
  std::vector<std::string> GetDependencies() const { return dependencies_; }

 private:
  ReaderFn reader_;
  WriterFn writer_;
  std::vector<std::string> dependencies_;
};

}  // namespace moriarty_internal

// InputFormat
//
// Specifies how to read and write input for a problem. Any type that has
// `Reader()`, `Writer()`, and `GetDependencies()` methods can be used as an
// InputFormat. Most commonly, this will be `SimpleIO`.
class InputFormat {
 public:
  template <MoriartyFormat Fmt>
  explicit InputFormat(Fmt format);

  ReaderFn Reader() const;
  WriterFn Writer() const;
  std::vector<std::string> GetDependencies() const;

  [[nodiscard]] moriarty_internal::Format UnsafeGetFormat() const;

 private:
  moriarty_internal::Format format_;
};

// OutputFormat
//
// Specifies how to read and write output for a problem. Any type that has
// `Reader()`, `Writer()`, and `GetDependencies()` methods can be used as an
// OutputFormat. Most commonly, this will be `SimpleIO`.
class OutputFormat {
 public:
  template <MoriartyFormat Fmt>
  explicit OutputFormat(Fmt format);

  ReaderFn Reader() const;
  WriterFn Writer() const;
  std::vector<std::string> GetDependencies() const;

  [[nodiscard]] moriarty_internal::Format UnsafeGetFormat() const;

 private:
  moriarty_internal::Format format_;
};

// Var
//
// A named variable for use in `Variables`.
class Var {
 public:
  template <MoriartyVariable T>
  Var(std::string name, T variable);

  const std::string& GetName() const;
  const moriarty_internal::AbstractVariable& GetVariable() const;

 private:
  std::string name_;
  std::unique_ptr<moriarty_internal::AbstractVariable> variable_;
};

// Variables
//
// A set of named variables. Use a comma-separated list in its constructor.
// Example usage:
//
// Variables(
//   Var("N", MInteger(Between(1, 100))),
//   Var("A", MArray<MInteger>(Elements<MInteger>(Between(3, 5)),
//                             Length("3 * N + 1"))),
//   Var("S", MString(Alphabet("abc"), Length("N")))
// );

class Variables {
 public:
  template <typename... T>
  Variables(T... variables);

  const moriarty_internal::VariableSet& UnsafeGetVariables();

 private:
  moriarty_internal::VariableSet variables_;
};

// Problem
//
// A full specification of a problem. Depending on what you're doing, you may
// only need to provide some of the information here. However, we recommend you
// make a full Problem specification, even if some parts are unused.
//
// Example usage:
//
//   Problem p(
//     Title("Example Problem"),
//     Variables(
//       Var("N", MInteger(Between(1, 100))),
//       Var("A", MArray<MInteger>(Elements<MInteger>(Between(1, 3)),
//                                 Length("N"))),
//       Var("S", MString(Alphabet("abc"), Length("N"))),
//       Var("X", MInteger(Between(20, 25)))
//     ),
//     Seed("example_seed"),
//     InputFormat(SimpleIO().AddLine("N", "S").AddLine("A")),
//     OutputFormat(SimpleIO().AddLine("X"))
//   );
class Problem {
 public:
  template <typename... T>
  explicit Problem(T... args);

  std::optional<Title> GetTitle() const;
  std::optional<Seed> GetSeed() const;
  std::optional<ReaderFn> GetInputReader() const;
  std::optional<WriterFn> GetInputWriter() const;
  std::optional<std::vector<std::string>> GetInputDependencies() const;
  std::optional<ReaderFn> GetOutputReader() const;
  std::optional<WriterFn> GetOutputWriter() const;
  std::optional<std::vector<std::string>> GetOutputDependencies() const;

  [[nodiscard]] std::vector<int64_t> BaseSeedForGenerator(
      std::string_view generator_seed) const;
  const moriarty_internal::VariableSet& UnsafeGetVariables() const;

 private:
  std::optional<Title> title_;
  moriarty_internal::VariableSet variables_;
  std::optional<Seed> seed_;
  std::optional<ReaderFn> input_reader_;
  std::optional<WriterFn> input_writer_;
  std::optional<std::vector<std::string>> input_dependencies_;
  std::optional<ReaderFn> output_reader_;
  std::optional<WriterFn> output_writer_;
  std::optional<std::vector<std::string>> output_dependencies_;

  void Apply(Title title);
  void Apply(Variables vars);
  void Apply(Seed seed);
  void Apply(InputFormat format);
  void Apply(OutputFormat format);
};

// -----------------------------------------------------------------------------
//  Template implementations below

template <typename... T>
Problem::Problem(T... args) {
  (Apply(std::move(args)), ...);
}

template <typename... T>
Variables::Variables(T... variables) {
  (variables_.SetVariable(variables.GetName(), variables.GetVariable()), ...);
}

template <MoriartyFormat Fmt>
InputFormat::InputFormat(Fmt format)
    : format_(format.Reader(), format.Writer(), format.GetDependencies()) {}

template <MoriartyFormat Fmt>
OutputFormat::OutputFormat(Fmt format)
    : format_(format.Reader(), format.Writer(), format.GetDependencies()) {}

template <MoriartyVariable T>
Var::Var(std::string name, T variable)
    : name_(std::move(name)),
      variable_(std::make_unique<T>(std::move(variable))) {}

}  // namespace moriarty

#endif  // MORIARTY_PROBLEM_H_
