/*
 * Copyright 2024 Google LLC
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

#ifndef MORIARTY_SRC_INTERNAL_GENERATION_HANDLER_H_
#define MORIARTY_SRC_INTERNAL_GENERATION_HANDLER_H_

#include <cstdint>
#include <optional>
#include <stack>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// FIXME: Should this fail if you Start() a variable that was Complete()d?

namespace moriarty {
namespace moriarty_internal {

// RetryRecommendation
//
// On a failed generation attempt, this recommends if you should retry to not.
// Also, provides information about values of variables that should be
// deleted.
struct RetryRecommendation {
  enum Policy {
    kAbort,  // Do not continue retrying
    kRetry   // Retry generation
  };
  Policy policy;
  std::vector<std::string> variable_names_to_delete;
};

// GenerationHandler()
//
// Maintains the list of variables that are being generated. As you
// generate variables, it must be in a stack-based order (the stack comes from
// dependent variables and subvariables).
class GenerationHandler {
 public:
  constexpr static int64_t kDefaultMaxActiveRetries = 1000;
  constexpr static int64_t kDefaultMaxTotalRetries = 100'000;
  constexpr static int64_t kDefaultMaxTotalGenerateCalls = 10'000'000;

  explicit GenerationHandler(
      int64_t max_active_retries_per_variable = kDefaultMaxActiveRetries,
      int64_t max_total_retries_per_variable = kDefaultMaxTotalRetries,
      int64_t max_total_generate_calls_overall = kDefaultMaxTotalGenerateCalls);

  // Start()
  //
  // Starts the generation of `variable_name`. This variable becomes the active
  // variable (until another variable calls Start() or this variable calls
  // Complete() or Abandon()) in a stack-like fashion.
  //
  // Throws if this variable has already been started, but hasn't been completed
  // or abandoned. (Likely indicating a cyclic dependency.)
  void Start(std::string_view variable_name);

  // Complete()
  //
  // Completes the generation of the current variable (successfully).
  //
  // The variable that was  active before this variable's Start() becomes active
  // again. (Think: stack order)
  void Complete();

  // Abandon()
  //
  // Abandons the generation of the active variable.
  //
  // The variable that was  active before this variable's Start() becomes active
  // again. (Think: stack order)
  void Abandon();

  // ReportFailure()
  //
  // Informs that the active variable has failed to generate a value.
  // This function returns a recommendation for if the variable should retry
  // generation or abort generation.
  //
  // The list of variables to be deleted in the recommendation are those that
  // were generated since this variable started its generation. The caller
  // should delete those variables.
  //
  // The active variable is not updated.
  [[nodiscard]] RetryRecommendation ReportFailure(std::string failure_reason);

  // GetFailureReason()
  //
  // Returns the most recent failure reason for `variable_name`, if there is
  // one.
  [[nodiscard]] std::optional<std::string> GetFailureReason(
      std::string_view variable_name) const;

 private:
  int64_t max_active_retries_;
  int64_t max_total_retries_;
  int64_t max_total_generate_calls_;
  int64_t total_generate_calls_ = 0;

  // Retry counts.
  //  * "Active" is set to 0 whenever Complete() or Abandon() is called.
  //  * "Total" is never reset.
  struct GenerationInfo {
    std::string name;
    int64_t total_retry_count = 0;
    int64_t active_retry_count = 0;
    std::optional<std::string> most_recent_failure;
    std::optional<int64_t> variable_count_at_start;
  };
  std::vector<GenerationInfo> generation_info_;

  // This allows std::string_view to be used as a key in the map on lookups.
  struct Hash {
    using is_transparent = void;
    [[nodiscard]] size_t operator()(const std::string& txt) const {
      return std::hash<std::string>{}(txt);
    }
    [[nodiscard]] size_t operator()(std::string_view txt) const {
      return std::hash<std::string_view>{}(txt);
    }
  };

  // These variables represent the index in `generation_info_`.

  // variables that were generated, in the order they were generated.
  std::unordered_map<std::string, int64_t, Hash, std::equal_to<>>
      generation_info_index_;
  std::vector<int64_t> generated_variables_;
  std::stack<int64_t> variables_actively_being_generated_;
};

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_GENERATION_HANDLER_H_
