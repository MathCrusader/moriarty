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

#ifndef MORIARTY_CONTEXTS_LIBRARIAN_CONTEXT_H_
#define MORIARTY_CONTEXTS_LIBRARIAN_CONTEXT_H_

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

// AnalyzeVariableContext
//
// Allows you to inspect the current state of the variables and values.
// AnalyzeVariableContext is read-only. It does not allow you to modify the
// variables or values.
//
// Note: All Librarian contexts can be converted to this type.
class AnalyzeVariableContext : public moriarty_internal::NameContext,
                               public moriarty_internal::ViewOnlyContext {
 public:
  AnalyzeVariableContext(std::string_view variable_name,
                         Ref<const moriarty_internal::VariableSet> variables,
                         Ref<const moriarty_internal::ValueSet> values);
  AnalyzeVariableContext(const AnalyzeVariableContext& other,
                         NameContext new_name);
  template <typename T>
  AnalyzeVariableContext(const T& other)
      : NameContext(other), ViewOnlyContext(other) {}

  [[nodiscard]] AnalyzeVariableContext ForSubVariable(
      std::string_view sub_variable_name) const;

  [[nodiscard]] AnalyzeVariableContext ForIndexedSubVariable(
      std::function<std::string(int)> indexed_name_fn, int index) const;

  // ********************************************
  // ** See parent classes for more functions. **
  // ********************************************
};

// AssignVariableContext
//
// Allows you to inspect the current state of the variables and set values.
class AssignVariableContext : public moriarty_internal::NameContext,
                              public moriarty_internal::ViewOnlyContext,
                              public moriarty_internal::MutableValuesContext {
 public:
  AssignVariableContext(std::string_view variable_name,
                        Ref<const moriarty_internal::VariableSet> variables,
                        Ref<moriarty_internal::ValueSet> values);
  AssignVariableContext(const AssignVariableContext& other,
                        NameContext new_name);

  [[nodiscard]] AssignVariableContext ForSubVariable(
      std::string_view sub_variable_name) const;

  [[nodiscard]] AssignVariableContext ForIndexedSubVariable(
      std::function<std::string(int)> indexed_name_fn, int index) const;

  // ********************************************
  // ** See parent classes for more functions. **
  // ********************************************
};

// WriteVariableContext
//
// All context that MVariable<>::Write() has access to.
class WriteVariableContext : public moriarty_internal::NameContext,
                             public moriarty_internal::ViewOnlyContext,
                             public moriarty_internal::BasicOStreamContext {
 public:
  WriteVariableContext(std::string_view variable_name, Ref<std::ostream> os,
                       Ref<const moriarty_internal::VariableSet> variables,
                       Ref<const moriarty_internal::ValueSet> values);
  WriteVariableContext(const WriteVariableContext& other, NameContext new_name);

  [[nodiscard]] WriteVariableContext ForSubVariable(
      std::string_view sub_variable_name) const;

  [[nodiscard]] WriteVariableContext ForIndexedSubVariable(
      std::function<std::string(int)> indexed_name_fn, int index) const;

  // ********************************************
  // ** See parent classes for more functions. **
  // ********************************************
};

// ReadVariableContext
//
// All context that MVariable<>::Read() has access to.
class ReadVariableContext : public moriarty_internal::NameContext,
                            public moriarty_internal::ViewOnlyContext,
                            public moriarty_internal::BasicIStreamContext {
 public:
  ReadVariableContext(std::string_view variable_name, Ref<InputCursor> input,
                      Ref<const moriarty_internal::VariableSet> variables,
                      Ref<const moriarty_internal::ValueSet> values);
  ReadVariableContext(const ReadVariableContext& other, NameContext new_name);

  [[nodiscard]] ReadVariableContext ForSubVariable(
      std::string_view sub_variable_name) const;

  [[nodiscard]] ReadVariableContext ForIndexedSubVariable(
      std::function<std::string(int)> indexed_name_fn, int index) const;

  // ********************************************
  // ** See parent classes for more functions. **
  // ********************************************
};

// GenerateVariableContext
//
// All context that MVariable<>::Generate() has access to.
class GenerateVariableContext
    : public moriarty_internal::NameContext,
      public moriarty_internal::ViewOnlyContext,
      public moriarty_internal::MutableValuesContext,
      public moriarty_internal::ResolveValuesContext,
      public moriarty_internal::BasicRandomContext,
      public moriarty_internal::GenerationOrchestrationContext {
 public:
  GenerateVariableContext(std::string_view variable_name,
                          Ref<const moriarty_internal::VariableSet> variables,
                          Ref<moriarty_internal::ValueSet> values,
                          Ref<moriarty_internal::RandomEngine> engine,
                          Ref<moriarty_internal::GenerationHandler> handler);
  GenerateVariableContext(const GenerateVariableContext& other,
                          NameContext new_name);

  [[nodiscard]] GenerateVariableContext ForSubVariable(
      std::string_view sub_variable_name) const;

  [[nodiscard]] GenerateVariableContext ForIndexedSubVariable(
      std::function<std::string(int)> indexed_name_fn, int index) const;

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

#endif  // MORIARTY_CONTEXTS_LIBRARIAN_CONTEXT_H_
