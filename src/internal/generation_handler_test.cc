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

#include "src/internal/generation_handler.h"

#include <optional>
#include <stdexcept>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/testing/gtest_helpers.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

using RetryRecommendation::kAbort;
using RetryRecommendation::kRetry;

using moriarty_testing::ThrowsVariableNotFound;
using testing::ElementsAre;
using testing::Eq;
using testing::FieldsAre;
using testing::HasSubstr;
using testing::IsEmpty;
using testing::Optional;
using testing::ThrowsMessage;
using testing::UnorderedElementsAre;

TEST(GenerationHandlerTest, ActiveRetriesShouldRecommendToRetry) {
  GenerationHandler h(2);
  h.Start("x");
  EXPECT_THAT(h.ReportFailure("fail1"), FieldsAre(kRetry, IsEmpty()));
  EXPECT_THAT(h.ReportFailure("fail2"), FieldsAre(kRetry, IsEmpty()));
  EXPECT_THAT(h.ReportFailure("fail3"), FieldsAre(kAbort, IsEmpty()));
  EXPECT_THAT(h.ReportFailure("fail4"), FieldsAre(kAbort, IsEmpty()));
}

TEST(GenerationHandlerTest, ActiveRetriesResetsAfterFinishing) {
  {  // Via Complete()
    GenerationHandler h(2);
    h.Start("x");
    EXPECT_THAT(h.ReportFailure("1_1"), FieldsAre(kRetry, IsEmpty()));
    h.Complete();
    h.Start("x");
    EXPECT_THAT(h.ReportFailure("1_2"), FieldsAre(kRetry, IsEmpty()));
    EXPECT_THAT(h.ReportFailure("2_3"), FieldsAre(kRetry, IsEmpty()));
    EXPECT_THAT(h.ReportFailure("3_4"), FieldsAre(kAbort, IsEmpty()));
    EXPECT_THAT(h.ReportFailure("4_5"), FieldsAre(kAbort, IsEmpty()));
  }
  {  // Via Abandon()
    GenerationHandler h(2);
    h.Start("x");
    EXPECT_THAT(h.ReportFailure("1_1"), FieldsAre(kRetry, IsEmpty()));
    h.Abandon();
    h.Start("x");
    EXPECT_THAT(h.ReportFailure("1_2"), FieldsAre(kRetry, IsEmpty()));
    EXPECT_THAT(h.ReportFailure("2_3"), FieldsAre(kRetry, IsEmpty()));
    EXPECT_THAT(h.ReportFailure("3_4"), FieldsAre(kAbort, IsEmpty()));
    EXPECT_THAT(h.ReportFailure("4_5"), FieldsAre(kAbort, IsEmpty()));
  }
}

TEST(GenerationHandlerTest, CyclesShouldBeDetected) {
  {  // Direct cycle
    GenerationHandler h;
    h.Start("x");
    EXPECT_THAT([&] { h.Start("x"); },
                ThrowsMessage<std::runtime_error>(HasSubstr("ycl")));  // cycle
  }
  {  // Indirect cycle
    GenerationHandler h;
    h.Start("x");
    h.Start("y");
    EXPECT_THAT([&] { h.Start("x"); },
                ThrowsMessage<std::runtime_error>(HasSubstr("ycl")));  // cycle
  }
}

TEST(GenerationHandlerTest, GenerationAttemptForMultipleVariablesShouldWork) {
  {  // Via Complete()
    GenerationHandler h(2);
    h.Start("x");
    EXPECT_THAT(h.ReportFailure("x1"), FieldsAre(kRetry, IsEmpty()));
    h.Start("y");
    EXPECT_THAT(h.ReportFailure("y1"), FieldsAre(kRetry, IsEmpty()));
    h.Complete();
    EXPECT_THAT(h.ReportFailure("x2"), FieldsAre(kRetry, ElementsAre("y")));
    EXPECT_THAT(h.ReportFailure("x3"), FieldsAre(kAbort, IsEmpty()));
  }
  {  // Via Abandon()
    GenerationHandler h(2);
    h.Start("x");
    EXPECT_THAT(h.ReportFailure("x1"), FieldsAre(kRetry, IsEmpty()));
    h.Start("y");
    EXPECT_THAT(h.ReportFailure("y1"), FieldsAre(kRetry, IsEmpty()));
    h.Abandon();
    EXPECT_THAT(h.ReportFailure("x2"), FieldsAre(kRetry, IsEmpty()));
    EXPECT_THAT(h.ReportFailure("x3"), FieldsAre(kAbort, IsEmpty()));
  }
}

TEST(GenerationHandlerTest, VariablesCanBeReAddedToTheStackAfterRemoval) {
  {  // After Complete()
    GenerationHandler h;
    h.Start("x");
    h.Complete();
    h.Start("x");
  }
  {  // After Abandon()
    GenerationHandler h;
    h.Start("x");
    h.Abandon();
    h.Start("x");
  }
}

TEST(GenerationHandlerTest, TotalRetriesShouldStopGeneration) {
  GenerationHandler h(2, 3);
  h.Start("x");
  EXPECT_THAT(h.ReportFailure("1"), FieldsAre(kRetry, IsEmpty()));
  EXPECT_THAT(h.ReportFailure("2"), FieldsAre(kRetry, IsEmpty()));
  h.Abandon();
  h.Start("x");
  EXPECT_THAT(h.ReportFailure("3"), FieldsAre(kRetry, IsEmpty()));
  EXPECT_THAT(h.ReportFailure("4"), FieldsAre(kAbort, IsEmpty()));
  h.Abandon();
  h.Start("x");
  EXPECT_THAT(h.ReportFailure("5"), FieldsAre(kAbort, IsEmpty()));

  h.Start("y");  // y can still retry
  EXPECT_THAT(h.ReportFailure("1"), FieldsAre(kRetry, IsEmpty()));
}

