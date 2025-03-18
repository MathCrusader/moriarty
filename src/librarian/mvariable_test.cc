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

#include "src/librarian/mvariable.h"

#include <cstdint>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/contexts/librarian/analysis_context.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/generation_config.h"
#include "src/internal/random_engine.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/test_utils.h"
#include "src/testing/mtest_type.h"
#include "src/testing/status_test_util.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace librarian {
namespace {

using ::moriarty::IsOkAndHolds;
using ::moriarty::StatusIs;
using ::moriarty_testing::Context;
using ::moriarty_testing::Generate;
using ::moriarty_testing::GeneratedValuesAre;
using ::moriarty_testing::GetUniqueValue;
using ::moriarty_testing::IsNotSatisfiedWith;
using ::moriarty_testing::IsSatisfiedWith;
using ::moriarty_testing::MTestType;
using ::moriarty_testing::Print;
using ::moriarty_testing::Read;
using ::moriarty_testing::TestType;
using ::moriarty_testing::ThrowsValueNotFound;
using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::Contains;
using ::testing::HasSubstr;
using ::testing::Optional;
using ::testing::SizeIs;
using ::testing::Truly;

TEST(MVariableTest, PrintShouldSucceed) {
  EXPECT_THAT(Print(MTestType(), -1), IsOkAndHolds("-1"));
  EXPECT_THAT(Print(MTestType(), 153), IsOkAndHolds("153"));
}

TEST(MVariableTest, GenerateShouldProduceAValue) {
  EXPECT_THAT(Generate(MTestType()), IsOkAndHolds(MTestType::kGeneratedValue));
}

TEST(MVariableTest, GenerateShouldObserveIs) {
  EXPECT_THAT(Generate(MTestType().Is(10)), IsOkAndHolds(10));
}

TEST(MVariableTest, GenerateShouldObserveIsOneOf) {
  EXPECT_THAT(GenerateN(MTestType().IsOneOf({50, 60, 70, 80, 90, 100}), 100),
              IsOkAndHolds(AllOf(Contains(50), Contains(60), Contains(70),
                                 Contains(80), Contains(90), Contains(100))));
}

TEST(MVariableTest, MergeFromShouldWork) {
  MTestType var1;
  MTestType var2;

  var1.MergeFrom(var2);
  EXPECT_TRUE(var1.WasMerged());
  EXPECT_FALSE(var2.WasMerged());

  var2.MergeFrom(var1);
  EXPECT_TRUE(var1.WasMerged());
  EXPECT_TRUE(var2.WasMerged());
}

TEST(MVariableTest, MergeFromShouldRespectOneOf) {
  MTestType var1 =
      MTestType().IsOneOf({TestType(11), TestType(22), TestType(33)});
  MTestType var2 =
      MTestType().IsOneOf({TestType(22), TestType(33), TestType(44)});

  ASSERT_THAT(var1, GeneratedValuesAre(
                        AnyOf(TestType(11), TestType(22), TestType(33))));

  var1.MergeFrom(var2);
  EXPECT_THAT(var1, GeneratedValuesAre(AnyOf(TestType(22), TestType(33))));
}

TEST(MVariableTest, MergeFromWithWrongTypeShouldFail) {
  MTestType var1;
  moriarty::MInteger var2;

  EXPECT_THAT(var1.MergeFromAnonymous(var2),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Unable to convert")));
  EXPECT_FALSE(var1.WasMerged());
}

TEST(MVariableTest, MergeFromUsingAbstractVariablesShouldRespectIsAndIsOneOf) {
  auto merge_from = [](MTestType& a, const MTestType& b) {
    ABSL_CHECK_OK(
        static_cast<moriarty_internal::AbstractVariable*>(&a)->MergeFrom(b));
  };

  {
    MTestType var1 = MTestType().Is(10);
    MTestType var2;

    merge_from(var1, var2);
    EXPECT_THAT(Generate(var1), IsOkAndHolds(10));
    EXPECT_TRUE(var1.WasMerged());
  }

  {
    MTestType var1;
    MTestType var2 = MTestType().Is(20);

    merge_from(var1, var2);
    EXPECT_THAT(Generate(var1), IsOkAndHolds(20));
    EXPECT_TRUE(var1.WasMerged());
  }

  {
    MTestType var1 = MTestType().Is(10);
    MTestType var2 = MTestType().Is(20);

    merge_from(var1, var2);
    EXPECT_THROW({ Generate(var1).IgnoreError(); }, std::runtime_error);
    EXPECT_TRUE(var1.WasMerged());  // Was merged, but bad options.
  }

  {
    MTestType var1 = MTestType().Is(10);
    MTestType var2 = MTestType().IsOneOf({40, 30, 20, 10});

    merge_from(var1, var2);
    EXPECT_THAT(Generate(var1), IsOkAndHolds(10));
    EXPECT_TRUE(var1.WasMerged());
  }

  {
    MTestType var1 = MTestType().IsOneOf({300, 17, 10, -1234});
    MTestType var2 = MTestType().IsOneOf({40, 30, 20, 10});

    merge_from(var1, var2);
    EXPECT_THAT(Generate(var1), IsOkAndHolds(10));
    EXPECT_TRUE(var1.WasMerged());
  }
}

TEST(MVariableTest, SubvariablesShouldBeSetableAndUseable) {
  MTestType var = MTestType().SetMultiplier(MInteger(Between(2, 2)));

  EXPECT_THAT(Generate(var), IsOkAndHolds(2 * MTestType::kGeneratedValue));
}

TEST(MVariableTest, BasingMyVariableOnAnotherSetValueShouldWorkBasicCase) {
  EXPECT_THAT(Generate(MTestType().SetAdder("x"),
                       Context().WithValue<MTestType>("x", TestType(20))),
              IsOkAndHolds(20 + MTestType::kGeneratedValue));
}

TEST(MVariableTest, BasingMyVariableOnAnotherUnSetVariableShouldWorkBasicCase) {
  EXPECT_THAT(Generate(MTestType().SetAdder("x"),
                       Context().WithVariable("x", MTestType())),
              IsOkAndHolds(2 * MTestType::kGeneratedValue));
}

TEST(MVariableTest, BasingMyVariableOnANonexistentOneShouldFail) {
  EXPECT_THROW(
      { Generate(MTestType().SetAdder("x")).IgnoreError(); },
      std::runtime_error);
}

TEST(MVariableTest, DependentVariableOfTheWrongTypeShouldFail) {
  EXPECT_THROW(
      {
        Generate(MTestType().SetAdder("x"),
                 Context().WithVariable("x", MInteger()))
            .IgnoreError();
      },  // Wrong type
      std::runtime_error);
}

TEST(MVariableTest, DependentVariablesInSubvariablesCanChain) {
  Context c = Context()
                  .WithVariable("x", MTestType().SetAdder("y"))
                  .WithVariable("y", MTestType().SetAdder("z"))
                  .WithValue<MTestType>("z", TestType(30));

  // Adds x (GeneratedValue), y (GeneratedValue), and z (30) to itself
  // (GeneratedValue).
  EXPECT_THAT(Generate(MTestType().SetAdder("x"), c),
              IsOkAndHolds(30 + 3 * MTestType::kGeneratedValue));
}

TEST(MVariableTest, SeparateCallsToGetShouldUseTheSameDependentVariableValue) {
  moriarty_internal::VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MInteger(Between("N", "N"))));
  MORIARTY_ASSERT_OK(variables.AddVariable("B", MInteger(Between("N", "N"))));
  MORIARTY_ASSERT_OK(
      variables.AddVariable("N", MInteger(Between(1, 1000000000))));

