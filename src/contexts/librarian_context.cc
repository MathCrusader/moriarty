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

#include "src/contexts/librarian_context.h"

#include <ostream>
#include <string_view>
#include <utility>

#include "src/contexts/internal/basic_istream_context.h"
#include "src/contexts/internal/basic_ostream_context.h"
#include "src/contexts/internal/mutable_values_context.h"
#include "src/contexts/internal/name_context.h"
#include "src/contexts/internal/view_only_context.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/io_config.h"
#include "src/librarian/util/ref.h"

namespace moriarty {
namespace librarian {

// ---------------------------------------------------
// AnalyzeVariableContext

AnalyzeVariableContext::AnalyzeVariableContext(
    std::string_view variable_name,
    Ref<const moriarty_internal::VariableSet> variables,
    Ref<const moriarty_internal::ValueSet> values)
    : NameContext(variable_name), ViewOnlyContext(variables, values) {}

AnalyzeVariableContext::AnalyzeVariableContext(
    const AnalyzeVariableContext& other, NameContext name)
    : NameContext(std::move(name)), ViewOnlyContext(other) {}

AnalyzeVariableContext AnalyzeVariableContext::ForSubVariable(
    std::string_view sub_variable_name) const {
  return AnalyzeVariableContext(*this,
                                NameContext::ForSubVariable(sub_variable_name));
}

AnalyzeVariableContext AnalyzeVariableContext::ForIndexedSubVariable(
    std::function<std::string(int)> indexed_name_fn, int index) const {
  return AnalyzeVariableContext(
  *this, NameContext::ForIndexedSubVariable(indexed_name_fn, index));
}

// ---------------------------------------------------
// AssignVariableContext

AssignVariableContext::AssignVariableContext(
    std::string_view variable_name,
    Ref<const moriarty_internal::VariableSet> variables,
    Ref<moriarty_internal::ValueSet> values)
    : NameContext(variable_name),
      ViewOnlyContext(variables, values),
      MutableValuesContext(values) {}

AssignVariableContext::AssignVariableContext(
    const AssignVariableContext& other, NameContext new_name)
    : NameContext(std::move(new_name)),
      ViewOnlyContext(other),
      MutableValuesContext(other) {}

AssignVariableContext AssignVariableContext::ForSubVariable(
    std::string_view sub_variable_name) const {
  return AssignVariableContext(*this,
                               NameContext::ForSubVariable(sub_variable_name));
}

AssignVariableContext AssignVariableContext::ForIndexedSubVariable(
    std::function<std::string(int)> indexed_name_fn, int index) const {
  return AssignVariableContext(
      *this, NameContext::ForIndexedSubVariable(indexed_name_fn, index));
}

// ---------------------------------------------------
// WriteVariableContext

WriteVariableContext::WriteVariableContext(
    std::string_view variable_name, Ref<std::ostream> os,
    Ref<const moriarty_internal::VariableSet> variables,
    Ref<const moriarty_internal::ValueSet> values)
    : NameContext(variable_name),
      ViewOnlyContext(variables, values),
      BasicOStreamContext(os) {}

WriteVariableContext::WriteVariableContext(
    const WriteVariableContext& other, NameContext new_name)
    : NameContext(std::move(new_name)),
      ViewOnlyContext(other),
      BasicOStreamContext(other) {}

WriteVariableContext WriteVariableContext::ForSubVariable(
    std::string_view sub_variable_name) const {
  return WriteVariableContext(*this,
                              NameContext::ForSubVariable(sub_variable_name));
}

WriteVariableContext WriteVariableContext::ForIndexedSubVariable(
    std::function<std::string(int)> indexed_name_fn, int index) const {
  return WriteVariableContext(
      *this, NameContext::ForIndexedSubVariable(indexed_name_fn, index));
}

// ---------------------------------------------------
// ReadVariableContext

ReadVariableContext::ReadVariableContext(
    std::string_view variable_name, Ref<InputCursor> input,
    Ref<const moriarty_internal::VariableSet> variables,
    Ref<const moriarty_internal::ValueSet> values)
    : NameContext(variable_name),
      ViewOnlyContext(variables, values),
      BasicIStreamContext(input) {}

ReadVariableContext::ReadVariableContext(const ReadVariableContext& other,
                                         NameContext new_name)
    : NameContext(std::move(new_name)),
      ViewOnlyContext(other),
      BasicIStreamContext(other) {}

ReadVariableContext ReadVariableContext::ForSubVariable(
    std::string_view sub_variable_name) const {
  return ReadVariableContext(*this,
                             NameContext::ForSubVariable(sub_variable_name));
}

ReadVariableContext ReadVariableContext::ForIndexedSubVariable(
    std::function<std::string(int)> indexed_name_fn, int index) const {
  return ReadVariableContext(
      *this, NameContext::ForIndexedSubVariable(indexed_name_fn, index));
}

// ---------------------------------------------------
// GenerateVariableContext

GenerateVariableContext::GenerateVariableContext(
    std::string_view variable_name,
    Ref<const moriarty_internal::VariableSet> variables,
    Ref<moriarty_internal::ValueSet> values,
    Ref<moriarty_internal::RandomEngine> engine,
    Ref<moriarty_internal::GenerationHandler> handler)
    : NameContext(variable_name),
      ViewOnlyContext(variables, values),
      MutableValuesContext(values),
      ResolveValuesContext(variables, values, engine, handler),
      BasicRandomContext(engine),
      GenerationOrchestrationContext(handler),
      variables_(variables),
      values_(values),
      engine_(engine),
      handler_(handler) {}

GenerateVariableContext::GenerateVariableContext(
    const GenerateVariableContext& other, NameContext new_name)
    : NameContext(std::move(new_name)),
      ViewOnlyContext(other),
      MutableValuesContext(other),
      ResolveValuesContext(other),
      BasicRandomContext(other),
      GenerationOrchestrationContext(other),
      variables_(other.variables_),
      values_(other.values_),
      engine_(other.engine_),
      handler_(other.handler_) {}

GenerateVariableContext GenerateVariableContext::ForSubVariable(
    std::string_view sub_variable_name) const {
  return GenerateVariableContext(*this,
                                 NameContext::ForSubVariable(sub_variable_name));
}

GenerateVariableContext GenerateVariableContext::ForIndexedSubVariable(
    std::function<std::string(int)> indexed_name_fn, int index) const {
  return GenerateVariableContext(
      *this, NameContext::ForIndexedSubVariable(indexed_name_fn, index));
}

}  // namespace librarian
}  // namespace moriarty
