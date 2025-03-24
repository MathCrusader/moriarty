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
#include <string_view>

#include "src/contexts/internal/basic_istream_context.h"
#include "src/contexts/internal/name_context.h"
#include "src/contexts/internal/view_only_context.h"
#include "src/librarian/policies.h"

namespace moriarty {
namespace librarian {

// ReaderContext
//
// All context that MVariable<>::Read() has access to.
class ReaderContext : public moriarty_internal::NameContext,
                      public moriarty_internal::ViewOnlyContext,
                      public moriarty_internal::BasicIStreamContext {
 public:
  // Created by Moriarty and passed to you; no need to instantiate.
  // See `src/Moriarty.h` for entry points.
  ReaderContext(
      std::string_view variable_name, std::reference_wrapper<std::istream> is,
      WhitespaceStrictness whitespace_strictness,
      std::reference_wrapper<const moriarty_internal::VariableSet> variables,
      std::reference_wrapper<const moriarty_internal::ValueSet> values)
      : NameContext(variable_name),
        ViewOnlyContext(variables, values),
        BasicIStreamContext(is, whitespace_strictness) {}
  ReaderContext(NameContext name_context, ViewOnlyContext view_context,
                BasicIStreamContext stream_context)
      : NameContext(name_context),
        ViewOnlyContext(view_context),
        BasicIStreamContext(stream_context) {}

  // ********************************************
  // ** See parent classes for more functions. **
  // ********************************************
};

}  // namespace librarian
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_LIBRARIAN_READER_CONTEXT_H_