  moriarty_internal::ValueSet values;

  moriarty_internal::GenerationConfig generation_config;
  moriarty_internal::RandomEngine engine({1, 2, 3}, "v0.1");

  MORIARTY_ASSERT_OK_AND_ASSIGN(moriarty_internal::AbstractVariable * var_A,
                                variables.GetAbstractVariable("A"));
  ResolverContext ctxA("A", variables, values, engine, generation_config);
  MORIARTY_ASSERT_OK(
      var_A->AssignValue(ctxA));  // By assigning A, we assigned N.

  MORIARTY_ASSERT_OK_AND_ASSIGN(moriarty_internal::AbstractVariable * var_B,
                                variables.GetAbstractVariable("B"));
  ResolverContext ctxB("B", variables, values, engine, generation_config);
  MORIARTY_ASSERT_OK(
      var_B->AssignValue(ctxB));  // Should use the already generated N.
  MORIARTY_ASSERT_OK_AND_ASSIGN(int N, values.Get<MInteger>("N"));

  EXPECT_THAT(values.Get<MInteger>("A"), IsOkAndHolds(N));
  EXPECT_THAT(values.Get<MInteger>("B"), IsOkAndHolds(N));
}

TEST(MVariableTest, CyclicDependenciesShouldFail) {
  EXPECT_THROW(
      {
        Generate(MTestType().SetAdder("y"),
                 Context()
                     .WithVariable("y", MTestType().SetAdder("z"))
                     .WithVariable("z", MTestType().SetAdder("y")))
            .IgnoreError();
      },
      std::runtime_error);

  // Self-loop
  EXPECT_THROW(
      {
        Generate(MTestType().SetAdder("y"),
                 Context().WithVariable("y", MTestType().SetAdder("y")))
            .IgnoreError();
      },
      std::runtime_error);
}

