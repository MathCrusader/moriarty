/*
 * Copyright 2025 Darcy Best
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MORIARTY_SRC_CONTEXTS_USER_EXPORT_CONTEXT_H_
#define MORIARTY_SRC_CONTEXTS_USER_EXPORT_CONTEXT_H_

#include <functional>
#include <ostream>
#include <stdexcept>
#include <string_view>

#include "src/contexts/internal/basic_ostream_context.h"
#include "src/contexts/internal/name_context.h"
#include "src/contexts/internal/view_only_context.h"
#include "src/contexts/librarian/printer_context.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/test_case.h"

namespace moriarty {

// ExportContext
//
// All context that Exporters have access to.
class ExportContext : public moriarty_internal::ViewOnlyContext,
                      public moriarty_internal::BasicOStreamContext {
  using ViewOnlyContext = moriarty_internal::ViewOnlyContext;
  using BasicOStreamContext = moriarty_internal::BasicOStreamContext;

 public:
  // Created by Moriarty and passed to you; no need to instantiate.
  // See `src/Moriarty.h` for entry points.
  ExportContext(const moriarty_internal::VariableSet& variables,
                std::ostream& os)
      : ViewOnlyContext(variables, {}),  // FIXME: VariableViewContext ?
        BasicOStreamContext(os),
        variables_(variables) {}

  void PrintVariable(std::string_view variable_name) {
    const moriarty_internal::AbstractVariable& variable =
        GetAnonymousVariable(variable_name);
    librarian::PrinterContext ctx(moriarty_internal::NameContext(variable_name),
                                  *this, *this);
    auto status = variable.PrintValue(ctx);
    if (!status.ok()) throw std::runtime_error(status.ToString());
  }

  void PrintVariableFrom(std::string_view variable_name,
                         const ConcreteTestCase& test_case) {
    const moriarty_internal::AbstractVariable& variable =
        GetAnonymousVariable(variable_name);
    moriarty_internal::ValueSet values =
        UnsafeExtractConcreteTestCaseInternals(test_case);
    librarian::PrinterContext ctx(
        moriarty_internal::NameContext(variable_name),
        moriarty_internal::ViewOnlyContext(variables_, values), *this);
    auto status = variable.PrintValue(ctx);
    if (!status.ok()) throw std::runtime_error(status.ToString());
  }

 private:
  std::reference_wrapper<const moriarty_internal::VariableSet> variables_;
};

}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_USER_EXPORT_CONTEXT_H_
