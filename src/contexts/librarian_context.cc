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

AnalysisContext::AnalysisContext(
    std::string_view variable_name,
    Ref<const moriarty_internal::VariableSet> variables,
    Ref<const moriarty_internal::ValueSet> values)
    : NameContext(variable_name), ViewOnlyContext(variables, values) {}

AnalysisContext::AnalysisContext(std::string_view name,
                                 const ViewOnlyContext& other)
    : NameContext(name), ViewOnlyContext(other) {}

AssignmentContext::AssignmentContext(
    std::string_view variable_name,
    Ref<const moriarty_internal::VariableSet> variables,
    Ref<moriarty_internal::ValueSet> values)
    : NameContext(variable_name),
      ViewOnlyContext(variables, values),
      MutableValuesContext(values) {}

PrinterContext::PrinterContext(
    std::string_view variable_name, Ref<std::ostream> os,
    Ref<const moriarty_internal::VariableSet> variables,
    Ref<const moriarty_internal::ValueSet> values)
    : NameContext(variable_name),
      ViewOnlyContext(variables, values),
      BasicOStreamContext(os) {}

ReaderContext::ReaderContext(
    std::string_view variable_name, Ref<InputCursor> input,
    Ref<const moriarty_internal::VariableSet> variables,
    Ref<const moriarty_internal::ValueSet> values)
    : NameContext(variable_name),
      ViewOnlyContext(variables, values),
      BasicIStreamContext(input) {}

ResolverContext::ResolverContext(
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

ResolverContext ResolverContext::ForVariable(std::string_view new_name) const {
  return ResolverContext(new_name, variables_, values_, engine_, handler_);
}

ResolverContext ResolverContext::ForSubVariable(
    std::string_view new_name) const {
  return ResolverContext(std::format("{}.{}", GetVariableName(), new_name),
                         variables_, values_, engine_, handler_);
}

}  // namespace librarian
}  // namespace moriarty