TEST(MVariableTest, SatisfiesConstraintsWorksForValid) {
  // In the simplist case, everything will work since it is checking it is a
  // multiple of 1.
  EXPECT_THAT(MTestType(), IsSatisfiedWith(5));
  EXPECT_THAT(MTestType(), IsSatisfiedWith(MTestType::kGeneratedValue));

  // Now set the multiplier. The number must be a multiple of something in the
  // range.
  EXPECT_THAT(MTestType().SetMultiplier(MInteger(Between(5, 5))),
              IsSatisfiedWith(100));
  EXPECT_THAT(MTestType().SetMultiplier(MInteger(Between(3, 7))),
              IsSatisfiedWith(100));  // Multiple of both 4 and 5

  // 101 is prime, so not a multiple of 3,4,5,6,7.
  EXPECT_THAT(
      MTestType().SetMultiplier(MInteger(Between(3, 7))),
      IsNotSatisfiedWith(101, "not a multiple of any valid multiplier"));
}

TEST(MVariableTest, SatisfiesConstraintsNeedsDependentValues) {
  {  // No value or variable known
    moriarty_internal::VariableSet variables;
    moriarty_internal::ValueSet values;
    librarian::AnalysisContext ctx("test", variables, values);

    EXPECT_THAT(
        [&] {
          MTestType()
              .SetAdder("t")
              .SetMultiplier(MInteger(Between(3, 3)))
              .IsSatisfiedWith(ctx, 1)
              .IgnoreError();
        },
        ThrowsValueNotFound("t"));
  }
  {  // Variable known, but no value known.
    moriarty_internal::VariableSet variables;
    MORIARTY_ASSERT_OK(variables.AddVariable("t", MTestType()));
    moriarty_internal::ValueSet values;
    librarian::AnalysisContext ctx("test", variables, values);

    EXPECT_THAT(
        [&] {
          MTestType()
              .SetAdder("t")
              .SetMultiplier(MInteger(Between(3, 3)))
              .IsSatisfiedWith(ctx, 1)
              .IgnoreError();
        },
        ThrowsValueNotFound("t"));
  }
}

TEST(MVariableTest, SatisfiesConstraintsWorksWithDependentValues) {
  // x = 107 + 3 * [something].
  //  So, 110 = 107 + 3 * 1 is valid, and 111 is not.
  EXPECT_THAT(
      MTestType().SetAdder("t").SetMultiplier(MInteger(Between(3, 3))),
      IsSatisfiedWith(110, Context().WithValue<MTestType>("t", TestType(107))));
  EXPECT_THAT(
      MTestType().SetAdder("t").SetMultiplier(MInteger(Between(3, 3))),
      IsNotSatisfiedWith(111, "not a multiple of any valid multiplier",
                         Context().WithValue<MTestType>("t", TestType(107))));
}

