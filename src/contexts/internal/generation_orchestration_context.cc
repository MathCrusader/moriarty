#include "src/contexts/internal/generation_orchestration_context.h"

#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>

#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace moriarty {
namespace moriarty_internal {

GenerationOrchestrationContext::GenerationOrchestrationContext(
    std::reference_wrapper<GenerationConfig> config)
    : config_(config) {}

void GenerationOrchestrationContext::MarkStartGeneration(
    std::string_view variable_name) {
  auto status = config_.get().MarkStartGeneration(variable_name);
  if (!status.ok()) throw std::runtime_error(std::string(status.message()));
}

void GenerationOrchestrationContext::MarkSuccessfulGeneration(
    std::string_view variable_name) {
  auto status = config_.get().MarkSuccessfulGeneration(variable_name);
  if (!status.ok()) throw std::runtime_error(std::string(status.message()));
}

void GenerationOrchestrationContext::MarkAbandonedGeneration(
    std::string_view variable_name) {
  auto status = config_.get().MarkAbandonedGeneration(variable_name);
  if (!status.ok()) throw std::runtime_error(std::string(status.message()));
}

GenerationConfig::RetryRecommendation
GenerationOrchestrationContext::AddGenerationFailure(
    std::string_view variable_name) {
  absl::Status status = absl::FailedPreconditionError("Fake error");
  auto recommendation =
      config_.get().AddGenerationFailure(variable_name, status);
  if (!recommendation.ok())
    throw std::runtime_error(std::string(recommendation.status().message()));
  return *recommendation;
}

}  // namespace moriarty_internal
}  // namespace moriarty
