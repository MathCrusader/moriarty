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

#include "src/internal/generation_bootstrap.h"

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

#include "absl/algorithm/container.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/internal/random_engine.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/testing/gtest_helpers.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/constraints/numeric_constraints.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

using ::moriarty_testing::Context;
using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::Each;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::HasSubstr;
using ::testing::Le;
using ::testing::SizeIs;
using ::testing::ThrowsMessage;

TEST(GenerationBootstrapTest,
     GenerateAllValuesWithNoVariablesButSomeKnownValuesSucceeds) {
  RandomEngine rng({1, 2, 3}, "");
  ValueSet known_values;
  known_values.Set<MInteger>("A", 5);
  known_values.Set<MInteger>("B", 10);

  ValueSet values = GenerateAllValues(VariableSet(), known_values, {rng});

  EXPECT_EQ(values.Get<MInteger>("A"), 5);
  EXPECT_EQ(values.Get<MInteger>("B"), 10);
}

TEST(
    GenerationBootstrapTest,
    GenerateAllValuesWithKnownValueThatIsNotInVariablesIncludesThatKnownValue) {
  RandomEngine rng({1, 2, 3}, "");
  Context context = Context()
                        .WithVariable("A", MInteger(Between(123, 456)))
                        .WithValue<MInteger>("B", 10);

  ValueSet values =
      GenerateAllValues(context.Variables(), context.Values(), {rng});

  EXPECT_THAT(values.Get<MInteger>("A"), AllOf(Ge(123), Le(456)));
  EXPECT_EQ(values.Get<MInteger>("B"), 10);
}

TEST(GenerationBootstrapTest, GenerateAllValuesGivesValuesToVariables) {
  RandomEngine rng({1, 2, 3}, "");

  Context context = Context()
                        .WithVariable("A", MInteger(Between(123, 456)))
                        .WithVariable("B", MInteger(Between(777, 888)));

  ValueSet values = GenerateAllValues(context.Variables(), ValueSet(), {rng});

  EXPECT_THAT(values.Get<MInteger>("A"), AllOf(Ge(123), Le(456)));
  EXPECT_THAT(values.Get<MInteger>("B"), AllOf(Ge(777), Le(888)));
}

TEST(GenerationBootstrapTest, GenerateAllValuesRespectsKnownValues) {
  RandomEngine rng({1, 2, 3}, "");
  Context context = Context()
                        .WithVariable("A", MInteger(Between(123, 456)))
                        .WithValue<MInteger>("A", 314);

  ValueSet values =
      GenerateAllValues(context.Variables(), context.Values(), {rng});

  EXPECT_EQ(values.Get<MInteger>("A"), 314);
}

TEST(GenerationBootstrapTest, GenerateAllValuesWithDependentVariablesSucceeds) {
  RandomEngine rng({1, 2, 3}, "");
  Context context = Context()
                        .WithVariable("A", MInteger(Between("N", "3 * N")))
                        .WithVariable("N", MInteger(Between(50, 100)));

  ValueSet values = GenerateAllValues(context.Variables(), ValueSet(), {rng});

  EXPECT_THAT(values.Get<MInteger>("N"), AllOf(Ge(50), Le(100)));
  int64_t N = values.Get<MInteger>("N");
  EXPECT_THAT(values.Get<MInteger>("A"), AllOf(Ge(N), Le(3 * N)));
}

TEST(GenerationBootstrapTest, GenerateAllValuesWithDependentValuesSucceeds) {
  RandomEngine rng({1, 2, 3}, "");
  Context context = Context()
                        .WithVariable("A", MInteger(Between("N", "3 * N")))
                        .WithVariable("C", MInteger(Exactly("N")))
                        .WithVariable("B", MInteger(Exactly("2 * C")))
                        .WithValue<MInteger>("N", 53);

  ValueSet values =
      GenerateAllValues(context.Variables(), context.Values(), {rng});

  EXPECT_THAT(values.Get<MInteger>("A"), AllOf(Ge(53), Le(3 * 53)));
  EXPECT_EQ(values.Get<MInteger>("C"), 53);
  EXPECT_EQ(values.Get<MInteger>("B"), 2 * 53);
}