TEST(MVariableTest, SatisfiesConstraintsCanValidateSubvariablesIfNeeded) {
  // Must use AtMost/AtLeast since Between will not allow invalid range.
  // (5, 0) is an invalid range. Should fail validation.
  EXPECT_THAT(MTestType().SetMultiplier(MInteger(AtLeast(5), AtMost(0))),
              IsNotSatisfiedWith(1, "not a multiple of any valid multiplier"));
}

TEST(MVariableTest, SatisfiesConstraintsShouldAcknowledgeIsAndIsOneOf) {
  // In the simplist case, everything will work since it is checking it is a
  // multiple of 1.
  EXPECT_THAT(MTestType().IsOneOf({2, 3, 5, 7}), IsSatisfiedWith(5));
  EXPECT_THAT(MTestType().IsOneOf({2, 4, 8}), IsNotSatisfiedWith(5, "IsOneOf"));

  // 8 is in the list and a multiple of 4.
  EXPECT_THAT(
      MTestType().IsOneOf({2, 4, 8}).SetMultiplier(MInteger(Between(4, 4))),
      IsSatisfiedWith(8));
}

TEST(MVariableTest,
     SatisfiesConstraintsShouldFailIfIsOneOfSucceedsButLowerFails) {
  // 2 is in the one-of list, but not a multiple of 4.
  EXPECT_THAT(
      MTestType().IsOneOf({2, 4, 8}).SetMultiplier(MInteger(Between(4, 4))),
      IsNotSatisfiedWith(2, "not a multiple of any valid multiplier"));
}

TEST(MVariableTest, SatisfiesConstraintsShouldCheckCustomConstraints) {
  auto is_small_prime = [](const TestType& value) -> bool {
    if (value < 2) return false;
    for (int64_t p = 2; p * p <= value; p++)
      if (value % p == 0) return false;
    return true;
  };

  MTestType var = MTestType().AddCustomConstraint("Prime", is_small_prime);

  EXPECT_THAT(var, IsSatisfiedWith(2));
  EXPECT_THAT(var, IsSatisfiedWith(3));
  EXPECT_THAT(var, IsNotSatisfiedWith(4, "Prime"));
}

TEST(MVariableTest, SatisfiesConstraintsShouldCheckMultipleCustomConstraints) {
  MTestType var =
      MTestType()
          .AddCustomConstraint("Prime",
                               [](const TestType& value) -> bool {
                                 if (value < 2) return false;
                                 for (int64_t p = 2; p * p <= value; p++)
                                   if (value % p == 0) return false;
                                 return true;
                               })
          .AddCustomConstraint("3 mod 4", [](const TestType& value) -> bool {
            return value % 4 == 3;
          });

  // Yes prime, yes 3 mod 4.
  EXPECT_THAT(var, IsSatisfiedWith(7));

  // Yes prime, not 3 mod 4.
  EXPECT_THAT(var, IsNotSatisfiedWith(2, "3 mod 4"));

  // Not prime, yes 3 mod 4.
  EXPECT_THAT(var, IsNotSatisfiedWith(15, "Prime"));

  // Not prime, not 3 mod 4.
  EXPECT_THAT(var, IsNotSatisfiedWith(6, "Prime"));
}

TEST(MVariableTest, CustomConstraintWithDependentVariables) {
  EXPECT_THAT(
      Generate(MTestType()
                   .SetMultiplier(MInteger().Is(2))
                   .AddCustomConstraint(
                       "Custom1", {"A"},
                       [](AnalysisContext ctx, const TestType& value) -> bool {
                         TestType A = ctx.GetValue<MTestType>("A");
                         return A.value == value;
                       }),
               Context().WithVariable(
                   "A", MTestType().SetMultiplier(MInteger().Is(2)))),
      IsOkAndHolds(MTestType::kGeneratedValue * 2));
}

TEST(MVariableTest, CustomConstraintInvalidFailsToGenerate) {
  EXPECT_THROW(
      {
        Generate(
            MTestType()
                .SetMultiplier(MInteger().Is(1))
                .AddCustomConstraint(
                    "Custom1", {"A"},
                    [](AnalysisContext ctx, const TestType& value) -> bool {
                      TestType A = ctx.GetValue<MTestType>("A");
                      return A.value == value;
                    }),
            Context().WithVariable("A",
                                   MTestType().SetMultiplier(MInteger().Is(2))))
            .IgnoreError();
      },
      std::runtime_error);
}

