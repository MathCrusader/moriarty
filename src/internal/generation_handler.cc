// Copyright 2024 Google LLC
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

#include "src/internal/generation_handler.h"

#include <format>
#include <optional>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "src/librarian/errors.h"
#include "src/librarian/policies.h"

namespace moriarty {
namespace moriarty_internal {

GenerationHandler::GenerationHandler(int64_t max_active_retries,
                                     int64_t max_total_retries,
                                     int64_t max_total_generate_calls)
    : max_active_retries_(max_active_retries),
      max_total_retries_(max_total_retries),
      max_total_generate_calls_(max_total_generate_calls) {}

void GenerationHandler::Start(std::string_view variable_name) {
  auto it = generation_info_index_.find(variable_name);
  if (it == generation_info_index_.end()) {
    it = generation_info_index_
             .emplace(std::string(variable_name), generation_info_.size())
             .first;
    generation_info_.push_back({.name = std::string(variable_name)});
  }
  int64_t idx = it->second;

  GenerationInfo& info = generation_info_[idx];
  if (info.variable_count_at_start.has_value()) {
    throw std::runtime_error(
        std::format("Cycle found in generation of {}", variable_name));
  }

  info.active_retry_count = 0;
  info.variable_count_at_start = generated_variables_.size();

  variables_actively_being_generated_.push(idx);
}

void GenerationHandler::Complete() {
  if (variables_actively_being_generated_.empty()) {
    throw std::runtime_error(
        "Attempting to complete generation, when none have been started.");
  }

  int idx = variables_actively_being_generated_.top();
  variables_actively_being_generated_.pop();

  generated_variables_.push_back(idx);
  generation_info_[idx].active_retry_count = 0;
  generation_info_[idx].variable_count_at_start = std::nullopt;
  total_generate_calls_++;
}

void GenerationHandler::Abandon() {
  if (variables_actively_being_generated_.empty()) {
    throw std::runtime_error(
        "Attempting to abandon generation, when none have been started.");
  }

  int idx = variables_actively_being_generated_.top();
  variables_actively_being_generated_.pop();
  generation_info_[idx].active_retry_count = 0;
  generation_info_[idx].variable_count_at_start = std::nullopt;
}

RetryRecommendation GenerationHandler::ReportFailure(
    std::string failure_reason) {
  if (variables_actively_being_generated_.empty()) {
    throw std::runtime_error(
        "Attempting to report a failure in generation, when none have been "
        "started.");
  }

  int idx = variables_actively_being_generated_.top();
  GenerationInfo& info = generation_info_[idx];
  info.active_retry_count++;
  info.total_retry_count++;
  info.most_recent_failure = std::move(failure_reason);

  total_generate_calls_++;

  std::vector<std::string> variables_to_delete;
  for (int64_t i = *info.variable_count_at_start;
       i < generated_variables_.size(); ++i) {
    variables_to_delete.push_back(
        generation_info_[generated_variables_[i]].name);
  }
  generated_variables_.resize(*info.variable_count_at_start);

  auto should_retry = (info.active_retry_count > max_active_retries_ ||
                       info.total_retry_count > max_total_retries_ ||
                       total_generate_calls_ > max_total_generate_calls_)
                          ? RetryPolicy::kAbort
                          : RetryPolicy::kRetry;

  return {should_retry, variables_to_delete};
}

std::optional<std::string> GenerationHandler::GetFailureReason(
    std::string_view variable_name) const {
  auto it = generation_info_index_.find(variable_name);
  if (it == generation_info_index_.end()) throw VariableNotFound(variable_name);

  return generation_info_[it->second].most_recent_failure;
}

}  // namespace moriarty_internal
}  // namespace moriarty
