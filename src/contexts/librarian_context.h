// Copyright 2025 Darcy Best
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

#ifndef MORIARTY_SRC_CONTEXTS_LIBRARIAN_CONTEXT_H_
#define MORIARTY_SRC_CONTEXTS_LIBRARIAN_CONTEXT_H_

#include <ostream>
#include <string_view>

#include "src/contexts/internal/basic_istream_context.h"
#include "src/contexts/internal/basic_ostream_context.h"
#include "src/contexts/internal/basic_random_context.h"
#include "src/contexts/internal/generation_orchestration_context.h"
#include "src/contexts/internal/mutable_values_context.h"
#include "src/contexts/internal/name_context.h"
#include "src/contexts/internal/resolve_values_context.h"
#include "src/contexts/internal/view_only_context.h"
#include "src/internal/generation_handler.h"
#include "src/internal/random_engine.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/io_config.h"
#include "src/librarian/util/ref.h"

namespace moriarty {
namespace librarian {

// AnalysisContext
//
// Allows you to inspect the current state of the variables and values.
// AnalysisContext is read-only. It does not allow you to modify the variables
// or values.
//
// Note: All Librarian contexts can be converted to this type.
class AnalysisContext : public moriarty_internal::NameContext,
                        public moriarty_internal::ViewOnlyContext {
 public:
  AnalysisContext(std::string_view variable_name,
                  Ref<const moriarty_internal::VariableSet> variables,
                  Ref<const moriarty_internal::ValueSet> values);
  AnalysisContext(std::string_view name, const ViewOnlyContext& other);
  template <typename T>
  AnalysisContext(const T& other)
      : NameContext(other.GetVariableName()), ViewOnlyContext(other) {}

  // ********************************************
  // ** See parent classes for more functions. **
  // ********************************************
};

// AssignmentContext
//
// Allows you to inspect the current state of the variables and set values.
class AssignmentContext : public moriarty_internal::NameContext,
                          public moriarty_internal::ViewOnlyContext,
                          public moriarty_internal::MutableValuesContext {
 public:
  AssignmentContext(std::string_view variable_name,
                    Ref<const moriarty_internal::VariableSet> variables,
                    Ref<moriarty_internal::ValueSet> values);

  // ********************************************
  // ** See parent classes for more functions. **
  // ********************************************
};

// PrinterContext
//
// All context that MVariable<>::Print() has access to.
class PrinterContext : public moriarty_internal::NameContext,
                       public moriarty_internal::ViewOnlyContext,
                       public moriarty_internal::BasicOStreamContext {
 public:
  PrinterContext(std::string_view variable_name, Ref<std::ostream> os,
                 Ref<const moriarty_internal::VariableSet> variables,
                 Ref<const moriarty_internal::ValueSet> values);

  // ********************************************
  // ** See parent classes for more functions. **
  // ********************************************
};

// ReaderContext
//
// All context that MVariable<>::Read() has access to.
class ReaderContext : public moriarty_internal::NameContext,
                      public moriarty_internal::ViewOnlyContext,
                      public moriarty_internal::BasicIStreamContext {
 public:
  ReaderContext(std::string_view variable_name, Ref<InputCursor> input,
                Ref<const moriarty_internal::VariableSet> variables,
                Ref<const moriarty_internal::ValueSet> values);

  // ********************************************
  // ** See parent classes for more functions. **
  // ********************************************
};

// ResolverContext
//
// All context that MVariable<>::Generate() has access to.
class ResolverContext
    : public moriarty_internal::NameContext,
      public moriarty_internal::ViewOnlyContext,
      public moriarty_internal::MutableValuesContext,
      public moriarty_internal::ResolveValuesContext,
      public moriarty_internal::BasicRandomContext,
      public moriarty_internal::GenerationOrchestrationContext {
 public:
  ResolverContext(std::string_view variable_name,
                  Ref<const moriarty_internal::VariableSet> variables,
                  Ref<moriarty_internal::ValueSet> values,
                  Ref<moriarty_internal::RandomEngine> engine,
                  Ref<moriarty_internal::GenerationHandler> handler);

  // ForVariable()
  //
  // Creates a copy of this context, except the variable name is replaced with
  // `variable_name`.
  [[nodiscard]] ResolverContext ForVariable(std::string_view new_name) const;

  // ForSubVariable()
  //
  // Creates a copy of this context, except the variable name is replaced with
  // the name of a subvariable of the current one. (E.g., if the current
  // variable is A, then ForSubVariable("length") will set the new variable to
  // be "A.length")
  [[nodiscard]] ResolverContext ForSubVariable(std::string_view new_name) const;

  // ********************************************
  // ** See parent classes for more functions. **
  // ********************************************

 private:
  Ref<const moriarty_internal::VariableSet> variables_;
  Ref<moriarty_internal::ValueSet> values_;
  Ref<moriarty_internal::RandomEngine> engine_;
  Ref<moriarty_internal::GenerationHandler> handler_;
};

}  // namespace librarian
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_LIBRARIAN_CONTEXT_H_
