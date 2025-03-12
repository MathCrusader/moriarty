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

#include <stdexcept>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/testing/status_test_util.h"
#include "src/util/test_status_macro/status_testutil.h"

namespace moriarty {
namespace {

using ::moriarty_testing::IsUnsatisfiedConstraint;
using ::moriarty_testing::ThrowsValueNotFound;
using ::moriarty_testing::ThrowsVariableNotFound;
using ::testing::Not;

// -----------------------------------------------------------------------------
//  General Moriarty Error space checks

TEST(StatusTest, IsMoriartyErrorAcceptsAllErrorFunctions) {
  // EXPECT_TRUE(IsMoriartyError(NonRetryableGenerationError("")));
  // EXPECT_TRUE(IsMoriartyError(RetryableGenerationError("")));
  EXPECT_TRUE(IsMoriartyError(UnsatisfiedConstraintError("")));
}

TEST(StatusTest, IsMoriartyErrorDoesNotAcceptOkStatus) {
  EXPECT_FALSE(IsMoriartyError(absl::OkStatus()));
}

TEST(StatusTest, IsMoriartyErrorDoesNotAcceptGenericStatuses) {
  EXPECT_FALSE(IsMoriartyError(absl::AlreadyExistsError("")));
  EXPECT_FALSE(IsMoriartyError(absl::FailedPreconditionError("")));
  EXPECT_FALSE(IsMoriartyError(absl::InvalidArgumentError("")));
  EXPECT_FALSE(IsMoriartyError(absl::NotFoundError("")));
  EXPECT_FALSE(IsMoriartyError(absl::UnknownError("")));
}

TEST(StatusTest, MoriartyErrorsRecognizeAppropriateError) {
  // EXPECT_TRUE(IsNonRetryableGenerationError(NonRetryableGenerationError("")));
  // EXPECT_TRUE(IsRetryableGenerationError(RetryableGenerationError("")));
  EXPECT_TRUE(IsUnsatisfiedConstraintError(UnsatisfiedConstraintError("")));
}

TEST(StatusTest, MoriartyErrorsDoNotAcceptUnderlyingTypesAlone) {
  // All "" are irrelevant, just creating empty statuses with a specific code.
  // EXPECT_FALSE(IsNonRetryableGenerationError(
  //     absl::Status(NonRetryableGenerationError("").code(), "")));
  // EXPECT_FALSE(IsRetryableGenerationError(
  //     absl::Status(RetryableGenerationError("").code(), "")));
  EXPECT_FALSE(IsUnsatisfiedConstraintError(
      absl::Status(UnsatisfiedConstraintError("").code(), "")));
}

TEST(StatusTest, MoriartyErrorsDoNotAcceptOkStatus) {
  // EXPECT_FALSE(IsNonRetryableGenerationError(absl::OkStatus()));
  // EXPECT_FALSE(IsRetryableGenerationError(absl::OkStatus()));
  EXPECT_FALSE(IsUnsatisfiedConstraintError(absl::OkStatus()));
}

// -----------------------------------------------------------------------------
//  UnsatisfiedConstraintError

TEST(StatusTest, IsUnsatisfiedConstraintsGoogleTestMatcherWorks) {
  EXPECT_THAT(UnsatisfiedConstraintError("reason"),
              IsUnsatisfiedConstraint("reason"));
  EXPECT_THAT(absl::StatusOr<int>(UnsatisfiedConstraintError("reason")),
              IsUnsatisfiedConstraint("reason"));
  EXPECT_THAT(UnsatisfiedConstraintError("long long reason"),
              IsUnsatisfiedConstraint("reason"));

  EXPECT_THAT(UnsatisfiedConstraintError("some reason"),
              Not(IsUnsatisfiedConstraint("another reason")));
  EXPECT_THAT(absl::FailedPreconditionError("reason"),
              Not(IsUnsatisfiedConstraint("reason")));
}

TEST(StatusTest, FailedCheckConstraintsReturnsAnUnsatisfiedConstrainError) {
  EXPECT_TRUE(IsUnsatisfiedConstraintError(
      CheckConstraint(absl::FailedPreconditionError("message"), "reason")));
  EXPECT_TRUE(IsUnsatisfiedConstraintError(CheckConstraint(false, "reason")));
}

TEST(StatusTest, SuccessfulCheckConstraintsReturnsOk) {
  MORIARTY_EXPECT_OK(CheckConstraint(absl::OkStatus(), "reason"));
  MORIARTY_EXPECT_OK(CheckConstraint(true, "reason"));
}

TEST(ErrorsTest, ThrowsVariableNotFoundMatcherShouldWorkCorrectly) {
  {  // Happy path
    EXPECT_THAT([] { throw VariableNotFound("X"); },
                ThrowsVariableNotFound("X"));
  }
  {  // Wrong name
    EXPECT_THAT([] { throw VariableNotFound("X"); },
                Not(ThrowsVariableNotFound("other")));
    EXPECT_THAT([] { throw VariableNotFound("X"); },
                Not(ThrowsVariableNotFound("")));
  }
  {  // Wrong exception
    EXPECT_THAT([] { throw std::runtime_error("words"); },
                Not(ThrowsVariableNotFound("X")));
    EXPECT_THAT([] { throw std::runtime_error("X"); },
                Not(ThrowsVariableNotFound("X")));
  }
  {  // No exception
    EXPECT_THAT([] {}, Not(ThrowsVariableNotFound("X")));
  }
  {  // Multiple throws (::testing::ThrowsMessage gets these wrong)
    EXPECT_THAT(
        [] {
          throw std::runtime_error("words");
          throw VariableNotFound("X");
        },
        Not(ThrowsVariableNotFound("X")));
    EXPECT_THAT(
        [] {
          throw VariableNotFound("X");
          throw std::runtime_error("words");
        },
        ThrowsVariableNotFound("X"));
  }
}

TEST(ErrorsTest, ThrowsValueNotFoundMatcherShouldWorkCorrectly) {
  {  // Happy path
    EXPECT_THAT([] { throw ValueNotFound("X"); }, ThrowsValueNotFound("X"));
  }
  {  // Wrong name
    EXPECT_THAT([] { throw ValueNotFound("X"); },
                Not(ThrowsValueNotFound("other")));
    EXPECT_THAT([] { throw ValueNotFound("X"); }, Not(ThrowsValueNotFound("")));
  }
  {  // Wrong exception
    EXPECT_THAT([] { throw std::runtime_error("words"); },
                Not(ThrowsValueNotFound("X")));
    EXPECT_THAT([] { throw std::runtime_error("X"); },
                Not(ThrowsValueNotFound("X")));
  }
  {  // No exception
    EXPECT_THAT([] {}, Not(ThrowsValueNotFound("X")));
  }
  {  // Multiple throws (::testing::ThrowsMessage gets these wrong)
    EXPECT_THAT(
        [] {
          throw std::runtime_error("words");
          throw ValueNotFound("X");
        },
        Not(ThrowsValueNotFound("X")));
    EXPECT_THAT(
        [] {
          throw ValueNotFound("X");
          throw std::runtime_error("words");
        },
        ThrowsValueNotFound("X"));
  }
}

}  // namespace
}  // namespace moriarty