TEST(MVariableTest, CustomConstraintsWithInvalidDependentVariableCrashes) {
  EXPECT_THROW(
      {
        Generate(MTestType()
                     .SetMultiplier(MInteger().Is(1))
                     .AddCustomConstraint("Custom1", {},
                                          [](AnalysisContext ctx,
                                             const TestType& value) -> bool {
                                            TestType A =
                                                ctx.GetValue<MTestType>("A");
                                            return A.value == value;
                                          }))
            .IgnoreError();
      },
      std::runtime_error);
}

TEST(MVariableTest,
     CustomConstraintsKnownVariableButNotAddedAsDependencyFailsGeneration) {
  // Note that "L"'s value is not known since the generated variable's name is
  // "Generate(MTestType)" -- which comes before "L" alphabetically. If the
  // dependent's name was "A" instead of "L", this would pass, but it is not
  // guaranteed to since the user didn't specify it as a constraint.
  EXPECT_THROW(
      {
        Generate(
            MTestType()
                .SetMultiplier(MInteger().Is(1))
                .AddCustomConstraint(
                    "Custom1", {},
                    [](AnalysisContext ctx, const TestType& value) -> bool {
                      TestType A = ctx.GetValue<MTestType>("A");
                      return A.value == value;
                    }),
            Context().WithVariable("L",
                                   MTestType().SetMultiplier(MInteger().Is(2))))
            .IgnoreError();
      },
      std::runtime_error);
}

// TODO(hivini): Test that by default, nothing is returned from
// ListEdgeCases.

TEST(MVariableTest, ListEdgeCasesPassesTheInstancesThrough) {
  AnalysisContext ctx("test", {}, {});
  EXPECT_THAT(MTestType().ListEdgeCases(ctx), SizeIs(2));
}

TEST(MVariableTest, ListEdgeCasesReturnsMergedInstance) {
  MTestType original = MTestType();
  AnalysisContext ctx("test", {}, {});
  std::vector<MTestType> difficult_instances = original.ListEdgeCases(ctx);

  EXPECT_THAT(difficult_instances, SizeIs(2));
  EXPECT_TRUE(difficult_instances[0].WasMerged());
  EXPECT_TRUE(difficult_instances[1].WasMerged());
  EXPECT_THAT(Generate(difficult_instances[0]), IsOkAndHolds(2));
  EXPECT_THAT(Generate(difficult_instances[1]), IsOkAndHolds(3));
}

class EmptyClass {
 public:
  bool operator==(const EmptyClass& other) const { return true; }
  bool operator<(const EmptyClass& other) const { return false; }
};
class MEmptyClass : public MVariable<MEmptyClass, EmptyClass> {
 public:
  std::string Typename() const override { return "MEmptyClass"; }

 private:
  EmptyClass GenerateImpl(ResolverContext) const override {
    throw std::runtime_error("Unimplemented: GenerateImpl");
  }
  absl::Status IsSatisfiedWithImpl(librarian::AnalysisContext ctx,
                                   const EmptyClass& c) const override {
    return absl::UnimplementedError("IsSatisfiedWith");
  }
  absl::Status MergeFromImpl(const MEmptyClass& c) override {
    return absl::UnimplementedError("MergeFromImpl");
  }
};

TEST(MVariableTest, MVariableShouldByDefaultNotBeAbleToRead) {
  EXPECT_THROW(
      { Read(MEmptyClass(), "1234").IgnoreError(); }, std::runtime_error);
}

TEST(MVariableTest, MVariableShouldByDefaultNotBeAbleToPrint) {
  EXPECT_THROW(
      { Print(MEmptyClass(), EmptyClass()).IgnoreError(); },
      std::runtime_error);
}

TEST(MVariableTest, ReadValueShouldBeSuccessfulInNormalState) {
  EXPECT_THAT(Read(MTestType(), "1234"), IsOkAndHolds(1234));
}

TEST(MVariableTest, FailedReadShouldFail) {
  EXPECT_THROW({ Read(MTestType(), "bad").IgnoreError(); }, std::runtime_error);
}

