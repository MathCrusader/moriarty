/*
 * Copyright 2025 Darcy Best
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

#include "src/variables/constraints/container_constraints.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/librarian/test_utils.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/constraints/numeric_constraints.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace {

using ::moriarty_testing::Context;
using ::moriarty_testing::GeneratedValuesAre;
using ::moriarty_testing::GenerateLots;
using ::testing::AllOf;
using ::testing::Each;
using ::testing::Ge;
using ::testing::HasSubstr;
using ::testing::Le;

TEST(ContainerConstraintsTest, LengthConstraintsAreCorrect) {
  EXPECT_THAT(Length(10).GetConstraints(), GeneratedValuesAre(10));

  // TODO: This are really "IsOkAndHolds" tests, but the matcher is not working
  auto val1 = GenerateLots(Length("2 * N").GetConstraints(),
                           Context().WithValue<MInteger>("N", 7));
  MORIARTY_EXPECT_OK(val1);
  EXPECT_THAT(*val1, Each(14));

  auto val2 = GenerateLots(Length(AtLeast("X"), AtMost(15)).GetConstraints(),
                           Context().WithValue<MInteger>("X", 3));
  MORIARTY_EXPECT_OK(val2);
  EXPECT_THAT(*val2, Each(AllOf(Ge(3), Le(15))));
}

TEST(ContainerConstraintsTest, ElementsConstraintsAreCorrect) {
  EXPECT_THAT(Elements<MInteger>(Between(1, 10)).GetConstraints(),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
  auto val = GenerateLots(
      Elements<MInteger>(AtLeast("X"), AtMost(15)).GetConstraints(),
      Context().WithValue<MInteger>("X", 3));
  MORIARTY_EXPECT_OK(val);
  EXPECT_THAT(*val, Each(AllOf(Ge(3), Le(15))));
}

TEST(ContainerConstraintsTest, LengthToStringWorks) {
  EXPECT_THAT(Length(Between(1, 10)).ToString(),
              AllOf(HasSubstr("Length"), HasSubstr("1, 10")));
}

}  // namespace
}  // namespace moriarty
