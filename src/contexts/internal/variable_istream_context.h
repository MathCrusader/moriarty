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

#ifndef MORIARTY_SRC_CONTEXTS_INTERNAL_VARIABLE_ISTREAM_CONTEXT_H_
#define MORIARTY_SRC_CONTEXTS_INTERNAL_VARIABLE_ISTREAM_CONTEXT_H_

#include <functional>
#include <istream>
#include <string_view>

#include "src/contexts/librarian/reader_context.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/io_config.h"
#include "src/test_case.h"

namespace moriarty {
namespace moriarty_internal {

// VariableIStreamContext
//
// Read variables from an input stream.
class VariableIStreamContext {
 public:
  VariableIStreamContext(std::reference_wrapper<std::istream> is,
                         WhitespaceStrictness whitespace_strictness,
                         std::reference_wrapper<const VariableSet> variables,
                         std::reference_wrapper<const ValueSet> values);

  // ReadVariable()
  //
  // Reads a known variable from the input stream and returns what was read.
  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  [[nodiscard]] T::value_type ReadVariable(std::string_view variable_name);

  // ReadVariable()
  //
  // Reads something from the input stream, using `variable` to define how to
  // read it, and returns what was read.
  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  [[nodiscard]] T::value_type ReadVariable(const T& variable);

  // ReadVariableTo()
  //
  // Reads a known variable from the input stream and stores its value into
  // `test_case`.
  void ReadVariableTo(std::string_view variable_name,
                      ConcreteTestCase& test_case);

 private:
  std::reference_wrapper<const VariableSet> variables_;
  std::reference_wrapper<const ValueSet> values_;
  std::reference_wrapper<std::istream> is_;
  WhitespaceStrictness whitespace_strictness_;
};

// ----------------------------------------------------------------------------
//  Template implementation below

template <typename T>
  requires std::derived_from<T, AbstractVariable>
T::value_type VariableIStreamContext::ReadVariable(
    std::string_view variable_name) {
  T variable = variables_.get().GetVariable<T>(variable_name);

  librarian::ReaderContext ctx(variable_name, is_, whitespace_strictness_,
                               variables_, values_);
  return variable.Read(ctx);
}

template <typename T>
  requires std::derived_from<T, AbstractVariable>
T::value_type VariableIStreamContext::ReadVariable(const T& variable) {
  librarian::ReaderContext ctx("ReadVariable()", is_, whitespace_strictness_,
                               variables_, values_);
  return variable.Read(ctx);
}

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_INTERNAL_VARIABLE_ISTREAM_CONTEXT_H_