TEST(GenerationHandlerTest, TotalGenerationCallsShouldStopGeneration) {
  GenerationHandler h(2, 3, 4);
  h.Start("x");
  EXPECT_THAT(h.ReportFailure("1"), FieldsAre(kRetry, IsEmpty()));
  EXPECT_THAT(h.ReportFailure("2"), FieldsAre(kRetry, IsEmpty()));
  h.Abandon();
  h.Start("x");
  EXPECT_THAT(h.ReportFailure("3"), FieldsAre(kRetry, IsEmpty()));
  EXPECT_THAT(h.ReportFailure("4"), FieldsAre(kAbort, IsEmpty()));
  h.Abandon();
  h.Start("x");
  EXPECT_THAT(h.ReportFailure("5"), FieldsAre(kAbort, IsEmpty()));

  h.Start("y");  // y can't retry either
  EXPECT_THAT(h.ReportFailure("1"), FieldsAre(kAbort, IsEmpty()));
}

TEST(GenerationHandlerTest, GetFailureReasonShouldWork) {
  {  // Not found
    GenerationHandler h;
    EXPECT_THAT([&] { (void)h.GetFailureReason("x"); },
                ThrowsVariableNotFound("x"));
  }
  {  // Before adding message
    GenerationHandler h;
    h.Start("x");
    EXPECT_EQ(h.GetFailureReason("x"), std::nullopt);
  }
  {  // After adding message
    GenerationHandler h;
    h.Start("x");
    EXPECT_THAT(h.ReportFailure("x fail"), FieldsAre(kRetry, IsEmpty()));
    EXPECT_THAT(h.GetFailureReason("x"), Optional(Eq("x fail")));
  }
  {  // After Complete()
    GenerationHandler h;
    h.Start("x");
    EXPECT_THAT(h.ReportFailure("x fail"), FieldsAre(kRetry, IsEmpty()));
    h.Complete();
    EXPECT_THAT(h.GetFailureReason("x"), Optional(Eq("x fail")));
  }
  {  // After Abandon()
    GenerationHandler h;
    h.Start("x");
    EXPECT_THAT(h.ReportFailure("x fail"), FieldsAre(kRetry, IsEmpty()));
    h.Abandon();
    EXPECT_THAT(h.GetFailureReason("x"), Optional(Eq("x fail")));
  }
}

TEST(GenerationHandlerTest, CompleteOrAbandonWithoutStartShouldFail) {
  {  // Immediate Abandon
    GenerationHandler h;
    EXPECT_THAT([&] { h.Abandon(); },
                ThrowsMessage<std::runtime_error>(HasSubstr("started")));
  }
  {  // Immediate Complete
    GenerationHandler h;
    EXPECT_THAT([&] { h.Complete(); },
                ThrowsMessage<std::runtime_error>(HasSubstr("started")));
  }
  {  // Start, complete, complete
    GenerationHandler h;
    h.Start("x");
    h.Complete();
    EXPECT_THAT([&] { h.Complete(); },
                ThrowsMessage<std::runtime_error>(HasSubstr("started")));
  }
  {  // Start, abandon, abandon
    GenerationHandler h;
    h.Start("x");
    h.Abandon();
    EXPECT_THAT([&] { h.Abandon(); },
                ThrowsMessage<std::runtime_error>(HasSubstr("started")));
  }
  {  // Start, abandon, complete
    GenerationHandler h;
    h.Start("x");
    h.Abandon();
    EXPECT_THAT([&] { h.Complete(); },
                ThrowsMessage<std::runtime_error>(HasSubstr("started")));
  }
}

TEST(GenerationHandlerTest, GenerationAttemptsWithoutCallingStartFails) {
  GenerationHandler h;
  EXPECT_THAT(
      [&] { (void)h.ReportFailure("x"); },
      ThrowsMessage<std::runtime_error>(HasSubstr(
          "Attempting to report a failure in generation, when none have "
          "been started.")));
}

TEST(GenerationHandlerTest,
     VariablesToDeleteAreOnesGeneratedBetweenBeginAndEnd) {
  GenerationHandler h;
  h.Start("x");
  h.Start("y");
  h.Start("z");
  h.Complete();
  h.Complete();
  EXPECT_THAT(h.ReportFailure("xfail"),
              FieldsAre(kRetry, UnorderedElementsAre("y", "z")));
}

TEST(GenerationHandlerTest,
     VariablesToDeleteShouldNotDeleteUnrelatedVariables) {
  GenerationHandler h;
  h.Start("w");
  h.Complete();

  h.Start("x");
  h.Start("y");
  h.Start("z");
  h.Complete();
  h.Complete();
  EXPECT_THAT(h.ReportFailure("xfail"),
              FieldsAre(kRetry, UnorderedElementsAre("y", "z")));
  h.Start("p");
  h.Start("q");
  h.Complete();
  h.Complete();
  EXPECT_THAT(h.ReportFailure("xfail"),
              FieldsAre(kRetry, UnorderedElementsAre("p", "q")));
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
