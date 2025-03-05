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

#ifndef MORIARTY_SRC_CONTEXTS_USER_IMPORT_CONTEXT_H_
#define MORIARTY_SRC_CONTEXTS_USER_IMPORT_CONTEXT_H_

#include <concepts>
#include <iostream>
#include <istream>
#include <string_view>

#include "src/contexts/internal/basic_istream_context.h"
#include "src/contexts/internal/mutable_values_context.h"
#include "src/contexts/internal/name_context.h"
#include "src/contexts/internal/view_only_context.h"
#include "src/contexts/librarian/reader_context.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/value_set.h"
#include "src/io_config.h"
#include "src/test_case.h"

namespace moriarty {

// ImportContext
//
// All context that Importers have access to.
class ImportContext : public moriarty_internal::ViewOnlyContext,
                      public moriarty_internal::BasicIStreamContext {
  using ViewOnlyContext = moriarty_internal::ViewOnlyContext;
  using BasicIStreamContext = moriarty_internal::BasicIStreamContext;

 public:
  // Created by Moriarty and passed to you; no need to instantiate.
  // See `src/Moriarty.h` for entry points.
  ImportContext(const moriarty_internal::VariableSet& variables,
                std::istream& is, WhitespaceStrictness whitespace_strictness)
      : ViewOnlyContext(variables, {}),
        BasicIStreamContext(is, whitespace_strictness),
        variables_(variables) {}

  // ********************************************
  // ** See parent classes for more functions. **
  // ********************************************

  template <typename T>
    requires std::derived_from<T, moriarty_internal::AbstractVariable>
  T::value_type ReadVariable(std::string_view variable_name) {
    auto variable = GetVariable<T>(variable_name);
    if (!variable.ok()) throw std::runtime_error(variable.status().ToString());
    librarian::ReaderContext ctx(moriarty_internal::NameContext(variable_name),
                                 *this, *this);
    return variable.Read(ctx);
  }

  template <typename T>
    requires std::derived_from<T, moriarty_internal::AbstractVariable>
  T::value_type ReadVariable(const T& variable) {
    librarian::ReaderContext ctx(moriarty_internal::NameContext("ReadVariable"),
                                 *this, *this);
    return variable.Read(ctx);
  }

  void ReadVariableTo(std::string_view variable_name,
                      ConcreteTestCase& test_case) {
    moriarty_internal::ValueSet values;
    moriarty_internal::ViewOnlyContext view_only_ctx(variables_, values);
    moriarty_internal::MutableValuesContext mutable_values_ctx(values);
    librarian::ReaderContext reader_ctx(
        moriarty_internal::NameContext(variable_name), view_only_ctx, *this);

    const moriarty_internal::AbstractVariable& variable =
        GetAnonymousVariable(variable_name);
    auto status = variable.ReadValue(reader_ctx, mutable_values_ctx);
    if (!status.ok()) throw std::runtime_error(status.ToString());

    // * hides a StatusOr<>
    test_case.UnsafeSetAnonymousValue(variable_name,
                                      *values.UnsafeGet(variable_name));
  }

 private:
  std::reference_wrapper<const moriarty_internal::VariableSet> variables_;
};

}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_USER_IMPORT_CONTEXT_H_
