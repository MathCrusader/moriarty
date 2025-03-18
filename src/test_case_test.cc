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

#include "src/test_case.h"

#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include "absl/status/statusor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/internal/generation_bootstrap.h"
#include "src/internal/random_engine.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/testing/mtest_type.h"
#include "src/testing/status_test_util.h"
#include "src/util/status_macro/status_macros.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace {

using ::moriarty::IsOkAndHolds;
using ::moriarty::moriarty_internal::GenerateAllValues;
using ::moriarty::moriarty_internal::RandomEngine;
using ::moriarty::moriarty_internal::ValueSet;
using ::moriarty_testing::MTestType;
using ::moriarty_testing::TestType;
using ::moriarty_testing::ThrowsMVariableTypeMismatch;
using ::moriarty_testing::ThrowsVariableNotFound;

template <typename T>
T GetVariable(const TestCase& test_case, std::string_view name) {
  auto [variables, values] = UnsafeExtractTestCaseInternals(test_case);
  return variables.GetVariable<T>(name);
}

template <typename T>
T::value_type GetValue(const TestCase& test_case, std::string_view name) {
  auto [variables, values] = UnsafeExtractTestCaseInternals(test_case);
  return values.Get<T>(name);
}

absl::StatusOr<ValueSet> AssignAllValues(const TestCase& test_case) {
  auto [variables, values] = UnsafeExtractTestCaseInternals(test_case);
  RandomEngine rng({1, 2, 3}, "v0.1");
  return GenerateAllValues(variables, values, {rng, std::nullopt});
}

TEST(TestCaseTest, ConstrainVariableAndGetVariableWorkInGeneralCase) {
  TestCase T;
  T.ConstrainVariable("A", MTestType());
  GetVariable<MTestType>(T, "A");  // No crash = good.
}

TEST(TestCaseTest, SetValueWithSpecificValueAndGetVariableWorkInGeneralCase) {
  TestCase T;
  T.SetValue<MTestType>("A", TestType());
  EXPECT_EQ(GetValue<MTestType>(T, "A"), TestType());
}

TEST(TestCaseTest, GetVariableWithWrongTypeShouldFail) {
  TestCase T;
  T.ConstrainVariable("A", MTestType());
  EXPECT_THAT([&] { GetVariable<MInteger>(T, "A"); },
              ThrowsMVariableTypeMismatch("MTestType", "MInteger"));
}

TEST(TestCaseTest, GetVariableWithWrongNameShouldNotFind) {
  TestCase T;
  T.ConstrainVariable("A", MTestType());
  EXPECT_THAT([&] { GetVariable<MTestType>(T, "xxx"); },
              ThrowsVariableNotFound("xxx"));
}

TEST(TestCaseTest, AssignAllValuesGivesSomeValueForEachVariable) {
  TestCase T;
  T.ConstrainVariable("A", MTestType());
  T.ConstrainVariable("B", MTestType());

  MORIARTY_ASSERT_OK_AND_ASSIGN(ValueSet value_set, AssignAllValues(T));

  EXPECT_TRUE(value_set.Contains("A"));
  EXPECT_TRUE(value_set.Contains("B"));
  EXPECT_FALSE(value_set.Contains("C"));
}

TEST(TestCaseTest, ConstrainAnonymousVariableAndGetVariableWorkInGeneralCase) {
  TestCase T;
  T.ConstrainAnonymousVariable("A", MTestType());
  T.ConstrainAnonymousVariable("B", MTestType());

  MORIARTY_ASSERT_OK_AND_ASSIGN(ValueSet value_set, AssignAllValues(T));

  EXPECT_TRUE(value_set.Contains("A"));
  EXPECT_TRUE(value_set.Contains("B"));
  EXPECT_FALSE(value_set.Contains("C"));
}

TEST(TestCaseTest, AssignAllValuesShouldGiveRepeatableResults) {
  using NameVariablePair = std::pair<std::string, MTestType>;

  // Generate the value for A, B, C in some order and return those values in the
  // order A, B, C.
  auto gen = [](NameVariablePair first, NameVariablePair second,
                NameVariablePair third)
      -> absl::StatusOr<std::tuple<TestType, TestType, TestType>> {
    TestCase T;
    T.ConstrainVariable(first.first, first.second);
    T.ConstrainVariable(second.first, second.second);
    T.ConstrainVariable(third.first, third.second);

    RandomEngine rng({1, 2, 3}, "v0.1");
    MORIARTY_ASSIGN_OR_RETURN(ValueSet value_set, AssignAllValues(T));
    return std::tuple<TestType, TestType, TestType>({
        value_set.Get<MTestType>("A"),
        value_set.Get<MTestType>("B"),
        value_set.Get<MTestType>("C"),
    });
  };

  NameVariablePair a = {"A",
                        MTestType().SetMultiplier(MInteger(Between(1, 10)))};
  NameVariablePair b = {"B",
                        MTestType().SetMultiplier(MInteger(Between(1, 3)))};
  NameVariablePair c = {"C", MTestType().SetAdder("A")};

  MORIARTY_ASSERT_OK_AND_ASSIGN(auto abc, gen(a, b, c));
  EXPECT_THAT(gen(a, c, b), IsOkAndHolds(abc));
  EXPECT_THAT(gen(b, a, c), IsOkAndHolds(abc));
  EXPECT_THAT(gen(b, c, a), IsOkAndHolds(abc));
  EXPECT_THAT(gen(c, b, a), IsOkAndHolds(abc));
  EXPECT_THAT(gen(c, a, b), IsOkAndHolds(abc));
}

TEST(TestCaseTest, AssignAllValuesShouldRespectSpecificValuesSet) {
  TestCase T;
  T.SetValue<MTestType>("A", TestType(789));
  T.SetValue<MTestType>("B", TestType(654));

  MORIARTY_ASSERT_OK_AND_ASSIGN(ValueSet value_set, AssignAllValues(T));

  EXPECT_EQ(value_set.Get<MTestType>("A"), TestType(789));
  EXPECT_EQ(value_set.Get<MTestType>("B"), TestType(654));
}

}  // namespace
}  // namespace moriarty
