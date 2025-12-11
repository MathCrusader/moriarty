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

#include "src/context.h"

#include "src/contexts/internal/basic_istream_context.h"
#include "src/contexts/internal/basic_random_context.h"
#include "src/contexts/internal/variable_random_context.h"
#include "src/contexts/internal/view_only_context.h"
#include "src/internal/random_engine.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/io_config.h"
#include "src/librarian/util/ref.h"

namespace moriarty {

GenerateContext::GenerateContext(
    Ref<const moriarty_internal::VariableSet> variables,
    Ref<const moriarty_internal::ValueSet> values,
    Ref<moriarty_internal::RandomEngine> rng)
    : ViewOnlyContext(variables, values),
      BasicRandomContext(rng),
      VariableRandomContext(variables, values, rng) {}

ImportContext::ImportContext(
    Ref<const moriarty_internal::VariableSet> variables, Ref<InputCursor> input)
    : ViewOnlyContext(variables, values_),
      BasicIStreamContext(input),
      VariableIStreamContext(input, variables, values_) {}

ExportContext::ExportContext(
    Ref<std::ostream> os, Ref<const moriarty_internal::VariableSet> variables,
    Ref<const moriarty_internal::ValueSet> values)
    : ViewOnlyContext(variables, values),
      BasicOStreamContext(os),
      VariableOStreamContext(os, variables, values) {}

ExportContext::ExportContext(ExportContext ctx, Ref<std::ostream> os)
    : ViewOnlyContext(ctx),
      BasicOStreamContext(os),
      VariableOStreamContext(ctx) {
  VariableOStreamContext::UpdateVariableOStream(os);
}

ConstraintContext::ConstraintContext(
    std::string_view variable_name,
    Ref<const moriarty_internal::VariableSet> variables,
    Ref<const moriarty_internal::ValueSet> values)
    : NameContext(variable_name), ViewOnlyContext(variables, values) {}

ConstraintContext::ConstraintContext(std::string_view name,
                                     const ViewOnlyContext& other)
    : NameContext(name), ViewOnlyContext(other) {}

void ValidationResults::AddFailure(int case_num, std::string reason) {
  failures_.emplace_back(case_num, std::move(reason));
}

bool ValidationResults::IsValid() const { return failures_.empty(); }

std::string ValidationResults::DescribeFailures() const {
  if (failures_.empty()) return "";

  std::string result;
  for (const auto& [case_num, reason] : failures_) {
    if (case_num != 0)
      result += std::format("Case #{} invalid:\n{}", case_num, reason);
    else
      result += std::format("Invalid test case:\n{}", reason);
  }
  return result;
}

}  // namespace moriarty
