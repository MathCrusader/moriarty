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

#include "src/problem.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "src/librarian/errors.h"

namespace moriarty {

const std::string& Var::GetName() const { return name_; }

const moriarty_internal::AbstractVariable& Var::GetVariable() const {
  return *variable_;
}

const moriarty_internal::VariableSet& Variables::UnsafeGetVariables() {
  return variables_;
}

ReaderFn InputFormat::Reader() const { return format_.Reader(); }

WriterFn InputFormat::Writer() const { return format_.Writer(); }

std::vector<std::string> InputFormat::GetDependencies() const {
  return format_.GetDependencies();
}

ReaderFn OutputFormat::Reader() const { return format_.Reader(); }

WriterFn OutputFormat::Writer() const { return format_.Writer(); }

std::vector<std::string> OutputFormat::GetDependencies() const {
  return format_.GetDependencies();
}

std::optional<Title> Problem::GetTitle() const { return title_; }

const moriarty_internal::VariableSet& Problem::UnsafeGetVariables() const {
  return variables_;
}

std::optional<Seed> Problem::GetSeed() const { return seed_; }

std::optional<ReaderFn> Problem::GetInputReader() const {
  return input_reader_;
}

std::optional<WriterFn> Problem::GetInputWriter() const {
  return input_writer_;
}

std::optional<std::vector<std::string>> Problem::GetInputDependencies() const {
  return input_dependencies_;
}

std::optional<ReaderFn> Problem::GetOutputReader() const {
  return output_reader_;
}

std::optional<WriterFn> Problem::GetOutputWriter() const {
  return output_writer_;
}

std::optional<std::vector<std::string>> Problem::GetOutputDependencies() const {
  return output_dependencies_;
}

std::vector<int64_t> Problem::BaseSeedForGenerator(
    std::string_view generator_seed) const {
  if (!seed_) {
    throw ConfigurationError("Problem::BaseSeedForGenerator",
                             "Problem seed is not set when generating.");
  }

  std::vector<int64_t> seed_vector;
  seed_vector.reserve(std::string(*seed_).size() + generator_seed.size());
  std::ranges::copy(static_cast<std::string>(*seed_),
                    std::back_inserter(seed_vector));
  std::ranges::copy(generator_seed, std::back_inserter(seed_vector));
  return seed_vector;
}

void Problem::Apply(Title title) { title_ = std::move(title); }

void Problem::Apply(Variables vars) { variables_ = vars.UnsafeGetVariables(); }

void Problem::Apply(Seed seed) { seed_ = std::move(seed); }

void Problem::Apply(InputFormat format) {
  input_reader_ = format.Reader();
  input_writer_ = format.Writer();
  input_dependencies_ = format.GetDependencies();
}

void Problem::Apply(OutputFormat format) {
  output_reader_ = format.Reader();
  output_writer_ = format.Writer();
  output_dependencies_ = format.GetDependencies();
}

Title::Title(std::string_view title) : title_(title) {}

Title::operator std::string() const { return title_; }

Seed::Seed(std::string_view seed) : seed_(seed) {}

Seed::operator std::string() const { return seed_; }

}  // namespace moriarty
