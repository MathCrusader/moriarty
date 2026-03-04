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

#include "src/constraints/string_constraints.h"

#include <unordered_set>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/context.h"
#include "src/librarian/testing/gtest_helpers.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace {

using ::moriarty::ConstraintContext;
using ::moriarty_testing::Context;
using ::moriarty_testing::HasNoViolation;
using ::moriarty_testing::HasViolation;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAre;

MATCHER(HasDuplicateLetter,
        negation ? "has no duplicate letters" : "has duplicate letters") {
  std::unordered_set<char> seen;
  for (char c : arg) {
    auto [it, inserted] = seen.insert(c);
    if (!inserted) {
      *result_listener << "duplicate letter = " << c;
      return true;
    }
  }
  return false;
}

// These tests below are simply safety checks to ensure the alphabets are
// correctly typed and not accidentally modified later.
TEST(AlphabetTest, CommonAlphabetsDoNotHaveDuplicatedLetters) {
  EXPECT_THAT(Alphabet::Letters().GetAlphabet(), Not(HasDuplicateLetter()));
  EXPECT_THAT(Alphabet::Uppercase().GetAlphabet(), Not(HasDuplicateLetter()));
  EXPECT_THAT(Alphabet::Lowercase().GetAlphabet(), Not(HasDuplicateLetter()));
  EXPECT_THAT(Alphabet::Numbers().GetAlphabet(), Not(HasDuplicateLetter()));
  EXPECT_THAT(Alphabet::Alphanumeric().GetAlphabet(),
              Not(HasDuplicateLetter()));
  EXPECT_THAT(Alphabet::UpperAlphanumeric().GetAlphabet(),
              Not(HasDuplicateLetter()));
  EXPECT_THAT(Alphabet::LowerAlphanumeric().GetAlphabet(),
              Not(HasDuplicateLetter()));
}

TEST(AlphabetTest, CommonAlphabetsHaveTheAppropriateNumberOfElements) {
  EXPECT_THAT(Alphabet::Letters().GetAlphabet(), SizeIs(26 + 26));
  EXPECT_THAT(Alphabet::Uppercase().GetAlphabet(), SizeIs(26));
  EXPECT_THAT(Alphabet::Lowercase().GetAlphabet(), SizeIs(26));
  EXPECT_THAT(Alphabet::Numbers().GetAlphabet(), SizeIs(10));
  EXPECT_THAT(Alphabet::Alphanumeric().GetAlphabet(), SizeIs(26 + 26 + 10));
  EXPECT_THAT(Alphabet::UpperAlphanumeric().GetAlphabet(), SizeIs(26 + 10));
  EXPECT_THAT(Alphabet::LowerAlphanumeric().GetAlphabet(), SizeIs(26 + 10));
}

TEST(AlphabetTest, BasicConstructorShouldReturnExactAlphabet) {
  EXPECT_EQ(Alphabet("abc").GetAlphabet(), "abc");
  EXPECT_EQ(Alphabet("AbC").GetAlphabet(), "AbC");
  EXPECT_EQ(Alphabet("A\tC").GetAlphabet(), "A\tC");
  EXPECT_EQ(Alphabet("AAA").GetAlphabet(), "AAA");
}

