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

// InputFormat
//
// Specifies how to read and write input for a problem. Any type that has
// `Reader()` and `Writer()` methods that return `ReaderFn` and `WriterFn`
// respectively can be used as an InputFormat. Most commonly, this will be
// `SimpleIO`.
template <typename T>
class InputFormat {
 public:
  explicit InputFormat(T format);
  ReaderFn Reader() const;
  WriterFn Writer() const;

 private:
  T format_;
};

// OutputFormat
//
// Specifies how to read and write output for a problem. Any type that has
// `Reader()` and `Writer()` methods that return `ReaderFn` and `WriterFn`
// respectively can be used as an OutputFormat. Most commonly, this will be
// `SimpleIO`.
template <typename T>
class OutputFormat {
 public:
  explicit OutputFormat(T format);
  ReaderFn Reader() const;
  WriterFn Writer() const;

 private:
  T format_;
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

  const moriarty_internal::VariableSet& GetVariables();

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
  const moriarty_internal::VariableSet& GetVariables() const;
  std::optional<Seed> GetSeed() const;
  std::optional<ReaderFn> GetInputReader() const;
  std::optional<WriterFn> GetInputWriter() const;
  std::optional<ReaderFn> GetOutputReader() const;
  std::optional<WriterFn> GetOutputWriter() const;

 private:
  std::optional<Title> title_;
  moriarty_internal::VariableSet variables_;
  std::optional<Seed> seed_;
  std::optional<ReaderFn> input_reader_;
  std::optional<WriterFn> input_writer_;
  std::optional<ReaderFn> output_reader_;
  std::optional<WriterFn> output_writer_;

  void Apply(Title title);
  void Apply(Variables vars);
  void Apply(Seed seed);
  template <typename T>
  void Apply(InputFormat<T> format);
  template <typename T>
  void Apply(OutputFormat<T> format);
};

// -----------------------------------------------------------------------------
//  Template implementations below

template <typename... T>
Problem::Problem(T... args) {
  (Apply(std::move(args)), ...);
}

template <typename T>
void Problem::Apply(InputFormat<T> format) {
  input_reader_ = format.Reader();
  input_writer_ = format.Writer();
}

template <typename T>
void Problem::Apply(OutputFormat<T> format) {
  output_reader_ = format.Reader();
  output_writer_ = format.Writer();
}

template <typename... T>
Variables::Variables(T... variables) {
  (variables_.SetVariable(variables.GetName(), variables.GetVariable()), ...);
}

template <typename T>
InputFormat<T>::InputFormat(T format) : format_(std::move(format)) {}

template <typename T>
ReaderFn InputFormat<T>::Reader() const {
  return format_.Reader();
}

template <typename T>
WriterFn InputFormat<T>::Writer() const {
  return format_.Writer();
}

template <typename T>
OutputFormat<T>::OutputFormat(T format) : format_(std::move(format)) {}

template <typename T>
ReaderFn OutputFormat<T>::Reader() const {
  return format_.Reader();
}

template <typename T>
WriterFn OutputFormat<T>::Writer() const {
  return format_.Writer();
}

template <MoriartyVariable T>
Var::Var(std::string name, T variable)
    : name_(std::move(name)),
      variable_(std::make_unique<T>(std::move(variable))) {}

}  // namespace moriarty

#endif  // MORIARTY_PROBLEM_H_
