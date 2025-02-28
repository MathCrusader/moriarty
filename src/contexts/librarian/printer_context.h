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

#ifndef MORIARTY_SRC_CONTEXTS_LIBRARIAN_PRINTER_CONTEXT_H_
#define MORIARTY_SRC_CONTEXTS_LIBRARIAN_PRINTER_CONTEXT_H_

#include <ostream>
#include <string>

#include "src/contexts/internal/basic_ostream_context.h"
#include "src/contexts/internal/variable_name_context.h"
#include "src/contexts/librarian/analysis_context.h"

namespace moriarty {
namespace librarian {

// PrinterContext
//
// All context that MVariable<>::Print() has access to.
class PrinterContext : public AnalysisContext,
                       public moriarty_internal::BasicOStreamContext,
                       public moriarty_internal::VariableNameContext {
  using AnalysisBase = AnalysisContext;
  using OStreamBase = moriarty_internal::BasicOStreamContext;
  using NameBase = moriarty_internal::VariableNameContext;

 public:
  // Note: Users should not need to create this object. This object will be
  // created by Moriarty and passed to you. See `src/Moriarty.h` for
  // entry-points.
  PrinterContext(const std::string& variable_name, std::ostream& os,
                 const moriarty_internal::VariableSet& variables,
                 const moriarty_internal::ValueSet& values)
      : AnalysisBase(variables, values),
        OStreamBase(os),
        NameBase(variable_name) {}

  // GetVariableName()
  //
  // Returns the name of the variable being printed.
  using NameBase::GetVariableName;

  // PrintToken()
  //
  // Prints a single token to the output stream.
  using OStreamBase::PrintToken;

  // PrintWhitespace()
  //
  // Prints the whitespace character to the output stream.
  using OStreamBase::PrintWhitespace;

  // GetValue()
  //
  // Returns the value of the variable `variable_name`.
  using AnalysisBase::GetValue;

  // GetVariable()
  //
  // Returns the variable `variable_name`.
  using AnalysisBase::GetVariable;
};

}  // namespace librarian
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_LIBRARIAN_PRINTER_CONTEXT_H_