TEST(AlphabetTest, IsSatisfiedWithShouldWork) {
  Context context;
  ConstraintContext ctx("test", context.Variables(), context.Values());

  EXPECT_THAT(Alphabet("abc").Validate(ctx, "a"), HasNoViolation());
  EXPECT_THAT(Alphabet("abc").Validate(ctx, "A"),
              HasViolation("one of \"abc\""));

  {
    EXPECT_THAT(Alphabet::Alphanumeric().Validate(ctx, "b"), HasNoViolation());
    EXPECT_THAT(Alphabet::Alphanumeric().Validate(ctx, "B"), HasNoViolation());
    EXPECT_THAT(Alphabet::Alphanumeric().Validate(ctx, "3"), HasNoViolation());
    EXPECT_THAT(
        Alphabet::Alphanumeric().Validate(ctx, "%"),
        HasViolation("one of \"AB"));  // Note I'm only checking the start
  }
  {
    EXPECT_THAT(Alphabet::Letters().Validate(ctx, "b"), HasNoViolation());
    EXPECT_THAT(Alphabet::Letters().Validate(ctx, "B"), HasNoViolation());
    EXPECT_THAT(Alphabet::Letters().Validate(ctx, "3"),
                HasViolation("one of \"AB"));
    EXPECT_THAT(Alphabet::Letters().Validate(ctx, "%"),
                HasViolation("one of \"AB"));
  }
  {
    EXPECT_THAT(Alphabet::LowerAlphanumeric().Validate(ctx, "b"),
                HasNoViolation());
    EXPECT_THAT(Alphabet::LowerAlphanumeric().Validate(ctx, "B"),
                HasViolation("one of \"ab"));
    EXPECT_THAT(Alphabet::LowerAlphanumeric().Validate(ctx, "3"),
                HasNoViolation());
    EXPECT_THAT(Alphabet::LowerAlphanumeric().Validate(ctx, "%"),
                HasViolation("one of \"ab"));
  }
  {
    EXPECT_THAT(Alphabet::Lowercase().Validate(ctx, "b"), HasNoViolation());
    EXPECT_THAT(Alphabet::Lowercase().Validate(ctx, "B"),
                HasViolation("one of \"ab"));
    EXPECT_THAT(Alphabet::Lowercase().Validate(ctx, "3"),
                HasViolation("one of \"ab"));
    EXPECT_THAT(Alphabet::Lowercase().Validate(ctx, "%"),
                HasViolation("one of \"ab"));
  }
  {
    EXPECT_THAT(Alphabet::Numbers().Validate(ctx, "b"),
                HasViolation("one of \"0123456789\""));
    EXPECT_THAT(Alphabet::Numbers().Validate(ctx, "B"),
                HasViolation("one of \"0123456789\""));
    EXPECT_THAT(Alphabet::Numbers().Validate(ctx, "3"), HasNoViolation());
    EXPECT_THAT(Alphabet::Numbers().Validate(ctx, "%"),
                HasViolation("one of \"0123456789\""));
  }
  {
    EXPECT_THAT(Alphabet::UpperAlphanumeric().Validate(ctx, "b"),
                HasViolation("one of \"AB"));
    EXPECT_THAT(Alphabet::UpperAlphanumeric().Validate(ctx, "B"),
                HasNoViolation());
    EXPECT_THAT(Alphabet::UpperAlphanumeric().Validate(ctx, "3"),
                HasNoViolation());
    EXPECT_THAT(Alphabet::UpperAlphanumeric().Validate(ctx, "%"),
                HasViolation("one of \"AB"));
  }
  {
    EXPECT_THAT(Alphabet::Uppercase().Validate(ctx, "b"),
                HasViolation("one of \"AB"));
    EXPECT_THAT(Alphabet::Uppercase().Validate(ctx, "B"), HasNoViolation());
    EXPECT_THAT(Alphabet::Uppercase().Validate(ctx, "3"),
                HasViolation("one of \"AB"));
    EXPECT_THAT(Alphabet::Uppercase().Validate(ctx, "%"),
                HasViolation("one of \"AB"));
  }
}

TEST(AlphabetTest, ToStringShouldWork) {
  EXPECT_EQ(Alphabet("abc").ToString(), "contains only the characters \"abc\"");
  EXPECT_EQ(Alphabet("AbC").ToString(), "contains only the characters \"AbC\"");
  EXPECT_EQ(Alphabet("A\tC").ToString(),
            "contains only the characters \"A\\tC\"");
  EXPECT_EQ(Alphabet("AAA").ToString(), "contains only the characters \"AAA\"");
}

TEST(DistinctCharactersTest, IsSatisfiedWithShouldWork) {
  Context context;
  ConstraintContext ctx("test", context.Variables(), context.Values());

  EXPECT_THAT(DistinctCharacters().Validate(ctx, ""), HasNoViolation());
  EXPECT_THAT(DistinctCharacters().Validate(ctx, "a"), HasNoViolation());
  EXPECT_THAT(DistinctCharacters().Validate(ctx, "ab"), HasNoViolation());
  EXPECT_THAT(DistinctCharacters().Validate(ctx, "abc"), HasNoViolation());
  EXPECT_THAT(DistinctCharacters().Validate(ctx, "aa"),
              HasViolation("distinct characters"));
  EXPECT_THAT(DistinctCharacters().Validate(ctx, "aba"),
              HasViolation("distinct characters"));
  EXPECT_THAT(DistinctCharacters().Validate(ctx, "abcabc"),
              HasViolation("distinct characters"));
  EXPECT_THAT(DistinctCharacters().Validate(ctx, "abcABC"), HasNoViolation());
}

