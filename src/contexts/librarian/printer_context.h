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
#include <string_view>

#include "src/contexts/internal/basic_ostream_context.h"
#include "src/contexts/internal/name_context.h"
#include "src/contexts/internal/view_only_context.h"

namespace moriarty {
namespace librarian {

// PrinterContext
//
// All context that MVariable<>::Print() has access to.
class PrinterContext : public moriarty_internal::NameContext,
                       public moriarty_internal::ViewOnlyContext,
                       public moriarty_internal::BasicOStreamContext {
  using NameContext = moriarty_internal::NameContext;
  using ViewOnlyContext = moriarty_internal::ViewOnlyContext;
  using OStreamBase = moriarty_internal::BasicOStreamContext;

 public:
  // Created by Moriarty and passed to you; no need to instantiate.
  // See `src/Moriarty.h` for entry points.
  PrinterContext(std::string_view variable_name, std::ostream& os,
                 const moriarty_internal::VariableSet& variables,
                 const moriarty_internal::ValueSet& values)
      : NameContext(variable_name),
        ViewOnlyContext(variables, values),
        OStreamBase(os) {}
  PrinterContext(NameContext name_context, ViewOnlyContext view_context,
                 OStreamBase stream_context)
      : NameContext(name_context),
        ViewOnlyContext(view_context),
        OStreamBase(stream_context) {}

  // ********************************************
  // ** See parent classes for more functions. **
  // ********************************************
};

}  // namespace librarian
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_LIBRARIAN_PRINTER_CONTEXT_H_
