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

#ifndef MORIARTY_CONTEXT_H_
#define MORIARTY_CONTEXT_H_

#include <iostream>
#include <optional>
#include <span>
#include <vector>

#include "src/contexts/internal/basic_istream_context.h"
#include "src/contexts/internal/basic_ostream_context.h"
#include "src/contexts/internal/basic_random_context.h"
#include "src/contexts/internal/name_context.h"
#include "src/contexts/internal/variable_istream_context.h"
#include "src/contexts/internal/variable_ostream_context.h"
#include "src/contexts/internal/variable_random_context.h"
#include "src/contexts/internal/view_only_context.h"
#include "src/internal/value_set.h"
#include "src/librarian/io_config.h"
#include "src/librarian/policies.h"
#include "src/librarian/util/ref.h"
#include "src/test_case.h"

namespace moriarty {

// -----------------------------------------------------------------------------
//  Generate

// GenerateContext
//
// All context that Generators have access to.
class GenerateContext : public moriarty_internal::ViewOnlyContext,
                        public moriarty_internal::BasicRandomContext,
                        public moriarty_internal::VariableRandomContext {
 public:
  // Created by Moriarty and passed to you; no need to instantiate.
  GenerateContext(Ref<const moriarty_internal::VariableSet> variables,
                  Ref<const moriarty_internal::ValueSet> values,
                  Ref<moriarty_internal::RandomEngine> rng);

  // *****************************************************
  // ** See parent classes for all available functions. **
  // *****************************************************
};

// The function signature for a generator.
using GenerateFn = std::function<std::vector<TestCase>(GenerateContext)>;

// Possible future additions:
//  - Make some generations non-fatal (aka, if they fail, it's okay)
//  - GenerateUntil (aka, we'll keep generating until g(x) is true)
struct GenerateOptions {
  // The descriptive name of this generator.
  std::string name;

  // How many times to call the generator.
  int num_calls = 1;

  // The seed to be passed to this generator. This will be combined with
  // Moriarty's general seed. If empty, a seed will be auto-generated.
  std::optional<std::string> seed;

  // Only auto-generate values for these variables (and any variables they
  // depend on). If empty, all variables will be generated.
  std::vector<std::string> variables_to_generate;
};

// -----------------------------------------------------------------------------
//  Import

// ImportContext
//
// All context that Importers have access to.
class ImportContext : public moriarty_internal::ViewOnlyContext,
                      public moriarty_internal::BasicIStreamContext,
                      public moriarty_internal::VariableIStreamContext {
 public:
  // Created by Moriarty and passed to you; no need to instantiate.
  ImportContext(Ref<const moriarty_internal::VariableSet> variables,
                Ref<InputCursor> input);

  // *****************************************************
  // ** See parent classes for all available functions. **
  // *****************************************************

 private:
  // TODO: In the future, these values will be the testset-wide values.
  moriarty_internal::ValueSet values_;
};

// The function signature for an importer.
using ImportFn = std::function<std::vector<ConcreteTestCase>(ImportContext)>;

// TODO: Auto-Validate?
struct ImportOptions {
  // The input stream to read from.
  Ref<std::istream> is = std::cin;

  // How strict the importer should be about whitespace.
  WhitespaceStrictness whitespace_strictness = WhitespaceStrictness::kPrecise;
};

// -----------------------------------------------------------------------------
//  Export

// ExportContext
//
// All context that Exporters have access to.
class ExportContext : public moriarty_internal::ViewOnlyContext,
                      public moriarty_internal::BasicOStreamContext,
                      public moriarty_internal::VariableOStreamContext {
 public:
  // Created by Moriarty and passed to you; no need to instantiate.
  ExportContext(Ref<std::ostream> os,
                Ref<const moriarty_internal::VariableSet> variables,
                Ref<const moriarty_internal::ValueSet> values);
  ExportContext(ExportContext ctx, Ref<std::ostream> os);

  // *****************************************************
  // ** See parent classes for all available functions. **
  // *****************************************************
};

// The function signature for an exporter.
using ExportFn =
    std::function<void(ExportContext, std::span<const ConcreteTestCase>)>;

struct ExportOptions {
  // The output stream to write to.
  Ref<std::ostream> os = std::cout;
};

// -----------------------------------------------------------------------------
//  Custom Constraints

// ConstraintContext
//
// All context that CustomConstraints have access to.
class ConstraintContext : public moriarty_internal::NameContext,
                          public moriarty_internal::ViewOnlyContext {
 public:
  ConstraintContext(std::string_view variable_name,
                    Ref<const moriarty_internal::VariableSet> variables,
                    Ref<const moriarty_internal::ValueSet> values);
  ConstraintContext(std::string_view name, const ViewOnlyContext& other);

  // ********************************************
  // ** See parent classes for more functions. **
  // ********************************************
};

}  // namespace moriarty

#endif  // MORIARTY_CONTEXT_H_
