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

#include "src/variables/constraints/string_constraints.h"

#include <unordered_set>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/contexts/librarian_context.h"
#include "src/testing/gtest_helpers.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace {

using ::moriarty::librarian::AnalysisContext;
using ::moriarty_testing::Context;
using ::moriarty_testing::HasConstraintViolation;
using ::moriarty_testing::HasNoConstraintViolation;
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
  EXPECT_THAT(Alphabet::UpperCase().GetAlphabet(), Not(HasDuplicateLetter()));
  EXPECT_THAT(Alphabet::LowerCase().GetAlphabet(), Not(HasDuplicateLetter()));
  EXPECT_THAT(Alphabet::Numbers().GetAlphabet(), Not(HasDuplicateLetter()));
  EXPECT_THAT(Alphabet::AlphaNumeric().GetAlphabet(),
              Not(HasDuplicateLetter()));
  EXPECT_THAT(Alphabet::UpperAlphaNumeric().GetAlphabet(),
              Not(HasDuplicateLetter()));
  EXPECT_THAT(Alphabet::LowerAlphaNumeric().GetAlphabet(),
              Not(HasDuplicateLetter()));
}

TEST(AlphabetTest, CommonAlphabetsHaveTheAppropriateNumberOfElements) {
  EXPECT_THAT(Alphabet::Letters().GetAlphabet(), SizeIs(26 + 26));
  EXPECT_THAT(Alphabet::UpperCase().GetAlphabet(), SizeIs(26));
  EXPECT_THAT(Alphabet::LowerCase().GetAlphabet(), SizeIs(26));
  EXPECT_THAT(Alphabet::Numbers().GetAlphabet(), SizeIs(10));
  EXPECT_THAT(Alphabet::AlphaNumeric().GetAlphabet(), SizeIs(26 + 26 + 10));
  EXPECT_THAT(Alphabet::UpperAlphaNumeric().GetAlphabet(), SizeIs(26 + 10));
  EXPECT_THAT(Alphabet::LowerAlphaNumeric().GetAlphabet(), SizeIs(26 + 10));
}

TEST(AlphabetTest, BasicConstructorShouldReturnExactAlphabet) {
  EXPECT_EQ(Alphabet("abc").GetAlphabet(), "abc");
  EXPECT_EQ(Alphabet("AbC").GetAlphabet(), "AbC");
  EXPECT_EQ(Alphabet("A\tC").GetAlphabet(), "A\tC");
  EXPECT_EQ(Alphabet("AAA").GetAlphabet(), "AAA");
}

