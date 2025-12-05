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

// Moriarty is a data generation/validation library. Moriarty provides a
// centralized language for data generators to speak in. Moreover, interactions
// between different parameters you are generating is allowed/encouraged.
//
// New data types can be added by subject matter experts and used by everyone
// else.

#ifndef MORIARTY_SRC_MORIARTY_H_
#define MORIARTY_SRC_MORIARTY_H_

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "src/context.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"

namespace moriarty {

// Moriarty
//
// This is the central class of the Moriarty suite. Variables should be declared
// through this class. Then Importers, Generators, and Exporters will use those
// variables.
//
// Example usage:
//
//   Moriarty M;
//   M.SetName("Example Constraints")
//     .AddVariable("N", MInteger(Between(1, 100)))
//     .AddVariable("A", MArray<MInteger>(Elements<MInteger>(Between(3, 5)),
//                                        Length("3 * N + 1")))
//     .AddVariable("S", MString(Alphabet("abc"), Length("N")));
//   M.GenerateTestCases(FancyGenerator);
//   M.GenerateTestCases(SmallCaseGenerator(), {.call_n_times = 5});
//   M.ExportTestCase(FancyPrinter);
class Moriarty {
 public:
  // SetName() [required]
  //
  // This is the name of this question. This is useful to distinguish different
  // questions (for interviews/competitions), CUJs, etc. The name is required
  // and is encoded into the random seed to ensure a different seed is provided
  // for each question.
  Moriarty& SetName(std::string_view name);

  // SetNumCases() [optional]
  //
  // Sets an aspirational number of test cases to generate. All custom
  // generators will be called first, then specialized generators (min_, max_,
  // random_) will be called to increase the number of cases to `num_cases`. If
  // the custom generators produce more than `num_cases`, then those will all
  // still be generated. Setting `num_cases` = 0 means only your custom
  // generators will be run.
  Moriarty& SetNumCases(int num_cases);

  // SetSeed() [required]
  //
  // This is the seed used for random generation. The seed must:
  //  * Be at least 10 characters long.
  //
  // In the future, this may also be added as a requirement:
  //  * The first X characters must encode the `name` provided (this helps
  //    ensures a distinct seed is used for every question).
  Moriarty& SetSeed(std::string_view seed);

  // AddVariable()
  //
  // Adds a variable to Moriarty with all global constraints applied to it. For
  // example:
  //
  //   M.AddVariable("N", MInteger(Between(1, 10)));
  //
  // means that *all* instances of N that are generated will be between 1
  // and 10. Additional local constraints can be added to this in specific
  // generators, but no generator may violate these constraints placed here.
  // (For example, if a generator says it wants N to be between 20 and 30, an
  // error will be thrown since there is no number that is between 1 and 10 AND
  // 20 and 30.)
  //
  // Variable names must start with a letter (A-Za-z), and then only contain
  // letters, numbers, and underscores (A-Za-z0-9_).
  template <MoriartyVariable T>
  Moriarty& AddVariable(std::string_view name, T variable);

  // AddAnonymousVariable()
  //
  // Same as AddVariable, but you do not know the type of the variable at
  // compile-time. It is possible this will be deprecated in the future.
  Moriarty& AddAnonymousVariable(
      std::string_view name,
      const moriarty_internal::AbstractVariable& variable);

  // GenerateTestCases()
  //
  // Generates test cases using the provided generator. The generator will be
  // called `num_calls` times. If `num_calls` is not provided, it will be
  // called once.
  void GenerateTestCases(GenerateFn fn, GenerateOptions options = {});

  // ImportTestCases()
  //
  // Imports test cases using the provided importer. The importer will be called
  // once.
  void ImportTestCases(ImportFn fn, ImportOptions options = {});

  // ExportTestCases()
  //
  // Exports test cases using the provided exporter. The exporter will be called
  // once.
  void ExportTestCases(ExportFn fn, ExportOptions options = {}) const;

  // ValidateTestCases()
  //
  // Checks if all variables in all test cases are valid. If there are
  // any failures, this will return one of them. If this return std::nullopt,
  // then the test cases are valid.
  [[nodiscard]] std::optional<std::string> ValidateTestCases();

 private:
  // Seed info
  static constexpr int kMinimumSeedLength = 10;
  std::vector<int64_t> seed_;

  // Metadata
  std::string name_;
  int num_cases_ = 0;

  // Variables
  moriarty_internal::VariableSet variables_;

  // TestCases
  std::vector<moriarty_internal::ValueSet> assigned_test_cases_;

  // Generates the seed for generator_[index]. Negative numbers are reserved
  // for specialized generators (e.g., min_, max_, random_ generators).
  std::span<const int64_t> GetSeedForGenerator(int index);
};

// -----------------------------------------------------------------------------
//  Template implementation below

template <MoriartyVariable T>
Moriarty& Moriarty::AddVariable(std::string_view name, T variable) {
  return AddAnonymousVariable(name, variable);
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_MORIARTY_H_