TEST(DistinctCharactersTest, ToStringShouldWork) {
  EXPECT_EQ(DistinctCharacters().ToString(), "has distinct characters");
}

TEST(SimplePatternTest, IsSatisfiedWithShouldWork) {
  // See moriarty_internal::SimplePattern class for a more exhaustive set of
  // tests.
  Context context;
  ConstraintContext ctx("test", context.Variables(), context.Values());
  {
    EXPECT_THAT(SimplePattern("[a-z]*").Validate(ctx, ""), HasNoViolation());
    EXPECT_THAT(SimplePattern("[a-z]*").Validate(ctx, "a"), HasNoViolation());
    EXPECT_THAT(SimplePattern("[a-z]*").Validate(ctx, "abc"), HasNoViolation());
    EXPECT_THAT(SimplePattern("[a-z]*").Validate(ctx, "ABC"),
                HasViolation("simple pattern"));
  }
  {
    EXPECT_THAT(SimplePattern("[a-z]+").Validate(ctx, ""),
                HasViolation("simple pattern"));
    EXPECT_THAT(SimplePattern("[a-z]+").Validate(ctx, "a"), HasNoViolation());
    EXPECT_THAT(SimplePattern("[a-z]+").Validate(ctx, "abc"), HasNoViolation());
    EXPECT_THAT(SimplePattern("[a-z]+").Validate(ctx, "ABC"),
                HasViolation("simple pattern"));
  }
  {
    EXPECT_THAT(SimplePattern("[a-z]?").Validate(ctx, ""), HasNoViolation());
    EXPECT_THAT(SimplePattern("[a-z]?").Validate(ctx, "a"), HasNoViolation());
    EXPECT_THAT(SimplePattern("[a-z]?").Validate(ctx, "abc"),
                HasViolation("simple pattern"));
    EXPECT_THAT(SimplePattern("[a-z]?").Validate(ctx, "ABC"),
                HasViolation("simple pattern"));
  }
  {
    EXPECT_THAT(SimplePattern("a|b").Validate(ctx, "a"), HasNoViolation());
    EXPECT_THAT(SimplePattern("a|b").Validate(ctx, "b"), HasNoViolation());
    EXPECT_THAT(SimplePattern("a|b").Validate(ctx, "c"),
                HasViolation("simple pattern"));
  }
}

TEST(SimplePatternTest, IsSatisfiedWithIncludingVariablesShouldWork) {
  // See moriarty_internal::SimplePattern class for a more exhaustive set of
  // tests.
  Context context =
      Context().WithValue<MInteger>("N", 2).WithValue<MInteger>("X", 7);
  ConstraintContext ctx("test", context.Variables(), context.Values());
  EXPECT_THAT(SimplePattern("[a-z]{N, X}").Validate(ctx, "asdf"),
              HasNoViolation());
  EXPECT_THAT(SimplePattern("[a-z]{2 * N}").Validate(ctx, "asdf"),
              HasNoViolation());
  EXPECT_THAT(SimplePattern("[a-z]{2 * N}").Validate(ctx, "a"),
              HasViolation("simple pattern"));
  EXPECT_THAT(SimplePattern("[a-z]{max(N, X)}").Validate(ctx, "abcdefg"),
              HasNoViolation());
  EXPECT_THAT(SimplePattern("[a-z]{min(N, X)}").Validate(ctx, "ab"),
              HasNoViolation());
  EXPECT_THAT(SimplePattern("[a-z]{abs(-N)}").Validate(ctx, "ab"),
              HasNoViolation());
  EXPECT_THAT(SimplePattern("N{N}").Validate(ctx, "NN"),
              HasNoViolation());  // Variable + char
}

TEST(SimplePatternTest, ToStringShouldWork) {
  EXPECT_EQ(SimplePattern("[a-z]*").ToString(),
            "has a simple pattern of \"[a-z]*\"");
  EXPECT_EQ(SimplePattern("[^a-z]?").ToString(),
            "has a simple pattern of \"[^a-z]?\"");
}

TEST(SimplePatternTest, GetDependenciesShouldWork) {
  EXPECT_THAT(SimplePattern("[a-z]{5}").GetDependencies(), IsEmpty());
  EXPECT_THAT(SimplePattern("[a-z]{N}").GetDependencies(), ElementsAre("N"));
  EXPECT_THAT(SimplePattern("[a-z]{N, X}").GetDependencies(),
              UnorderedElementsAre("N", "X"));
}

}  // namespace
}  // namespace moriarty