TEST(AlphabetTest, IsSatisfiedWithShouldWork) {
  EXPECT_THAT(Alphabet("abc").CheckValue("a"), HasNoConstraintViolation());
  EXPECT_THAT(Alphabet("abc").CheckValue("A"),
              HasConstraintViolation(HasSubstr("`A`")));

  {
    EXPECT_THAT(Alphabet::AlphaNumeric().CheckValue("b"),
                HasNoConstraintViolation());
    EXPECT_THAT(Alphabet::AlphaNumeric().CheckValue("B"),
                HasNoConstraintViolation());
    EXPECT_THAT(Alphabet::AlphaNumeric().CheckValue("3"),
                HasNoConstraintViolation());
    EXPECT_THAT(Alphabet::AlphaNumeric().CheckValue("%"),
                HasConstraintViolation(HasSubstr("`%`")));
  }
  {
    EXPECT_THAT(Alphabet::Letters().CheckValue("b"),
                HasNoConstraintViolation());
    EXPECT_THAT(Alphabet::Letters().CheckValue("B"),
                HasNoConstraintViolation());
    EXPECT_THAT(Alphabet::Letters().CheckValue("3"),
                HasConstraintViolation(HasSubstr("`3`")));
    EXPECT_THAT(Alphabet::Letters().CheckValue("%"),
                HasConstraintViolation(HasSubstr("`%`")));
  }
  {
    EXPECT_THAT(Alphabet::LowerAlphaNumeric().CheckValue("b"),
                HasNoConstraintViolation());
    EXPECT_THAT(Alphabet::LowerAlphaNumeric().CheckValue("B"),
                HasConstraintViolation(HasSubstr("`B`")));
    EXPECT_THAT(Alphabet::LowerAlphaNumeric().CheckValue("3"),
                HasNoConstraintViolation());
    EXPECT_THAT(Alphabet::LowerAlphaNumeric().CheckValue("%"),
                HasConstraintViolation(HasSubstr("`%`")));
  }
  {
    EXPECT_THAT(Alphabet::LowerCase().CheckValue("b"),
                HasNoConstraintViolation());
    EXPECT_THAT(Alphabet::LowerCase().CheckValue("B"),
                HasConstraintViolation(HasSubstr("`B`")));
    EXPECT_THAT(Alphabet::LowerCase().CheckValue("3"),
                HasConstraintViolation(HasSubstr("`3`")));
    EXPECT_THAT(Alphabet::LowerCase().CheckValue("%"),
                HasConstraintViolation(HasSubstr("`%`")));
  }
  {
    EXPECT_THAT(Alphabet::Numbers().CheckValue("b"),
                HasConstraintViolation(HasSubstr("`b`")));
    EXPECT_THAT(Alphabet::Numbers().CheckValue("B"),
                HasConstraintViolation(HasSubstr("`B`")));
    EXPECT_THAT(Alphabet::Numbers().CheckValue("3"),
                HasNoConstraintViolation());
    EXPECT_THAT(Alphabet::Numbers().CheckValue("%"),
                HasConstraintViolation(HasSubstr("`%`")));
  }
  {
    EXPECT_THAT(Alphabet::UpperAlphaNumeric().CheckValue("b"),
                HasConstraintViolation(HasSubstr("`b`")));
    EXPECT_THAT(Alphabet::UpperAlphaNumeric().CheckValue("B"),
                HasNoConstraintViolation());
    EXPECT_THAT(Alphabet::UpperAlphaNumeric().CheckValue("3"),
                HasNoConstraintViolation());
    EXPECT_THAT(Alphabet::UpperAlphaNumeric().CheckValue("%"),
                HasConstraintViolation(HasSubstr("`%`")));
  }
  {
    EXPECT_THAT(Alphabet::UpperCase().CheckValue("b"),
                HasConstraintViolation(HasSubstr("`b`")));
    EXPECT_THAT(Alphabet::UpperCase().CheckValue("B"),
                HasNoConstraintViolation());
    EXPECT_THAT(Alphabet::UpperCase().CheckValue("3"),
                HasConstraintViolation(HasSubstr("`3`")));
    EXPECT_THAT(Alphabet::UpperCase().CheckValue("%"),
                HasConstraintViolation(HasSubstr("`%`")));
  }
}

TEST(AlphabetTest, ToStringShouldWork) {
  EXPECT_EQ(Alphabet("abc").ToString(), "contains only the characters `abc`");
  EXPECT_EQ(Alphabet("AbC").ToString(), "contains only the characters `AbC`");
  EXPECT_EQ(Alphabet("A\tC").ToString(),
            "contains only the characters `A\\tC`");
  EXPECT_EQ(Alphabet("AAA").ToString(), "contains only the characters `AAA`");
}

TEST(DistinctCharactersTest, IsSatisfiedWithShouldWork) {
  EXPECT_THAT(DistinctCharacters().CheckValue(""), HasNoConstraintViolation());
  EXPECT_THAT(DistinctCharacters().CheckValue("a"), HasNoConstraintViolation());
  EXPECT_THAT(DistinctCharacters().CheckValue("ab"),
              HasNoConstraintViolation());
  EXPECT_THAT(DistinctCharacters().CheckValue("abc"),
              HasNoConstraintViolation());
  EXPECT_THAT(DistinctCharacters().CheckValue("aa"),
              HasConstraintViolation(HasSubstr("multiple times")));
  EXPECT_THAT(DistinctCharacters().CheckValue("aba"),
              HasConstraintViolation(HasSubstr("multiple times")));
  EXPECT_THAT(DistinctCharacters().CheckValue("abcabc"),
              HasConstraintViolation(HasSubstr("multiple times")));
  EXPECT_THAT(DistinctCharacters().CheckValue("abcABC"),
              HasNoConstraintViolation());
}

TEST(DistinctCharactersTest, ToStringShouldWork) {
  EXPECT_EQ(DistinctCharacters().ToString(), "has distinct characters");
}

