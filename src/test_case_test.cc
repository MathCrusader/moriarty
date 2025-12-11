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

#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/internal/generation_bootstrap.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/testing/gtest_helpers.h"
#include "src/librarian/testing/mtest_type.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace {

using ::moriarty::moriarty_internal::GenerateAllValues;
using ::moriarty::moriarty_internal::ValueSet;
using ::moriarty_testing::Context;
using ::moriarty_testing::LastDigit;
using ::moriarty_testing::MTestType;
using ::moriarty_testing::NumberOfDigits;
using ::moriarty_testing::TestType;
using ::moriarty_testing::ThrowsMVariableTypeMismatch;
using ::moriarty_testing::ThrowsVariableNotFound;

template <typename T>
T GetVariable(const MTestCase& test_case, std::string_view name) {
  return test_case.UnsafeGetVariables().GetVariable<T>(name);
}

template <typename T>
T::value_type GetValue(const MTestCase& test_case, std::string_view name) {
  return test_case.UnsafeGetValues().Get<T>(name);
}

ValueSet AssignAllValues(const MTestCase& test_case) {
  Context ctx;
  return GenerateAllValues(test_case.UnsafeGetVariables(),
                           test_case.UnsafeGetValues(), {ctx.RandomEngine()});
}

TEST(MTestCaseTest, ConstrainVariableAndGetVariableWorkInGeneralCase) {
  MTestCase T;
  T.ConstrainVariable("A", MTestType());
  GetVariable<MTestType>(T, "A");  // No crash = good.
}

TEST(MTestCaseTest, SetValueWithSpecificValueAndGetVariableWorkInGeneralCase) {
  MTestCase T;
  T.SetValue<MTestType>("A", TestType(123));
  EXPECT_EQ(GetValue<MTestType>(T, "A"), TestType(123));
}

TEST(MTestCaseTest, GetVariableWithWrongTypeShouldFail) {
  MTestCase T;
  T.ConstrainVariable("A", MTestType());
  EXPECT_THAT([&] { GetVariable<MInteger>(T, "A"); },
              ThrowsMVariableTypeMismatch("MTestType", "MInteger"));
}

TEST(MTestCaseTest, GetVariableWithWrongNameShouldNotFind) {
  MTestCase T;
  T.ConstrainVariable("A", MTestType());
  EXPECT_THAT([&] { GetVariable<MTestType>(T, "xxx"); },
              ThrowsVariableNotFound("xxx"));
}

TEST(MTestCaseTest, AssignAllValuesGivesSomeValueForEachVariable) {
  MTestCase T;
  T.ConstrainVariable("A", MTestType());
  T.ConstrainVariable("B", MTestType());

  ValueSet value_set = AssignAllValues(T);

  EXPECT_TRUE(value_set.Contains("A"));
  EXPECT_TRUE(value_set.Contains("B"));
  EXPECT_FALSE(value_set.Contains("C"));
}

TEST(MTestCaseTest, ConstrainAnonymousVariableAndGetVariableWorkInGeneralCase) {
  MTestCase T;
  T.ConstrainAnonymousVariable("A", MTestType());
  T.ConstrainAnonymousVariable("B", MTestType());

  ValueSet value_set = AssignAllValues(T);

  EXPECT_TRUE(value_set.Contains("A"));
  EXPECT_TRUE(value_set.Contains("B"));
  EXPECT_FALSE(value_set.Contains("C"));
}

TEST(MTestCaseTest, AssignAllValuesShouldGiveRepeatableResults) {
  using NameVariablePair = std::pair<std::string, MTestType>;

  // Generate the value for A, B, C in some order and return those values in the
  // order A, B, C.
  auto gen =
      [](NameVariablePair first, NameVariablePair second,
         NameVariablePair third) -> std::tuple<TestType, TestType, TestType> {
    MTestCase T;
    T.ConstrainVariable(first.first, first.second);
    T.ConstrainVariable(second.first, second.second);
    T.ConstrainVariable(third.first, third.second);

    ValueSet value_set = AssignAllValues(T);
    return std::tuple<TestType, TestType, TestType>({
        value_set.Get<MTestType>("A"),
        value_set.Get<MTestType>("B"),
        value_set.Get<MTestType>("C"),
    });
  };

  NameVariablePair a = {"A",
                        MTestType(NumberOfDigits(MInteger(Between(2, 8))))};
  NameVariablePair b = {"B",
                        MTestType(NumberOfDigits(MInteger(Between(1, 6))))};
  NameVariablePair c = {"C", MTestType(LastDigit(MInteger(Between(2, 7))))};

  auto abc = gen(a, b, c);
  EXPECT_EQ(gen(a, c, b), abc);
  EXPECT_EQ(gen(b, a, c), abc);
  EXPECT_EQ(gen(b, c, a), abc);
  EXPECT_EQ(gen(c, b, a), abc);
  EXPECT_EQ(gen(c, a, b), abc);
}

TEST(MTestCaseTest, AssignAllValuesShouldRespectSpecificValuesSet) {
  MTestCase T;
  T.SetValue<MTestType>("A", TestType(789));
  T.SetValue<MTestType>("B", TestType(654));

  ValueSet value_set = AssignAllValues(T);

  EXPECT_EQ(value_set.Get<MTestType>("A"), TestType(789));
  EXPECT_EQ(value_set.Get<MTestType>("B"), TestType(654));
}

}  // namespace
}  // namespace moriarty
