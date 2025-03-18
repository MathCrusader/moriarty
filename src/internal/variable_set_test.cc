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

#include "src/internal/variable_set.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/librarian/test_utils.h"
#include "src/testing/mtest_type.h"
#include "src/testing/mtest_type2.h"
#include "src/testing/status_test_util.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/constraints/base_constraints.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

using ::moriarty::IsOkAndHolds;
using ::moriarty_testing::Generate;
using ::moriarty_testing::MTestType;
using ::moriarty_testing::MTestType2;
using ::moriarty_testing::TestType;
using ::moriarty_testing::ThrowsVariableNotFound;

TEST(VariableSetTest, GetVariableCanRetrieveFromAddVariable) {
  VariableSet v;
  MORIARTY_ASSERT_OK(
      v.AddVariable("A", MTestType().AddConstraint(Exactly<TestType>(111111))));
  MORIARTY_ASSERT_OK(
      v.AddVariable("B", MTestType().AddConstraint(Exactly<TestType>(222222))));

  MTestType A = v.GetVariable<MTestType>("A");
  EXPECT_THAT(Generate(A), IsOkAndHolds(111111));

  MTestType B = v.GetVariable<MTestType>("B");
  EXPECT_THAT(Generate(B), IsOkAndHolds(222222));
}

TEST(VariableSetTest, AddOrMergeVariableSetsProperlyWhenNotMerging) {
  VariableSet v;
  v.AddOrMergeVariable("A",
                       MTestType().AddConstraint(Exactly<TestType>(111111)));
  v.AddOrMergeVariable("B",
                       MTestType().AddConstraint(Exactly<TestType>(222222)));

  MTestType A = v.GetVariable<MTestType>("A");
  EXPECT_THAT(Generate(A), IsOkAndHolds(111111));

  MTestType B = v.GetVariable<MTestType>("B");
  EXPECT_THAT(Generate(B), IsOkAndHolds(222222));
}

TEST(VariableSetTest, AddOrMergeVariableSetsProperlyWhenMerging) {
  VariableSet v;
  v.AddOrMergeVariable("A",
                       MTestType().AddConstraint(OneOf<TestType>({11, 22})));
  v.AddOrMergeVariable("A",
                       MTestType().AddConstraint(OneOf<TestType>({22, 33})));

  MTestType A = v.GetVariable<MTestType>("A");
  EXPECT_THAT(Generate(A), IsOkAndHolds(22));
}

TEST(VariableSetTest, GetVariableOnNonExistentVariableFails) {
  VariableSet v;

  EXPECT_THAT([&] { v.GetAbstractVariable("unknown1"); },
              ThrowsVariableNotFound("unknown1"));
  EXPECT_THAT([&] { v.GetVariable<MTestType>("unknown2"); },
              ThrowsVariableNotFound("unknown2"));
}

TEST(VariableSetTest, GetAnonymousVariableShouldWork) {
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MTestType()));
  MORIARTY_ASSERT_OK(variables.AddVariable("B", MTestType()));
  MORIARTY_ASSERT_OK(variables.AddVariable("C", MTestType2()));
  MORIARTY_ASSERT_OK(variables.AddVariable("D", MTestType2()));

  variables.GetAbstractVariable("A");  // No crash = good.
  EXPECT_THAT([&] { variables.GetAbstractVariable("X"); },
              ThrowsVariableNotFound("X"));
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
