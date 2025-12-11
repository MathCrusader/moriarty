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

#include "src/test_case.h"

#include <string_view>
#include <utility>

#include "src/internal/abstract_variable.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"

namespace moriarty {

MTestCase& MTestCase::ConstrainAnonymousVariable(
    std::string_view variable_name,
    const moriarty_internal::AbstractVariable& constraints) {
  variables_.AddOrMergeVariable(variable_name, constraints);
  return *this;
}

MTestCase& MTestCase::UnsafeSetAnonymousValue(std::string_view variable_name,
                                              std::any value) {
  values_.UnsafeSet(variable_name, std::move(value));
  return *this;
}

moriarty_internal::ValueSet& MTestCase::UnsafeGetValues() & { return values_; }
const moriarty_internal::ValueSet& MTestCase::UnsafeGetValues() const& {
  return values_;
}
moriarty_internal::VariableSet& MTestCase::UnsafeGetVariables() & {
  return variables_;
}
const moriarty_internal::VariableSet& MTestCase::UnsafeGetVariables() const& {
  return variables_;
}

TestCase::TestCase(const moriarty_internal::ValueSet& values)
    : values_(values) {}

TestCase& TestCase::UnsafeSetAnonymousValue(std::string_view variable_name,
                                            std::any value) {
  values_.UnsafeSet(variable_name, std::move(value));
  return *this;
}

moriarty_internal::ValueSet& TestCase::UnsafeGetValues() & { return values_; }
const moriarty_internal::ValueSet& TestCase::UnsafeGetValues() const& {
  return values_;
}

}  // namespace moriarty
