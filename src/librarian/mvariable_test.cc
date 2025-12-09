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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/contexts/librarian_context.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/generation_handler.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/testing/gtest_helpers.h"
#include "src/librarian/testing/mtest_type.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace librarian {

class MEmptyClass;
class EmptyClass;
template <>
struct MVariableValueTypeTrait<MEmptyClass> {
  using type = EmptyClass;
};

class EmptyClass {
 public:
  bool operator==(const EmptyClass& other) const { return true; }
  bool operator<(const EmptyClass& other) const { return false; }
};

class MEmptyClass : public MVariable<MEmptyClass> {
 public:
  std::string Typename() const override { return "MEmptyClass"; }

 private:
  EmptyClass GenerateImpl(ResolverContext) const override {
    throw std::runtime_error("Unimplemented: GenerateImpl");
  }
};

namespace {

using ::moriarty::moriarty_internal::AbstractVariable;
using ::moriarty::moriarty_internal::ValueSet;
using ::moriarty::moriarty_internal::VariableSet;
using ::moriarty_testing::Context;
using ::moriarty_testing::Generate;
using ::moriarty_testing::GeneratedValuesAre;
using ::moriarty_testing::GenerateThrowsGenerationError;
using ::moriarty_testing::GenerateThrowsNotFoundError;
using ::moriarty_testing::GetUniqueValue;
using ::moriarty_testing::IsNotSatisfiedWith;
using ::moriarty_testing::IsSatisfiedWith;
using ::moriarty_testing::LastDigit;
using ::moriarty_testing::MTestType;
using ::moriarty_testing::NumberOfDigits;
using ::moriarty_testing::Print;
using ::moriarty_testing::Read;
using ::moriarty_testing::TestType;
using ::moriarty_testing::ThrowsImpossibleToSatisfy;
using ::moriarty_testing::ThrowsMVariableTypeMismatch;
using ::moriarty_testing::ThrowsValueNotFound;
using ::moriarty_testing::ThrowsVariableNotFound;
using ::testing::_;
using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::ElementsAre;
using ::testing::Optional;
using ::testing::Truly;

class MissingValueType {
  static constexpr bool is_moriarty_variable = true;
};
class MissingBoolean {
  using value_type = int;
};
TEST(MVariableTest, MoriartyVariableConceptShouldWork) {
  static_assert(!MoriartyVariable<moriarty_internal::AbstractVariable>);
  static_assert(MoriartyVariable<MTestType>);
  static_assert(!MoriartyVariable<TestType>);
  static_assert(!MoriartyVariable<int>);
  static_assert(!MoriartyVariable<MissingValueType>);
  static_assert(!MoriartyVariable<MissingBoolean>);
}

TEST(MVariableTest, PrintShouldSucceed) {
  EXPECT_EQ(Print(MTestType(), -1), "-1");
  EXPECT_EQ(Print(MTestType(), 153), "153");
}

TEST(MVariableTest, GenerateShouldProduceAValue) {
  EXPECT_THAT(MTestType(),
              GeneratedValuesAre(AnyOf(123456789, 23456789, 3456789, 456789,
                                       56789, 6789, 789, 89, 9)));
}

TEST(MVariableTest, GenerateShouldObserveExactly) {
  EXPECT_THAT(MTestType(Exactly<TestType>(10)), GeneratedValuesAre(10));
}

TEST(MVariableTest, GenerateShouldObserveIsOneOf) {
  EXPECT_THAT(
      MTestType(OneOf<TestType>({50, 60, 70, 80, 90, 100})),
      GeneratedValuesAre(AnyOf(TestType(50), TestType(60), TestType(70),
                               TestType(80), TestType(90), TestType(100))));
}

TEST(MVariableTest, MergeFromShouldRespectOneOf) {
  MTestType var1 =
      MTestType(OneOf<TestType>({TestType(11), TestType(22), TestType(33)}));
  MTestType var2 =
      MTestType(OneOf<TestType>({TestType(22), TestType(33), TestType(44)}));

  ASSERT_THAT(var1, GeneratedValuesAre(
                        AnyOf(TestType(11), TestType(22), TestType(33))));

  var1.MergeFrom(var2);
  EXPECT_THAT(var1, GeneratedValuesAre(AnyOf(TestType(22), TestType(33))));
}

TEST(MVariableTest, MergeFromWithWrongTypeShouldFail) {
  MTestType var1;
  moriarty::MInteger var2;

  EXPECT_THAT([&] { (void)var1.MergeFromAnonymous(var2); },
              ThrowsMVariableTypeMismatch("MInteger", "MTestType"));
}

TEST(MVariableTest, MergeFromShouldWork) {
  {
    MTestType var1 = MTestType(Exactly<TestType>(10));
    MTestType var2;
    var1.MergeFrom(var2);
    EXPECT_EQ(Generate(var1), 10);
  }
  {
    MTestType var1;
    MTestType var2 = MTestType(Exactly<TestType>(20));
    var1.MergeFrom(var2);
    EXPECT_EQ(Generate(var1), 20);
  }
  {
    MTestType var1 = MTestType(Exactly<TestType>(10));
    MTestType var2 = MTestType(Exactly<TestType>(20));
    // Can't be both 10 and 20.
    EXPECT_THAT([&] { var1.MergeFrom(var2); },
                ThrowsImpossibleToSatisfy("exactly"));
  }
  {
    MTestType var1 = MTestType(Exactly<TestType>(10));
    MTestType var2 = MTestType(OneOf<TestType>({40, 30, 20, 10}));
    var1.MergeFrom(var2);
    EXPECT_EQ(Generate(var1), 10);
  }
  {
    MTestType var1 = MTestType(OneOf<TestType>({300, 17, 10, -1234}));
    MTestType var2 = MTestType(OneOf<TestType>({40, 30, 20, 10}));
    var1.MergeFrom(var2);
    EXPECT_EQ(Generate(var1), 10);
  }
  {
    MTestType var1;
    MInteger var2;
    EXPECT_THAT(
        [&] { static_cast<AbstractVariable&>(var1).MergeFromAnonymous(var2); },
        ThrowsMVariableTypeMismatch("MInteger", "MTestType"));
  }
}

TEST(MVariableTest, SubvariablesShouldBeSetableAndUseable) {
  EXPECT_EQ(Generate(MTestType(LastDigit(MInteger(Between(3, 3))),
                               NumberOfDigits(MInteger(Exactly(9))))),
            123456783);
}

TEST(MVariableTest, BasingMyVariableOnAnotherSetValueShouldWorkBasicCase) {
  EXPECT_EQ(Generate(MTestType(LastDigit(MInteger(Exactly("x"))),
                               NumberOfDigits(MInteger(Exactly(9)))),
                     Context().WithValue<MInteger>("x", 4)),
            123456784);
}

TEST(MVariableTest, BasingMyVariableOnAnotherUnSetVariableShouldWorkBasicCase) {
  // Doesn't matter what the value is.
  EXPECT_THAT(MTestType(LastDigit(MInteger(Exactly("x")))),
              GeneratedValuesAre(
                  _, Context().WithVariable("x", MInteger(Between(1, 9)))));
}

TEST(MVariableTest, BasingMyVariableOnANonexistentOneShouldFail) {
  EXPECT_THROW(
      { (void)Generate(MTestType(LastDigit(MInteger(Exactly("x"))))); },
      std::runtime_error);
}

TEST(MVariableTest, DependentVariableOfTheWrongTypeShouldFail) {
  EXPECT_THAT(
      [] {
        (void)Generate(MTestType(LastDigit(MInteger(Exactly("x")))),
                       Context().WithVariable("x", MTestType()));
      },
      ThrowsMVariableTypeMismatch("MTestType", "MInteger"));
}

TEST(MVariableTest, DependentVariablesInSubvariablesCanChain) {
  Context c = Context()
                  .WithVariable("y", MInteger(Exactly("N + 1")))
                  .WithVariable("N", MInteger(Exactly(5)));

  EXPECT_THAT(MTestType(LastDigit(MInteger(Exactly("y")))),
              GeneratedValuesAre(AnyOf(123456786, 23456786, 3456786, 456786,
                                       56786, 6786, 786, 86, 6),
                                 c));
}

TEST(MVariableTest, SeparateCallsToGetShouldUseTheSameDependentVariableValue) {
  Context context = Context()
                        .WithVariable("A", MInteger(Between("N", "N")))
                        .WithVariable("B", MInteger(Between("N", "N")))
                        .WithVariable("N", MInteger(Between(1, 1000000000)));

  moriarty_internal::GenerationHandler generation_handler;

  AbstractVariable* var_A = context.Variables().GetAnonymousVariable("A");
  var_A->AssignValue("A", context.Variables(), context.Values(),
                     context.RandomEngine(),
                     generation_handler);  // By assigning A, we assigned N.

  AbstractVariable* var_B = context.Variables().GetAnonymousVariable("B");
  var_B->AssignValue(
      "B", context.Variables(), context.Values(), context.RandomEngine(),
      generation_handler);  // Should use the already generated N.
  int N = context.Values().Get<MInteger>("N");

  EXPECT_EQ(context.Values().Get<MInteger>("A"), N);
  EXPECT_EQ(context.Values().Get<MInteger>("B"), N);
}

TEST(MVariableTest, CyclicDependenciesShouldFail) {
  EXPECT_THROW(
      {
        (void)Generate(MInteger(AtLeast("x")),
                       Context()
                           .WithVariable("y", MInteger(Between("z", 4)))
                           .WithVariable("z", MInteger(Between("y", 4))));
      },
      std::runtime_error);

  // Self-loop
  EXPECT_THROW(
      {
        (void)Generate(MInteger(Exactly("x + 1")),
                       Context().WithVariable("x", MInteger(AtLeast("x"))));
      },
      std::runtime_error);
}

TEST(MVariableTest, IsSatisfiedWithWorksForValid) {
  // In the simplist case, everything will work.
  EXPECT_THAT(MTestType(), IsSatisfiedWith(5));
  EXPECT_THAT(MTestType(), IsSatisfiedWith(3453));

  EXPECT_THAT(MTestType(LastDigit(MInteger(Exactly(4)))),
              IsSatisfiedWith(134534));
  EXPECT_THAT(MTestType(NumberOfDigits(MInteger(Exactly(3)))),
              IsSatisfiedWith(123));

  EXPECT_THAT(MTestType(LastDigit(MInteger(Between(3, 7)))),
              IsNotSatisfiedWith(101, "last"));
  EXPECT_THAT(MTestType(NumberOfDigits(MInteger(Between(3, 7)))),
              IsNotSatisfiedWith(21, "digits"));
}

TEST(MVariableTest, IsSatisfiedWithNeedsDependentValues) {
  {  // No value or variable known
    Context context;
    librarian::AnalysisContext ctx("test", context.Variables(),
                                   context.Values());

    EXPECT_THAT(
        [&] {
          (void)MTestType(NumberOfDigits(MInteger(Exactly("t"))))
              .CheckValue(ctx, 1);
        },
        ThrowsVariableNotFound("t"));
  }
  {  // Variable known, but no value known.
    Context context = Context().WithVariable("t", MInteger());
    librarian::AnalysisContext ctx("test", context.Variables(),
                                   context.Values());

    EXPECT_THAT(
        [&] {
          (void)MTestType(NumberOfDigits(MInteger(Exactly("t"))))
              .CheckValue(ctx, 1);
        },
        ThrowsValueNotFound("t"));
  }
}

TEST(MVariableTest, IsSatisfiedWithWorksWithDependentValues) {
  EXPECT_THAT(MTestType(NumberOfDigits(MInteger(Exactly("t")))),
              IsSatisfiedWith(110, Context().WithValue<MInteger>("t", 3)));
  EXPECT_THAT(MTestType(LastDigit(MInteger(Exactly("t")))),
              IsSatisfiedWith(5542, Context().WithValue<MInteger>("t", 2)));
}

TEST(MVariableTest, IsSatisfiedWithCanValidateSubvariablesIfNeeded) {
  EXPECT_THAT(MTestType(NumberOfDigits(MInteger(AtLeast(5), AtMost(3)))),
              IsNotSatisfiedWith(1, "digits"));
  EXPECT_THAT(MTestType(NumberOfDigits(MInteger(AtLeast(5), AtMost(6)))),
              IsNotSatisfiedWith(1, "digits"));
}

TEST(MVariableTest, IsSatisfiedWithShouldAcknowledgeExactlyAndOneOf) {
  EXPECT_THAT(MTestType(OneOf<TestType>({2, 3, 5, 7})), IsSatisfiedWith(5));
  EXPECT_THAT(MTestType(OneOf<TestType>({2, 4, 8})),
              IsNotSatisfiedWith(5, "one of"));

  EXPECT_THAT(MTestType()
                  .AddConstraint(OneOf<TestType>({2, 4, 8}))
                  .AddConstraint(LastDigit(MInteger(Between(4, 4)))),
              IsSatisfiedWith(4));
}

TEST(MVariableTest, IsSatisfiedWithShouldFailIfIsOneOfSucceedsButOtherFails) {
  EXPECT_THAT(MTestType()
                  .AddConstraint(OneOf<TestType>({2, 4, 8}))
                  .AddConstraint(LastDigit(MInteger(Between(4, 4)))),
              IsNotSatisfiedWith(8, "last digit"));
}

TEST(MVariableTest, IsSatisfiedWithShouldCheckCustomConstraints) {
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

TEST(MVariableTest, IsSatisfiedWithShouldCheckMultipleCustomConstraints) {
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

bool SameAsA(AnalysisContext ctx, const TestType& value) {
  TestType A = ctx.GetValue<MTestType>("A");
  return A.value == value;
}

bool SameAsL(AnalysisContext ctx, const TestType& value) {
  TestType L = ctx.GetValue<MTestType>("L");
  return L.value == value;
}

TEST(MVariableTest, CustomConstraintWithDependentVariablesShouldWork) {
  EXPECT_THAT(MTestType()
                  .AddConstraint(LastDigit(MInteger(Exactly(2))))
                  .AddCustomConstraint("Custom1", {"A"}, SameAsA),
              GeneratedValuesAre(
                  AnyOf(2, 12, 22, 32, 42, 52, 62, 72, 82, 92),
                  Context().WithVariable(
                      "A", MTestType(LastDigit(MInteger(Exactly(2))),
                                     NumberOfDigits(MInteger(AtMost(2)))))));
}

TEST(MVariableTest, UnsatisfiableCustomConstraintShouldThrow) {
  EXPECT_THAT(MTestType()
                  .AddConstraint(LastDigit(MInteger(Exactly(1))))
                  .AddCustomConstraint("Custom1", {"A"}, SameAsA),
              GenerateThrowsGenerationError(
                  "", Context().WithVariable(
                          "A", MTestType(LastDigit(MInteger(Exactly(2)))))));
}

TEST(MVariableTest, CustomConstraintsWithWrongDependentVariableShouldThrow) {
  EXPECT_THAT(MTestType().AddCustomConstraint("Custom1", {}, SameAsA),
              GenerateThrowsNotFoundError("A", Context()));
}

TEST(MVariableTest,
     CustomConstraintsKnownVariableButNotAddedAsDependencyFailsGeneration) {
  // TODO: We might want to enforce that they cannot call the dependent
  // variable, even if they get lucky.

  // Note that "L"'s value is not known since the generated variable's name is
  // "Generate(MTestType)" -- which comes before "L" alphabetically. If the
  // dependent's name was "A" instead of "L", this would pass, but it is not
  // guaranteed to since the user didn't specify it as a constraint.
  EXPECT_THAT(MTestType().AddCustomConstraint("Custom1", {}, SameAsL),
              GenerateThrowsNotFoundError(
                  "L", Context().WithVariable(
                           "L", MTestType(LastDigit(MInteger(Exactly(2)))))));
}

// TODO(hivini): Test that by default, nothing is returned from
// ListEdgeCases.

// TODO: Test that ListEdgeCases returns values which have been merged with the
// existing constraints.
TEST(MVariableTest, ListEdgeCasesReturnsMVariablesThatCanBeGenerated) {
  VariableSet variables;
  ValueSet values;
  AnalysisContext ctx("test", variables, values);
  EXPECT_THAT(MTestType().ListEdgeCases(ctx),
              ElementsAre(GeneratedValuesAre(2), GeneratedValuesAre(3)));
}

TEST(MVariableTest, MVariableShouldByDefaultNotBeAbleToRead) {
  EXPECT_THROW({ (void)Read(MEmptyClass(), "1234"); }, std::runtime_error);
}

TEST(MVariableTest, MVariableShouldByDefaultNotBeAbleToPrint) {
  EXPECT_THROW(
      { (void)Print(MEmptyClass(), EmptyClass()); }, std::runtime_error);
}

TEST(MVariableTest, ReadValueShouldBeSuccessfulInNormalState) {
  EXPECT_EQ(Read(MTestType(), "1234"), 1234);
}

TEST(MVariableTest, FailedReadShouldFail) {
  EXPECT_THROW({ (void)Read(MTestType(), "bad"); }, std::runtime_error);
}

TEST(MVariableTest, PrintValueShouldBeSuccessfulInNormalState) {
  EXPECT_EQ(Print(MTestType(), TestType(1234)), "1234");
}

TEST(MVariableTest, PrintValueShouldPrintTheAssignedValue) {
  Context context = Context()
                        .WithVariable("x", MTestType())
                        .WithValue<MTestType>("x", TestType(12345));

  std::stringstream ss;

  AbstractVariable* var = context.Variables().GetAnonymousVariable("x");
  var->PrintValue("x", ss, context.Variables(), context.Values());
  EXPECT_EQ(ss.str(), "12345");
}

TEST(MVariableTest, GetUniqueValueShouldReturnUniqueValueViaIsMethod) {
  EXPECT_THAT(GetUniqueValue(MInteger(Exactly(123))), Optional(123));
  EXPECT_THAT(GetUniqueValue(
                  MTestType(Exactly<TestType>(2 * MTestType::kGeneratedValue))),
              Optional(2 * MTestType::kGeneratedValue));
}

TEST(MVariableTest, GetUniqueValueShouldReturnNulloptByDefault) {
  EXPECT_EQ(GetUniqueValue(MInteger()), std::nullopt);
  EXPECT_EQ(GetUniqueValue(MTestType()), std::nullopt);
}

TEST(MVariableTest, GetUniqueValueWithMultipleOptionsShouldReturnNullopt) {
  EXPECT_EQ(GetUniqueValue(MInteger(OneOf({123, 456}))), std::nullopt);
  EXPECT_EQ(
      GetUniqueValue(MTestType(OneOf<TestType>(
          {MTestType::kGeneratedValue, MTestType::kGeneratedValueSmallSize}))),
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
  Context context = Context()
                        .WithVariable("N", MInteger(Between(1, 1000000000)))
                        .WithVariable("A", MInteger(Between(1, "N")));

  moriarty_internal::GenerationHandler generation_handler;

  AbstractVariable* var_A = context.Variables().GetAnonymousVariable("A");
  var_A->AssignValue("A", context.Variables(), context.Values(),
                     context.RandomEngine(),
                     generation_handler);  // By assigning A, we assigned N.
  int N = context.Values().Get<MInteger>("N");

  // Attempt to re-assign N.
  AbstractVariable* var_N = context.Variables().GetAnonymousVariable("N");
  var_N->AssignValue("N", context.Variables(), context.Values(),
                     context.RandomEngine(), generation_handler);

  // Should not have changed.
  EXPECT_EQ(context.Values().Get<MInteger>("N"), N);
}

}  // namespace
}  // namespace librarian
}  // namespace moriarty
