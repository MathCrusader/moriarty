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

#ifndef MORIARTY_SRC_CONTEXTS_LIBRARIAN_READER_CONTEXT_H_
#define MORIARTY_SRC_CONTEXTS_LIBRARIAN_READER_CONTEXT_H_

#include <istream>
#include <string>

#include "src/contexts/internal/analysis_context.h"
#include "src/contexts/internal/basic_istream_context.h"
#include "src/contexts/internal/mutable_values_context.h"
#include "src/contexts/internal/variable_name_context.h"
#include "src/io_config.h"

namespace moriarty {
namespace librarian {

// ReaderContext
//
// All context that MVariable<>::Read() has access to.
class ReaderContext : public moriarty_internal::AnalysisContext,
                      public moriarty_internal::BasicIStreamContext,
                      public moriarty_internal::MutableValuesContext,
                      public moriarty_internal::VariableNameContext {
  using AnalysisBase = moriarty_internal::AnalysisContext;
  using IStreamBase = moriarty_internal::BasicIStreamContext;
  using ValuesBase = moriarty_internal::MutableValuesContext;
  using NameBase = moriarty_internal::VariableNameContext;

 public:
  // Note: Users should not need to create this object. This object will be
  // created by Moriarty and passed to you. See `src/Moriarty.h` for
  // entry-points.
  ReaderContext(const std::string& variable_name, std::istream& is,
                WhitespaceStrictness strictness,
                const moriarty_internal::VariableSet& variables,
                moriarty_internal::ValueSet& values)
      : AnalysisBase(variables, values),
        IStreamBase(is, strictness),
        ValuesBase(values),
        NameBase(variable_name) {}

  // GetVariableName()
  //
  // Returns the name of the variable being printed.
  using NameBase::GetVariableName;

  // ReadToken()
  //
  // Reads the next token in the input stream.
  //
  // If there is whitespace before the next token, depending on the whitespace
  // strictness:
  //
  //  * If `WhitespaceStrictness::kFlexible`, then leading whitespace
  //    will be ignored.
  //  * If `WhitespaceStrictness::kPrecise`, then an exception will be thrown.
  //
  // End of file will throw an exception.
  using IStreamBase::ReadToken;

  // ReadEof()
  //
  // Reads the end of file character from the input stream. If the end of file
  // character is not found, an exception is thrown.
  using IStreamBase::ReadEof;

  // ReadWhitespace()
  //
  // Attempts to reads the next whitespace character from the input stream. If
  // reading is not successful or the whitespace is not what is requested, an
  // exception is thrown.
  //
  // If `WhitespaceMode() == kIgnoreWhitespace`, then this function is a no-op.
  using IStreamBase::ReadWhitespace;

  // GetValue()
  //
  // Returns the value of the variable `variable_name`.
  using AnalysisBase::GetValue;

  // GetVariable()
  //
  // Returns the variable `variable_name`.
  using AnalysisBase::GetVariable;

  // SetValue()
  //
  // Sets the value of the variable `variable_name`.
  using ValuesBase::SetValue;
};

}  // namespace librarian
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_LIBRARIAN_READER_CONTEXT_H_
