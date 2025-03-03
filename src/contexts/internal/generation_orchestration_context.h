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

#include <functional>
#include <optional>
#include <stdexcept>
#include <string_view>

#include "src/internal/generation_config.h"

namespace moriarty {
namespace moriarty_internal {

// GenerationOrchestrationContext
//
// Orchestrates the entire generation process.
class GenerationOrchestrationContext {
 public:
  explicit GenerationOrchestrationContext(GenerationConfig& config)
      : config_(config) {}

  void MarkStartGeneration(std::string_view variable_name) {
    auto status = config_.get().MarkStartGeneration(variable_name);
    if (!status.ok()) throw std::runtime_error(std::string(status.message()));
  }

  void MarkSuccessfulGeneration(std::string_view variable_name) {
    auto status = config_.get().MarkSuccessfulGeneration(variable_name);
    if (!status.ok()) throw std::runtime_error(std::string(status.message()));
  }

  void MarkAbandonedGeneration(std::string_view variable_name) {
    auto status = config_.get().MarkAbandonedGeneration(variable_name);
    if (!status.ok()) throw std::runtime_error(std::string(status.message()));
  }

  [[nodiscard]] GenerationConfig::RetryRecommendation AddGenerationFailure(
      std::string_view variable_name) {
    absl::Status status = absl::FailedPreconditionError("Fake error");
    auto recommendation =
        config_.get().AddGenerationFailure(variable_name, status);
    if (!recommendation.ok())
      throw std::runtime_error(std::string(recommendation.status().message()));
    return *recommendation;
  }

  std::optional<int64_t> GetSoftGenerationLimit() const {
    return config_.get().GetSoftGenerationLimit();
  }

 private:
  std::reference_wrapper<GenerationConfig> config_;
};

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_INTERNAL_GENERATION_ORCHESTRATION_CONTEXT_H_
