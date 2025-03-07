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

#ifndef MORIARTY_SRC_IMPORT_EXPORT_H_
#define MORIARTY_SRC_IMPORT_EXPORT_H_

#include <functional>
#include <iostream>
#include <optional>
#include <ostream>
#include <span>
#include <vector>

#include "src/contexts/users/export_context.h"
#include "src/contexts/users/generate_context.h"
#include "src/contexts/users/import_context.h"
#include "src/io_config.h"
#include "test_case.h"

namespace moriarty {

// -----------------------------------------------------------------------------
//  Generate

using GenerateFn = std::function<std::vector<TestCase>(GenerateContext)>;

// Possible future additions:
//  - Make some generations non-fatal (aka, if they fail, it's okay)
//  - Soft generation limit
//  - GenerateUntil (aka, we'll keep generating until g(x) is true)
struct GenerateOptions {
  // The descriptive name of this generator.
  std::string name;

  // How many times to call the generator.
  int num_calls = 1;

  // The seed to be passed to this generator. This will be combined with
  // Moriarty's general seed. If empty, a seed will be auto-generated.
  std::optional<std::string> seed;
};

// -----------------------------------------------------------------------------
//  Import

using ImportFn = std::function<std::vector<ConcreteTestCase>(ImportContext)>;

// TODO: Auto-Validate?
struct ImportOptions {
  // The input stream to read from.
  std::istream& is = std::cin;

  // How strict the importer should be about whitespace.
  WhitespaceStrictness whitespace_strictness = WhitespaceStrictness::kPrecise;
};

// -----------------------------------------------------------------------------
//  Export

using ExportFn =
    std::function<void(ExportContext, std::span<const ConcreteTestCase>)>;

struct ExportOptions {
  // The output stream to write to.
  std::ostream& os = std::cout;
};

}  // namespace moriarty

#endif  // MORIARTY_SRC_IMPORT_EXPORT_H_