TEST(SimplePatternTest, IsSatisfiedWithShouldWork) {
  // See moriarty_internal::SimplePattern class for a more exhaustive set of
  // tests.
  Context context;
  AnalysisContext ctx("test", context.Variables(), context.Values());
  {
    EXPECT_THAT(SimplePattern("[a-z]*").CheckValue(ctx, ""),
                HasNoConstraintViolation());
    EXPECT_THAT(SimplePattern("[a-z]*").CheckValue(ctx, "a"),
                HasNoConstraintViolation());
    EXPECT_THAT(SimplePattern("[a-z]*").CheckValue(ctx, "abc"),
                HasNoConstraintViolation());
    EXPECT_THAT(SimplePattern("[a-z]*").CheckValue(ctx, "ABC"),
                HasConstraintViolation(HasSubstr("simple pattern")));
  }
  {
    EXPECT_THAT(SimplePattern("[a-z]+").CheckValue(ctx, ""),
                HasConstraintViolation(HasSubstr("simple pattern")));
    EXPECT_THAT(SimplePattern("[a-z]+").CheckValue(ctx, "a"),
                HasNoConstraintViolation());
    EXPECT_THAT(SimplePattern("[a-z]+").CheckValue(ctx, "abc"),
                HasNoConstraintViolation());
    EXPECT_THAT(SimplePattern("[a-z]+").CheckValue(ctx, "ABC"),
                HasConstraintViolation(HasSubstr("simple pattern")));
  }
  {
    EXPECT_THAT(SimplePattern("[a-z]?").CheckValue(ctx, ""),
                HasNoConstraintViolation());
    EXPECT_THAT(SimplePattern("[a-z]?").CheckValue(ctx, "a"),
                HasNoConstraintViolation());
    EXPECT_THAT(SimplePattern("[a-z]?").CheckValue(ctx, "abc"),
                HasConstraintViolation(HasSubstr("simple pattern")));
    EXPECT_THAT(SimplePattern("[a-z]?").CheckValue(ctx, "ABC"),
                HasConstraintViolation(HasSubstr("simple pattern")));
  }
  {
    EXPECT_THAT(SimplePattern("a|b").CheckValue(ctx, "a"),
                HasNoConstraintViolation());
    EXPECT_THAT(SimplePattern("a|b").CheckValue(ctx, "b"),
                HasNoConstraintViolation());
    EXPECT_THAT(SimplePattern("a|b").CheckValue(ctx, "c"),
                HasConstraintViolation(HasSubstr("simple pattern")));
  }
}

TEST(SimplePatternTest, IsSatisfiedWithIncludingVariablesShouldWork) {
  // See moriarty_internal::SimplePattern class for a more exhaustive set of
  // tests.
  Context context =
      Context().WithValue<MInteger>("N", 2).WithValue<MInteger>("X", 7);
  AnalysisContext ctx("test", context.Variables(), context.Values());
  EXPECT_THAT(SimplePattern("[a-z]{N, X}").CheckValue(ctx, "asdf"),
              HasNoConstraintViolation());
  EXPECT_THAT(SimplePattern("[a-z]{2 * N}").CheckValue(ctx, "asdf"),
              HasNoConstraintViolation());
  EXPECT_THAT(SimplePattern("[a-z]{2 * N}").CheckValue(ctx, "a"),
              HasConstraintViolation(HasSubstr("simple pattern")));
  EXPECT_THAT(SimplePattern("[a-z]{max(N, X)}").CheckValue(ctx, "abcdefg"),
              HasNoConstraintViolation());
  EXPECT_THAT(SimplePattern("[a-z]{min(N, X)}").CheckValue(ctx, "ab"),
              HasNoConstraintViolation());
  EXPECT_THAT(SimplePattern("[a-z]{abs(-N)}").CheckValue(ctx, "ab"),
              HasNoConstraintViolation());
  EXPECT_THAT(SimplePattern("N{N}").CheckValue(ctx, "NN"),
              HasNoConstraintViolation());  // Variable + char
}

TEST(SimplePatternTest, ToStringShouldWork) {
  EXPECT_EQ(SimplePattern("[a-z]*").ToString(),
            "has a simple pattern of `[a-z]*`");
  EXPECT_EQ(SimplePattern("[^a-z]?").ToString(),
            "has a simple pattern of `[^a-z]?`");
}

TEST(SimplePatternTest, GetDependenciesShouldWork) {
  EXPECT_THAT(SimplePattern("[a-z]{5}").GetDependencies(), IsEmpty());
  EXPECT_THAT(SimplePattern("[a-z]{N}").GetDependencies(), ElementsAre("N"));
  EXPECT_THAT(SimplePattern("[a-z]{N, X}").GetDependencies(),
              UnorderedElementsAre("N", "X"));
}

}  // namespace
}  // namespace moriarty
