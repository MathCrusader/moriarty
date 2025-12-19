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

namespace moriarty {

const std::string& Var::GetName() const { return name_; }

const moriarty_internal::AbstractVariable& Var::GetVariable() const {
  return *variable_;
}

const moriarty_internal::VariableSet& Variables::GetVariables() {
  return variables_;
}

std::optional<Title> Problem::GetTitle() const { return title_; }

const moriarty_internal::VariableSet& Problem::GetVariables() const {
  return variables_;
}

std::optional<Seed> Problem::GetSeed() const { return seed_; }

std::optional<ReaderFn> Problem::GetInputReader() const {
  return input_reader_;
}

std::optional<WriterFn> Problem::GetInputWriter() const {
  return input_writer_;
}

std::optional<ReaderFn> Problem::GetOutputReader() const {
  return output_reader_;
}

std::optional<WriterFn> Problem::GetOutputWriter() const {
  return output_writer_;
}

void Problem::Apply(Title title) { title_ = std::move(title); }

void Problem::Apply(Variables vars) { variables_ = vars.GetVariables(); }

void Problem::Apply(Seed seed) { seed_ = std::move(seed); }

Title::Title(std::string_view title) : title_(title) {}

Title::operator std::string() const { return title_; }

Seed::Seed(std::string_view seed) : seed_(seed) {}

Seed::operator std::string() const { return seed_; }

}  // namespace moriarty
