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

#include "src/variables/mtuple.h"

#include <cstdint>
#include <tuple>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/librarian/errors.h"
#include "src/librarian/mvariable.h"
#include "src/testing/gtest_helpers.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/constraints/container_constraints.h"
#include "src/variables/constraints/io_constraints.h"
#include "src/variables/constraints/numeric_constraints.h"
#include "src/variables/minteger.h"
#include "src/variables/mstring.h"

namespace moriarty {
namespace {

using ::moriarty_testing::GeneratedValuesAre;
using ::moriarty_testing::GenerateSameValues;
using ::moriarty_testing::IsNotSatisfiedWith;
using ::moriarty_testing::IsSatisfiedWith;
using ::moriarty_testing::Print;
using ::moriarty_testing::Read;
using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::FieldsAre;
using ::testing::Ge;
using ::testing::Le;
using ::testing::Throws;

TEST(MTupleTest, TypenameIsCorrect) {
  EXPECT_EQ(MTuple(MInteger()).Typename(), "MTuple<MInteger>");
  EXPECT_EQ(MTuple(MInteger(), MInteger()).Typename(),
            "MTuple<MInteger, MInteger>");
  EXPECT_EQ(MTuple(MInteger(), MString(), MInteger()).Typename(),
            "MTuple<MInteger, MString, MInteger>");
  EXPECT_EQ(MTuple(MInteger(), MTuple(MInteger(), MInteger())).Typename(),
            "MTuple<MInteger, MTuple<MInteger, MInteger>>");
}
TEST(MTupleTest, PrintShouldSucceed) {
  EXPECT_EQ(Print(MTuple(MInteger(), MInteger()), {1, 2}), "1 2");
  EXPECT_EQ(
      Print(MTuple(MInteger(), MInteger(), MTuple(MInteger(), MInteger())),
            {1, 2, {5, 6}}),
      "1 2 5 6");
}

TEST(MTupleTest, PrintWithProperSeparatorShouldSucceed) {
  EXPECT_EQ(Print(MTuple<MInteger, MInteger, MInteger>(IOSeparator::Newline()),
                  {1, 22, 333}),
            "1\n22\n333");
  EXPECT_EQ(Print(MTuple<MInteger, MString, MInteger>(IOSeparator::Tab()),
                  {1, "twotwo", 333}),
            "1\ttwotwo\t333");
}

TEST(MTupleTest, TypicalReadCaseWorks) {
  EXPECT_EQ(Read(MTuple<MInteger, MInteger, MInteger>(), "1 22 333"),
            std::make_tuple(1, 22, 333));
  EXPECT_EQ(Read(MTuple<MInteger, MString, MInteger>(), "1 twotwo 333"),
            std::make_tuple(1, "twotwo", 333));
}

TEST(MTupleTest, ReadWithProperSeparatorShouldSucceed) {
  EXPECT_EQ(Read(MTuple<MInteger, MInteger, MInteger>(IOSeparator::Newline()),
                 "1\n22\n333"),
            std::make_tuple(1, 22, 333));
  EXPECT_EQ(Read(MTuple<MInteger, MString, MInteger>(IOSeparator::Tab()),
                 "1\ttwotwo\t333"),
            std::make_tuple(1, "twotwo", 333));
}

TEST(MTupleTest, ReadWithIncorrectSeparatorShouldFail) {
  EXPECT_THROW((void)Read(MTuple<MInteger, MInteger, MInteger>(), "1\t22\t333"),
               IOError);
  EXPECT_THROW(
      (void)Read(MTuple<MInteger, MInteger, MInteger>(IOSeparator::Newline()),
                 "1 22 333"),
      IOError);
  EXPECT_THROW(
      (void)Read(MTuple<MInteger, MString, MInteger>(IOSeparator::Tab()),
                 "1\ttwotwo 333"),
      IOError);
}

TEST(MTupleTest, ReadingTheWrongTypeShouldFail) {
  // MString where MInteger should be
  EXPECT_THROW((void)Read(MTuple<MInteger, MInteger, MInteger>(), "1 two 3"),
               IOError);
  // Not enough input
  EXPECT_THROW((void)Read(MTuple<MInteger, MInteger, MInteger>(), "1 22"),
               IOError);
}

TEST(MTupleTest, SimpleGenerateCaseWorks) {
  EXPECT_THAT(MTuple(MInteger(Between(1, 10)), MInteger(Between(30, 40))),
              GeneratedValuesAre(
                  FieldsAre(AllOf(Ge(1), Le(10)), AllOf(Ge(30), Le(40)))));

  EXPECT_THAT(
      MTuple(MInteger(Between(0, 9)),
             MTuple(MInteger(Between(10, 99)), MInteger(Between(100, 999)))),
      GeneratedValuesAre(FieldsAre(
          AllOf(Ge(0), Le(9)),
          FieldsAre(AllOf(Ge(10), Le(99)), AllOf(Ge(100), Le(999))))));
}

TEST(MTupleTest, MergeFromCorrectlyMergesEachArgument) {
  {
    MTuple a = MTuple(MInteger(Between(1, 10)), MInteger(Between(20, 30)));
    MTuple b = MTuple(MInteger(Between(5, 15)), MInteger(Between(22, 25)));
    MTuple c = MTuple(MInteger(Between(5, 10)), MInteger(Between(22, 25)));

    EXPECT_TRUE(GenerateSameValues(a.MergeFrom(b), c));
  }

  {
    MTuple a =
        MTuple(MInteger(Between(1, 10)),
               MTuple(MInteger(Between(20, 30)), MInteger(Between(40, 50))));
    MTuple b =
        MTuple(MInteger(Between(5, 15)),
               MTuple(MInteger(Between(22, 25)), MInteger(Between(34, 45))));
    MTuple c =
        MTuple(MInteger(Between(5, 10)),
               MTuple(MInteger(Between(22, 25)), MInteger(Between(40, 45))));

    EXPECT_TRUE(GenerateSameValues(a.MergeFrom(b), c));
  }
}

TEST(MTupleTest, OfShouldMergeIndependentArguments) {
  {
    MTuple a = MTuple(MInteger(Between(1, 10)), MInteger(Between(20, 30)));
    MTuple b = MTuple(MInteger(Between(5, 10)), MInteger(Between(20, 30)));
    MInteger restrict_a = MInteger(Between(5, 15));

    EXPECT_TRUE(GenerateSameValues(
        a.AddConstraint(Element<0, MInteger>(restrict_a)), b));
  }

  {
    MTuple a =
        MTuple(MInteger(Between(1, 10)),
               MTuple(MInteger(Between(20, 30)), MInteger(Between(40, 50))));
    MTuple b =
        MTuple(MInteger(Between(1, 10)),
               MTuple(MInteger(Between(22, 30)), MInteger(Between(40, 50))));
    MInteger restrict_a = MInteger(Between(22, 40));

    EXPECT_TRUE(GenerateSameValues(
        a.AddConstraint(Element<1, MTuple<MInteger, MInteger>>(
            MTuple(restrict_a, MInteger()))),
        b));
  }
}

TEST(MTupleTest, IsSatisfiedWithWorksForValid) {
  {  // Simple
    MTuple constraints =
        MTuple(MInteger(Between(100, 111)), MInteger(Between(200, 222)));
    EXPECT_THAT(constraints,
                IsSatisfiedWith(std::tuple<int64_t, int64_t>({105, 205})));
  }

  {  // Nested [1, [2, 3]]
    MTuple constraints = MTuple(
        MInteger(Between(100, 111)),
        MTuple(MInteger(Between(200, 222)), MInteger(Between(300, 333))));
    using TupleType = std::tuple<int64_t, std::tuple<int64_t, int64_t>>;
    EXPECT_THAT(constraints, IsSatisfiedWith(TupleType({105, {205, 305}})));
  }

  {  // Very nested [1, [[2, 3], [4, 5, 6]]]
    MTuple constraints = MTuple(
        MInteger(Between(100, 111)),
        MTuple(MTuple(MInteger(Between(200, 222)), MInteger(Between(300, 333))),
               MTuple(MInteger(Between(400, 444)), MInteger(Between(500, 555)),
                      MInteger(Between(600, 666)))));
    using TupleType =
        std::tuple<int64_t, std::tuple<std::tuple<int64_t, int64_t>,
                                       std::tuple<int64_t, int64_t, int64_t>>>;
    EXPECT_THAT(constraints,
                IsSatisfiedWith(TupleType{105, {{205, 305}, {405, 505, 605}}}));
  }
}

TEST(MTupleTest, IsSatisfiedWithWorksForInvalid) {
  {  // Simple
    MTuple constraints =
        MTuple(MInteger(Between(100, 111)), MInteger(Between(200, 222)));
    using Type = std::tuple<int64_t, int64_t>;

    EXPECT_THAT(constraints, IsNotSatisfiedWith(Type{0, 205},
                                                "between"));  // first
    EXPECT_THAT(constraints,
                IsNotSatisfiedWith(Type{105, 0}, "between"));  // second
    EXPECT_THAT(constraints,
                IsNotSatisfiedWith(Type{0, 0}, "between"));  // both
  }

  {  // Nested [1, [2, 3]]
    MTuple constraints = MTuple(
        MInteger(Between(100, 111)),
        MTuple(MInteger(Between(200, 222)), MInteger(Between(300, 333))));
    using Type = std::tuple<int64_t, std::tuple<int64_t, int64_t>>;

    EXPECT_THAT(constraints,
                IsNotSatisfiedWith(Type{0, {205, 305}}, "between"));  // first
    EXPECT_THAT(constraints, IsNotSatisfiedWith(Type{105, {0, 305}},
                                                "between"));  // second.first
    EXPECT_THAT(constraints, IsNotSatisfiedWith(Type{105, {205, 0}},
                                                "between"));  // second.Second
    EXPECT_THAT(constraints, IsNotSatisfiedWith(Type{105, {0, 0}},
                                                "between"));  // second.both
  }
}

TEST(MTupleTest, MultipleIOSeparatorsShouldThrow) {
  // These lambdas are out-of-line because EXPECT_THAT macro doesn't like the
  // <,>.
  auto a = []() {
    return MTuple<MInteger, MInteger>(IOSeparator::Newline(),
                                      IOSeparator::Tab());
  };
  auto b = []() {
    return MTuple<MInteger, MInteger>(IOSeparator::Newline(),
                                      IOSeparator::Space());
  };

  EXPECT_THAT(a, Throws<ImpossibleToSatisfy>());
  EXPECT_THAT(b, Throws<ImpossibleToSatisfy>());
}

TEST(MTupleTest, ExactlyAndOneOfShouldGenerateAndValidate) {
  using tup = std::tuple<int64_t, int64_t>;
  {
    EXPECT_THAT((MTuple<MInteger, MInteger>(Exactly(tup{11, 22}))),
                GeneratedValuesAre(FieldsAre(11, 22)));

    EXPECT_THAT((MTuple<MInteger, MInteger>(OneOf({tup{11, 22}}))),
                GeneratedValuesAre(FieldsAre(11, 22)));

    EXPECT_THAT(
        (MTuple<MInteger, MInteger>(OneOf({tup{11, 22}, tup{33, 44}}))),
        GeneratedValuesAre(AnyOf(FieldsAre(11, 22), FieldsAre(33, 44))));
  }
  {
    EXPECT_THAT((MTuple<MInteger, MInteger>(Exactly(tup{11, 22}))),
                IsSatisfiedWith(tup{11, 22}));
    EXPECT_THAT((MTuple<MInteger, MInteger>(OneOf({tup{11, 22}}))),
                IsSatisfiedWith(tup{11, 22}));
    EXPECT_THAT(
        (MTuple<MInteger, MInteger>(OneOf({tup{11, 22}, tup{33, 44}}))),
        AllOf(IsSatisfiedWith(tup{11, 22}), IsSatisfiedWith(tup{33, 44})));
  }
  {
    EXPECT_THAT((MTuple<MInteger, MInteger>(Exactly(tup{11, 22}))),
                IsNotSatisfiedWith(tup{33, 44}, "exactly"));
    EXPECT_THAT((MTuple<MInteger, MInteger>(OneOf({tup{11, 22}}))),
                IsNotSatisfiedWith(tup{11, 33}, "one of"));
    EXPECT_THAT((MTuple<MInteger, MInteger>(OneOf({tup{11, 22}, tup{33, 44}}))),
                IsNotSatisfiedWith(tup{11, 44}, "one of"));
  }
}

}  // namespace
}  // namespace moriarty
