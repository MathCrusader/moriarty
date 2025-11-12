#include "src/contexts/internal/generation_orchestration_context.h"

#include <optional>
#include <string>
#include <string_view>

#include "src/librarian/util/ref.h"

namespace moriarty {
namespace moriarty_internal {

GenerationOrchestrationContext::GenerationOrchestrationContext(
    Ref<GenerationHandler> handler)
    : handler_(handler) {}

void GenerationOrchestrationContext::MarkStartGeneration(
    std::string_view variable_name) {
  handler_.get().Start(variable_name);
}

void GenerationOrchestrationContext::MarkSuccessfulGeneration() {
  handler_.get().Complete();
}

void GenerationOrchestrationContext::MarkAbandonedGeneration() {
  handler_.get().Abandon();
}

RetryRecommendation GenerationOrchestrationContext::ReportGenerationFailure(
    std::string failure_reason) {
  return handler_.get().ReportFailure(failure_reason);
}

std::optional<std::string> GenerationOrchestrationContext::GetFailureReason(
    std::string variable_name) const {
  return handler_.get().GetFailureReason(variable_name);
}

}  // namespace moriarty_internal
}  // namespace moriarty
