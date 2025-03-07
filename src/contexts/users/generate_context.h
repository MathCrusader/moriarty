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

#ifndef MORIARTY_SRC_CONTEXTS_USER_GENERATE_CONTEXT_H_
#define MORIARTY_SRC_CONTEXTS_USER_GENERATE_CONTEXT_H_

#include "src/contexts/internal/basic_random_context.h"
#include "src/contexts/internal/variable_random_context.h"
#include "src/contexts/internal/view_only_context.h"
#include "src/internal/random_engine.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"

namespace moriarty {

// GenerateContext
//
// All context that Generators have access to.
class GenerateContext : public moriarty_internal::ViewOnlyContext,
                        public moriarty_internal::BasicRandomContext,
                        public moriarty_internal::VariableRandomContext {
 public:
  // Created by Moriarty and passed to you; no need to instantiate.
  // See `src/Moriarty.h` for entry points.
  GenerateContext(const moriarty_internal::VariableSet& variables,
                  const moriarty_internal::ValueSet& values,
                  moriarty_internal::RandomEngine& rng);

  // ********************************************
  // ** See parent classes for more functions. **
  // ********************************************
};

}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_USER_GENERATE_CONTEXT_H_