TEST(GenerationBootstrapTest,
     GenerateAllValuesWithMissingDependentVariableAndValueFails) {
  RandomEngine rng({1, 2, 3}, "");
  Context context =
      Context().WithVariable("A", MInteger(Between("N", "3 * N")));

  EXPECT_THAT(
      [&] { GenerateAllValues(context.Variables(), ValueSet(), {rng}); },
      ThrowsMessage<std::runtime_error>(HasSubstr("unknown")));
}

TEST(GenerationBootstrapTest, GenerateAllValuesShouldRespectIsAndIsOneOf) {
  RandomEngine rng({1, 2, 3}, "");
  Context context = Context()
                        .WithVariable("A", MInteger(Exactly(15)))
                        .WithVariable("B", MInteger(OneOf({111, 222, 333})));

  ValueSet values = GenerateAllValues(context.Variables(), ValueSet(), {rng});

  EXPECT_THAT(values.Get<MInteger>("A"), 15);
  EXPECT_THAT(values.Get<MInteger>("B"), AnyOf(111, 222, 333));
}

TEST(GenerationBootstrapTest,
     GenerateAllValuesWithAKnownValueThatIsInvalidShouldFail) {
  RandomEngine rng({1, 2, 3}, "");
  Context context = Context()
                        .WithVariable("A", MInteger(Between(123, 456)))
                        .WithValue<MInteger>("A", 0);

  EXPECT_THAT(
      [&] { GenerateAllValues(context.Variables(), context.Values(), {rng}); },
      ThrowsMessage<std::runtime_error>(HasSubstr("A")));
}

TEST(GenerationBootstrapTest,
     GenerateAllValuesGivesStableResultsNoMatterTheInsertionOrder) {
  std::unordered_map<std::string, MInteger> named_variables = {
      {"A", MInteger(Between(111, 222))},
      {"B", MInteger(Between(333, 444))},
      {"C", MInteger(Between(555, 666))},
  };
  std::vector<std::string> names = {"A", "B", "C"};

  // Do all 3! = 6 permutations, check they all generate the same values.
  std::vector<std::tuple<int64_t, int64_t, int64_t>> results;
  do {
    RandomEngine rng({1, 2, 3}, "");
    Context context;
    for (const std::string& name : names) {
      context.WithVariable(name, named_variables[name]);
    }

    ValueSet values = GenerateAllValues(context.Variables(), ValueSet(), {rng});
    results.push_back({
        values.Get<MInteger>("A"),
        values.Get<MInteger>("B"),
        values.Get<MInteger>("C"),
    });
  } while (absl::c_next_permutation(names));

  ASSERT_THAT(results, SizeIs(6));
  EXPECT_THAT(results, Each(Eq(results[0])));
}

TEST(GenerationBootstrapTest,
     GenerateAllValuesGivesStableResultsWithDependentVariables) {
  std::unordered_map<std::string, MInteger> named_variables = {
      {"A", MInteger(Between(111, "B"))},  // Forwards in alphabet. A -> B
      {"B", MInteger(Between(222, 333))},  //
      {"C", MInteger(Between(444, 555))},  // Backwards in alphabet. D -> C
      {"D", MInteger(Between("C", 666))},  //
      {"E", MInteger(Between(777, "G"))},  // Two layers deep E -> G -> F
      {"F", MInteger(Between(888, 999))},  //
      {"G", MInteger(Between("F", 999))}};
  std::vector<std::string> names = {"A", "B", "C", "D", "E", "F", "G"};

  // Do all 7! = 5040 permutations, check they all generate the same values.
  std::vector<std::vector<int64_t>> results;
  do {
    RandomEngine rng({1, 2, 3}, "");
    Context context;
    for (const std::string& name : names) {
      context.WithVariable(name, named_variables[name]);
    }

    ValueSet values = GenerateAllValues(context.Variables(), ValueSet(), {rng});

    results.push_back({
        values.Get<MInteger>("A"),
        values.Get<MInteger>("B"),
        values.Get<MInteger>("C"),
        values.Get<MInteger>("D"),
        values.Get<MInteger>("E"),
        values.Get<MInteger>("F"),
        values.Get<MInteger>("G"),
    });
  } while (absl::c_next_permutation(names));

  ASSERT_THAT(results, SizeIs(5040));
  EXPECT_THAT(results, Each(Eq(results[0])));
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
