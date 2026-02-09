// Copyright 2026 Darcy Best
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

#include "src/variables/mvariant.h"

#include <cstdint>
#include <string>
#include <variant>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/constraints/base_constraints.h"
#include "src/constraints/container_constraints.h"
#include "src/constraints/numeric_constraints.h"
#include "src/constraints/string_constraints.h"
#include "src/librarian/errors.h"
#include "src/librarian/io_config.h"
#include "src/librarian/mvariable.h"
#include "src/librarian/testing/gtest_helpers.h"
#include "src/variables/minteger.h"
#include "src/variables/mstring.h"

namespace moriarty {
namespace {

using ::moriarty_testing::GeneratedValuesAre;
using ::moriarty_testing::GenerateSameValues;
using ::moriarty_testing::IsNotSatisfiedWith;
using ::moriarty_testing::IsSatisfiedWith;
using ::moriarty_testing::Read;
using ::moriarty_testing::Write;
using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::Each;
using ::testing::Ge;
using ::testing::Le;
using ::testing::SizeIs;
using ::testing::Truly;
using ::testing::VariantWith;

TEST(MVariantTest, TypenameIsCorrect) {
  EXPECT_EQ(MVariant(MInteger()).Typename(), "MVariant<MInteger>");
  EXPECT_EQ(MVariant(MInteger(), MString()).Typename(),
            "MVariant<MInteger, MString>");
  EXPECT_EQ(MVariant(MInteger(), MString(), MInteger()).Typename(),
            "MVariant<MInteger, MString, MInteger>");
}
TEST(MVariantTest, WriteShouldSucceed) {
  auto constraints = MVariant(MInteger(), MString());
  constraints.Format().Discriminator({"int", "str"});

  EXPECT_EQ(Write(constraints, 1), "int 1");
  EXPECT_EQ(Write(constraints, "hello"), "str hello");
}

TEST(MVariantTest, WriteWithProperSeparatorShouldSucceed) {
  auto newline_constraints = MVariant(MInteger(), MString());
  newline_constraints.Format().Discriminator({"int", "str"}).NewlineSeparated();

  auto space_constraints = MVariant(MInteger(), MString());
  space_constraints.Format().Discriminator({"int", "str"}).SpaceSeparated();

  EXPECT_EQ(Write(newline_constraints, 1), "int\n1");
  EXPECT_EQ(Write(space_constraints, "twotwo"), "str twotwo");
}

TEST(MVariantTest, TypicalReadCaseWorks) {
  auto constraints = MVariant<MInteger, MString>(
      MVariantFormat().Discriminator({"int", "str"}));

  EXPECT_THAT(Read(constraints, "int 22"), VariantWith<int64_t>(22));
  EXPECT_THAT(Read(constraints, "str twotwo"),
              VariantWith<std::string>("twotwo"));
}

TEST(MVariantTest, ReadWithProperSeparatorShouldSucceed) {
  auto newline_constraints = MVariant<MInteger, MString>(
      MVariantFormat().Discriminator({"int", "str"}).NewlineSeparated());

  auto tab_constraints =
      MVariant<MInteger, MString>(MVariantFormat()
                                      .Discriminator({"int", "str"})
                                      .WithSeparator(Whitespace::kTab));

  EXPECT_THAT(Read(newline_constraints, "int\n22"), VariantWith<int64_t>(22));
  EXPECT_THAT(Read(tab_constraints, "str\ttwotwo"),
              VariantWith<std::string>("twotwo"));
}

TEST(MVariantTest, ReadWithIncorrectSeparatorShouldFail) {
  auto tab_constraints =
      MVariant<MInteger, MString>(MVariantFormat()
                                      .Discriminator({"int", "str"})
                                      .WithSeparator(Whitespace::kTab));

  auto newline_constraints = MVariant<MInteger, MString>(
      MVariantFormat().Discriminator({"int", "str"}).NewlineSeparated());

  EXPECT_THROW((void)Read(tab_constraints, "int 22"), IOError);
  EXPECT_THROW((void)Read(newline_constraints, "str twotwo"), IOError);
}

TEST(MVariantTest, ReadingTheWrongTypeShouldFail) {
  auto constraints = MVariant<MInteger, MString>(
      MVariantFormat().Discriminator({"int", "str"}));

  EXPECT_THROW((void)Read(constraints, "int two"), IOError);
  EXPECT_THROW((void)Read(constraints, "str"), IOError);
  EXPECT_THROW((void)Read(constraints, "bad 5"), IOError);
}

TEST(MVariantTest, ReadWithMissingDiscriminatorShouldFail) {
  auto constraints = MVariant<MInteger, MString>(
      MVariantFormat().Discriminator({"int", "str"}));

  EXPECT_THROW((void)Read(constraints, "22"), IOError);
  EXPECT_THROW((void)Read(constraints, "twotwo"), IOError);
}

TEST(MVariantTest, ReadWithInvalidDiscriminatorShouldFail) {
  auto constraints = MVariant<MInteger, MString>(
      MVariantFormat().Discriminator({"int", "str"}));

  EXPECT_THROW((void)Read(constraints, "num 22"), IOError);
  EXPECT_THROW((void)Read(constraints, "string twotwo"), IOError);
}

TEST(MVariantTest, ReadWithIncorrectDiscriminatorShouldFail) {
  auto constraints = MVariant<MInteger, MString>(
      MVariantFormat().Discriminator({"int", "str"}));

  EXPECT_THROW((void)Read(constraints, "int twotwo"), IOError);
  EXPECT_THAT(Read(constraints, "str 22"), VariantWith<std::string>("22"));
}

TEST(MVariantTest, ReadWithMNoneShouldNotReadTheWhitespace) {
  auto constraints = MVariant<MNone, MInteger>(
      MVariantFormat().Discriminator({"none", "int"}));

  EXPECT_THAT(Read(constraints, "none"), VariantWith<NoType>(NoType{}));
  EXPECT_THAT(Read(constraints, "int 12"), VariantWith<int64_t>(12));
}

TEST(MVariantTest, SimpleGenerateCaseWorks) {
  auto constraints =
      MVariant(MInteger(Between(1, 10)), MString(Length(5), Alphabet("abc")));

  EXPECT_THAT(constraints, GeneratedValuesAre(AnyOf(
                               VariantWith<int64_t>(AllOf(Ge(1), Le(10))),
                               VariantWith<std::string>(AllOf(
                                   SizeIs(5), Each(AnyOf('a', 'b', 'c')))))));
}

TEST(MVariantTest, MergeFromCorrectlyMergesEachArgument) {
  MVariant a = MVariant(MInteger(Between(1, 10)),
                        MString(Length(Between(3, 8)), Alphabet("abc")));
  MVariant b = MVariant(MInteger(Between(5, 15)),
                        MString(Length(Between(5, 10)), Alphabet("abc")));
  MVariant c = MVariant(MInteger(Between(5, 10)),
                        MString(Length(Between(5, 8)), Alphabet("abc")));

  EXPECT_TRUE(GenerateSameValues(a.MergeFrom(b), c));
}

TEST(MVariantTest, ElementConstraintsMergeIndependentAlternatives) {
  MVariant a = MVariant(MInteger(Between(1, 10)),
                        MString(Length(Between(3, 8)), Alphabet("abc")));
  MVariant b = MVariant(MInteger(Between(5, 10)),
                        MString(Length(Between(3, 8)), Alphabet("abc")));
  MInteger restrict_a = MInteger(Between(5, 15));

  EXPECT_TRUE(
      GenerateSameValues(a.AddConstraint(Element<0, MInteger>(restrict_a)), b));
}

TEST(MVariantTest, IsSatisfiedWithWorksForValid) {
  auto constraints = MVariant(MInteger(Between(100, 111)), MString(Length(5)));

  EXPECT_THAT(constraints, IsSatisfiedWith(105));
  EXPECT_THAT(constraints, IsSatisfiedWith("abcde"));
}

TEST(MVariantTest, IsSatisfiedWithWorksForInvalid) {
  auto constraints = MVariant(MInteger(Between(100, 111)), MString(Length(5)));

  EXPECT_THAT(constraints, IsNotSatisfiedWith(0, "between"));
  EXPECT_THAT(constraints, IsNotSatisfiedWith("toolong", "length"));
}

TEST(MVariantTest, ExactlyAndOneOfCanAcceptANakedNonVariantType) {
  EXPECT_THAT((MVariant<MInteger, MString>(Exactly(11))),
              GeneratedValuesAre(VariantWith<int64_t>(11)));
  EXPECT_THAT((MVariant<MInteger, MString>(Exactly("hi"))),
              GeneratedValuesAre(VariantWith<std::string>("hi")));
  EXPECT_THAT(
      (MVariant<MInteger, MString>(OneOf({"hi", "hello"}))),
      GeneratedValuesAre(VariantWith<std::string>(AnyOf("hi", "hello"))));
  EXPECT_THAT((MVariant<MInteger, MString>(OneOf({1, 4}))),
              GeneratedValuesAre(VariantWith<int64_t>(AnyOf(1, 4))));
}

TEST(MVariantTest, ExactlyAndOneOfShouldGenerateAndValidate) {
  {
    EXPECT_THAT((MVariant<MInteger, MString>(Exactly(11))),
                GeneratedValuesAre(VariantWith<int64_t>(11)));

    EXPECT_THAT((MVariant<MInteger, MString>(
                    OneOf({std::variant<int64_t, std::string>{int64_t{11}}}))),
                GeneratedValuesAre(VariantWith<int64_t>(11)));

    EXPECT_THAT((MVariant<MInteger, MString>(OneOf(
                    {std::variant<int64_t, std::string>{int64_t{11}},
                     std::variant<int64_t, std::string>{std::string("hi")}}))),
                GeneratedValuesAre(AnyOf(VariantWith<int64_t>(11),
                                         VariantWith<std::string>("hi"))));
  }
  {
    EXPECT_THAT((MVariant<MInteger, MString>(
                    Exactly(std::variant<int64_t, std::string>{int64_t{11}}))),
                IsSatisfiedWith(11));
    EXPECT_THAT((MVariant<MInteger, MString>(
                    OneOf({std::variant<int64_t, std::string>{int64_t{11}}}))),
                IsSatisfiedWith(11));
    EXPECT_THAT((MVariant<MInteger, MString>(OneOf(
                    {std::variant<int64_t, std::string>{int64_t{11}},
                     std::variant<int64_t, std::string>{std::string("hi")}}))),
                AllOf(IsSatisfiedWith(11), IsSatisfiedWith("hi")));
  }
  {
    EXPECT_THAT((MVariant<MInteger, MString>(
                    Exactly(std::variant<int64_t, std::string>{int64_t{11}}))),
                IsNotSatisfiedWith(33, "exactly"));
    EXPECT_THAT((MVariant<MInteger, MString>(
                    OneOf({std::variant<int64_t, std::string>{int64_t{11}}}))),
                IsNotSatisfiedWith("nope", "one of"));
    EXPECT_THAT((MVariant<MInteger, MString>(OneOf(
                    {std::variant<int64_t, std::string>{int64_t{11}},
                     std::variant<int64_t, std::string>{std::string("hi")}}))),
                IsNotSatisfiedWith("nope", "one of"));
  }
}

}  // namespace
}  // namespace moriarty
