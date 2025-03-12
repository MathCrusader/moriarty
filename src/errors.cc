// Copyright 2025 Darcy Best
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

#include "src/errors.h"

#include <optional>
#include <string_view>

#include "absl/status/status.h"
#include "absl/strings/cord.h"
#include "absl/strings/substitute.h"

namespace moriarty {

namespace {

// The error space name and payload strings are an implementation detail and may
// change at any point in time, so only including in `.cc`. Users should use
// `IsMoriartyError()` and equivalent functions to check if an `absl::Status` is
// from this error space, not check these strings directly.
//
// It is assumed that these contain no whitespace and are the first token in the
// payload string for the corresponding failure.
constexpr std::string_view kMoriartyErrorSpace = "moriarty";

// constexpr std::string_view kNonRetryableGenerationErrorPayload =
//     "NonRetryableGeneration";
// constexpr std::string_view kRetryableGenerationErrorPayload =
//     "RetryableGeneration";
constexpr std::string_view kUnsatisfiedConstraintErrorPayload =
    "UnsatisfiedConstraint";

absl::Status MakeMoriartyError(absl::StatusCode code, std::string_view message,
                               std::string_view payload) {
  absl::Status status = absl::Status(code, message);
  status.SetPayload(kMoriartyErrorSpace, absl::Cord(payload));
  return status;
}

}  // namespace

bool IsMoriartyError(const absl::Status& status) {
  return status.GetPayload(kMoriartyErrorSpace) != std::nullopt;
}

bool IsUnsatisfiedConstraintError(const absl::Status& status) {
  std::optional<absl::Cord> payload = status.GetPayload(kMoriartyErrorSpace);
  if (!payload.has_value()) return false;

  return *payload == kUnsatisfiedConstraintErrorPayload;
}

absl::Status UnsatisfiedConstraintError(
    std::string_view constraint_explanation) {
  return MakeMoriartyError(absl::StatusCode::kFailedPrecondition,
                           constraint_explanation,
                           kUnsatisfiedConstraintErrorPayload);
}

absl::Status CheckConstraint(const absl::Status& constraint,
                             std::string_view constraint_explanation) {
  if (constraint.ok()) return absl::OkStatus();
  return UnsatisfiedConstraintError(
      absl::Substitute("$0; $1", constraint_explanation, constraint.message()));
}

absl::Status CheckConstraint(bool constraint,
                             std::string_view constraint_explanation) {
  if (constraint) return absl::OkStatus();
  return UnsatisfiedConstraintError(constraint_explanation);
}

}  // namespace moriarty