TEST(MVariableTest, PrintValueShouldBeSuccessfulInNormalState) {
  EXPECT_THAT(Print(MTestType(), TestType(1234)), IsOkAndHolds("1234"));
}

TEST(MVariableTest, PrintValueShouldPrintTheAssignedValue) {
  moriarty_internal::VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("x", MTestType()));

  moriarty_internal::ValueSet values;
  values.Set<MTestType>("x", TestType(12345));

  std::stringstream ss;

  PrinterContext ctx("x", ss, variables, values);
  MORIARTY_ASSERT_OK_AND_ASSIGN(moriarty_internal::AbstractVariable * var,
                                variables.GetAbstractVariable("x"));
  MORIARTY_ASSERT_OK(var->PrintValue(ctx));
  EXPECT_EQ(ss.str(), "12345");
}

TEST(MVariableTest, GetUniqueValueShouldReturnUniqueValueViaIsMethod) {
  EXPECT_THAT(GetUniqueValue(MInteger().Is(123)), Optional(123));
  EXPECT_THAT(GetUniqueValue(MTestType().Is(2 * MTestType::kGeneratedValue)),
              Optional(2 * MTestType::kGeneratedValue));
}

TEST(MVariableTest, GetUniqueValueShouldReturnNulloptByDefault) {
  EXPECT_EQ(GetUniqueValue(MInteger()), std::nullopt);
  EXPECT_EQ(GetUniqueValue(MTestType()), std::nullopt);
}

TEST(MVariableTest, GetUniqueValueWithMultipleOptionsShouldReturnNullopt) {
  EXPECT_EQ(GetUniqueValue(MInteger().IsOneOf({123, 456})), std::nullopt);
  EXPECT_EQ(
      GetUniqueValue(MTestType().IsOneOf(
          {MTestType::kGeneratedValue, MTestType::kGeneratedValueSmallSize})),
      std::nullopt);
}

TEST(MVariableTest, GenerateShouldRetryIfNeeded) {
  // 1/7 numbers should be 3 mod 7, and 1/2 number should have 3rd digit even.
  // A single random guess should work 1/14 times. We'll generate 100. If
  // retries aren't there, this will fail frequently.
  EXPECT_THAT(
      MInteger(Between(0, 999))
          .AddCustomConstraint("3 mod 7", [](int64_t x) { return x % 7 == 3; })
          .AddCustomConstraint("3rd digit is even.",
                               [](int64_t x) { return (x / 100) % 2 == 0; }),
      GeneratedValuesAre(
          AllOf(Truly([](int x) { return x % 7 == 3; }),
                Truly([](int x) { return (x / 100) % 2 == 0; }))));
}

TEST(MVariableTest, AssignValueShouldNotOverwriteAlreadySetValue) {
  moriarty_internal::VariableSet variables;
  MORIARTY_ASSERT_OK(
      variables.AddVariable("N", MInteger(Between(1, 1000000000))));
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MInteger(Between(1, "N"))));

  moriarty_internal::ValueSet values;

  moriarty_internal::GenerationConfig generation_config;
  moriarty_internal::RandomEngine engine({1, 2, 3}, "v0.1");

  MORIARTY_ASSERT_OK_AND_ASSIGN(moriarty_internal::AbstractVariable * var_A,
                                variables.GetAbstractVariable("A"));
  ResolverContext ctxA("A", variables, values, engine, generation_config);
  MORIARTY_ASSERT_OK(
      var_A->AssignValue(ctxA));  // By assigning A, we assigned N.
  MORIARTY_ASSERT_OK_AND_ASSIGN(int N, values.Get<MInteger>("N"));

  // Attempt to re-assign N.
  ResolverContext ctxN("N", variables, values, engine, generation_config);
  MORIARTY_ASSERT_OK_AND_ASSIGN(moriarty_internal::AbstractVariable * var_N,
                                variables.GetAbstractVariable("N"));
  MORIARTY_ASSERT_OK(var_N->AssignValue(ctxN));

  // Should not have changed.
  EXPECT_THAT(values.Get<MInteger>("N"), IsOkAndHolds(N));
}

}  // namespace
}  // namespace librarian
}  // namespace moriarty
