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
#include <memory>
#include <string_view>

#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/io_config.h"
#include "src/test_case.h"

namespace moriarty {
namespace moriarty_internal {

// VariableIStreamContext
//
// Read variables from an input stream.
class VariableIStreamContext {
 public:
  VariableIStreamContext(std::reference_wrapper<InputCursor> input,
                         std::reference_wrapper<const VariableSet> variables,
                         std::reference_wrapper<const ValueSet> values);

  // ReadVariable()
  //
  // Reads a known variable from the input stream and returns what was read.
  //
  // FIXME: This is silly. No local context is known. If this variable depends
  // on another, you must use `ReadVariableTo()`.
  template <MoriartyVariable T>
  [[nodiscard]] T::value_type ReadVariable(std::string_view variable_name);

  // ReadVariable()
  //
  // Reads something from the input stream, using `variable` to define how to
  // read it, and returns what was read.
  template <MoriartyVariable T>
  [[nodiscard]] T::value_type ReadVariable(T variable, std::string_view name);

  // ReadVariableTo()
  //
  // Reads a known variable from the input stream and stores its value into
  // `test_case`.
  //
  // FIXME: The only set values come from `test_case`, not the global values.
  void ReadVariableTo(std::string_view variable_name,
                      ConcreteTestCase& test_case);

  // GetPartialReader()
  //
  // Returns `variable_name`'s partial reader. This is used to read
  // a variable from the input stream over multiple calls.
  std::unique_ptr<moriarty_internal::PartialReader> GetPartialReader(
      std::string_view variable_name, int calls,
      ConcreteTestCase& test_case) const;

 private:
  std::reference_wrapper<const VariableSet> variables_;
  std::reference_wrapper<const ValueSet> values_;
  std::reference_wrapper<InputCursor> input_;
};

// ----------------------------------------------------------------------------
//  Template implementation below

template <MoriartyVariable T>
T::value_type VariableIStreamContext::ReadVariable(
    std::string_view variable_name) {
  T variable = variables_.get().GetVariable<T>(variable_name);

  return variable.Read({variable_name, input_, variables_, values_});
}

template <MoriartyVariable T>
T::value_type VariableIStreamContext::ReadVariable(T variable,
                                                   std::string_view name) {
  return variable.Read({name, input_, variables_, values_});
}

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_INTERNAL_VARIABLE_ISTREAM_CONTEXT_H_
