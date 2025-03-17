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

#include "src/internal/variable_set.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/substitute.h"
#include "src/errors.h"
#include "src/internal/abstract_variable.h"

namespace moriarty {
namespace moriarty_internal {

// Copy constructor, making a copy of all constraints
VariableSet::VariableSet(const VariableSet& other) {
  for (const auto& [variable_name, abstract_var_ptr] : other.variables_) {
    variables_.emplace(variable_name, abstract_var_ptr->Clone());
  }
}

VariableSet& VariableSet::operator=(VariableSet other) {
  Swap(other);
  return *this;
}

void VariableSet::Swap(VariableSet& other) {
  std::swap(variables_, other.variables_);
}

absl::StatusOr<const AbstractVariable*> VariableSet::GetAbstractVariable(
    std::string_view name) const {
  const AbstractVariable* var = GetAbstractVariableOrNull(name);
  if (var == nullptr) throw VariableNotFound(name);
  return var;
}

absl::StatusOr<AbstractVariable*> VariableSet::GetAbstractVariable(
    std::string_view name) {
  AbstractVariable* var = GetAbstractVariableOrNull(name);
  if (var == nullptr) throw VariableNotFound(name);
  return var;
}

AbstractVariable* VariableSet::GetAbstractVariableOrNull(
    std::string_view name) {
  auto it = variables_.find(name);
  if (it == variables_.end()) return nullptr;
  return it->second.get();
}

const AbstractVariable* VariableSet::GetAbstractVariableOrNull(
    std::string_view name) const {
  auto it = variables_.find(name);
  if (it == variables_.end()) return nullptr;
  return it->second.get();
}

const absl::flat_hash_map<std::string, std::unique_ptr<AbstractVariable>>&
VariableSet::GetAllVariables() const {
  return variables_;
}

absl::Status VariableSet::AddVariable(std::string_view name,
                                      const AbstractVariable& variable) {
  auto [it, inserted] = variables_.emplace(name, variable.Clone());

  if (!inserted)
    return absl::AlreadyExistsError(absl::Substitute(
        "Variable '$0' already added to to this VariableSet instance", name));

  return absl::OkStatus();
}

absl::Status VariableSet::AddOrMergeVariable(std::string_view name,
                                             const AbstractVariable& variable) {
  auto [it, inserted] = variables_.try_emplace(name, nullptr);
  if (inserted) {
    it->second = std::unique_ptr<AbstractVariable>(variable.Clone());
    return absl::OkStatus();
  }

  return it->second->MergeFrom(variable);
}

}  // namespace moriarty_internal
}  // namespace moriarty
