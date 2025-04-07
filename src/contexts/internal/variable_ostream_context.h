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

#ifndef MORIARTY_SRC_CONTEXTS_INTERNAL_VARIABLE_OSTREAM_CONTEXT_H_
#define MORIARTY_SRC_CONTEXTS_INTERNAL_VARIABLE_OSTREAM_CONTEXT_H_

#include <functional>
#include <ostream>

#include "src/internal/abstract_variable.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/test_case.h"

namespace moriarty {
namespace moriarty_internal {

// VariableOStreamContext
//
// Print variables to an output stream.
class VariableOStreamContext {
 public:
  VariableOStreamContext(std::reference_wrapper<std::ostream> os,
                         std::reference_wrapper<const VariableSet> variables,
                         std::reference_wrapper<const ValueSet> values);

  // PrintVariable()
  //
  // Prints the value of `variable_name` to the output stream.
  void PrintVariable(std::string_view variable_name);

  // PrintVariable()
  //
  // Prints `value` to the output stream using `variable` to determine how to do
  // so.
  template <MoriartyVariable T>
  void PrintVariable(T variable, T::value_type value);

  // PrintVariableFrom()
  //
  // Prints the value of `variable_name` from `test_case` to the output stream.
  void PrintVariableFrom(std::string_view variable_name,
                         const ConcreteTestCase& test_case);

 protected:
  void UpdateVariableOStream(std::reference_wrapper<std::ostream> os) {
    os_ = os;
  }

 private:
  std::reference_wrapper<const VariableSet> variables_;
  std::reference_wrapper<const ValueSet> values_;
  std::reference_wrapper<std::ostream> os_;
};

// -----------------------------------------------------------------------------
//  Template implementation below

template <MoriartyVariable T>
void VariableOStreamContext::PrintVariable(T variable, T::value_type value) {
  variable.Print({"PrintVariable", os_, variables_, values_}, value);
}

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_INTERNAL_VARIABLE_OSTREAM_CONTEXT_H_
