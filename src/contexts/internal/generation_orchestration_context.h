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

#ifndef MORIARTY_SRC_CONTEXTS_INTERNAL_GENERATION_ORCHESTRATION_CONTEXT_H_
#define MORIARTY_SRC_CONTEXTS_INTERNAL_GENERATION_ORCHESTRATION_CONTEXT_H_

#include <string_view>

#include "src/internal/generation_handler.h"
#include "src/librarian/util/ref.h"

namespace moriarty {
namespace moriarty_internal {

// GenerationOrchestrationContext
//
// Orchestrates the entire generation process.
class GenerationOrchestrationContext {
 public:
  explicit GenerationOrchestrationContext(Ref<GenerationHandler> handler);

  // MarkStartGeneration()
  //
  // Informs the system that `variable_name` has started generation.
  void MarkStartGeneration(std::string_view variable_name);

  // MarkSuccessfulGeneration()
  //
  // Informs the system that `variable_name` has succeeded in its generation.
  // All variables that have started their generation since this one started
  // must have already finished as well.
  void MarkSuccessfulGeneration();

  // MarkAbandonedGeneration()
  //
  // Informs the system that `variable_name` has stopped attempting to generate
  // a value. All variables that have started their generation since this one
  // started must have already finished as well.
  void MarkAbandonedGeneration();

  // ReportGenerationFailure()
  //
  // Informs the system that `variable_name` has failed to generate a value.
  // Returns a recommendation for if the variable should retry generation or
  // abort generation.
  //
  // The list of variables to be deleted in the recommendation are those that
  // were generated since this variable started its generation. This class will
  // assume that the value for those variables have been deleted.
  //
  // All variables that have started their generation since this one
  // started must have already finished as well.
  [[nodiscard]] RetryRecommendation ReportGenerationFailure(
      std::string failure_reason);

  // GetFailureReason()
  //
  // Returns the most recent failure reason for `variable_name`.
  [[nodiscard]] std::optional<std::string> GetFailureReason(
      std::string variable_name) const;

 private:
  Ref<GenerationHandler> handler_;
};

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_INTERNAL_GENERATION_ORCHESTRATION_CONTEXT_H_
