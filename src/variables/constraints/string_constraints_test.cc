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
using ::testing::ElementsAre;
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
  EXPECT_TRUE(Alphabet("abc").IsSatisfiedWith("a"));
  EXPECT_FALSE(Alphabet("abc").IsSatisfiedWith("A"));

  {
    EXPECT_TRUE(Alphabet::AlphaNumeric().IsSatisfiedWith("b"));
    EXPECT_TRUE(Alphabet::AlphaNumeric().IsSatisfiedWith("B"));
    EXPECT_TRUE(Alphabet::AlphaNumeric().IsSatisfiedWith("3"));
    EXPECT_FALSE(Alphabet::AlphaNumeric().IsSatisfiedWith("%"));
  }
  {
    EXPECT_TRUE(Alphabet::Letters().IsSatisfiedWith("b"));
    EXPECT_TRUE(Alphabet::Letters().IsSatisfiedWith("B"));
    EXPECT_FALSE(Alphabet::Letters().IsSatisfiedWith("3"));
    EXPECT_FALSE(Alphabet::Letters().IsSatisfiedWith("%"));
  }
  {
    EXPECT_TRUE(Alphabet::LowerAlphaNumeric().IsSatisfiedWith("b"));
    EXPECT_FALSE(Alphabet::LowerAlphaNumeric().IsSatisfiedWith("B"));
    EXPECT_TRUE(Alphabet::LowerAlphaNumeric().IsSatisfiedWith("3"));
    EXPECT_FALSE(Alphabet::LowerAlphaNumeric().IsSatisfiedWith("%"));
  }
  {
    EXPECT_TRUE(Alphabet::LowerCase().IsSatisfiedWith("b"));
    EXPECT_FALSE(Alphabet::LowerCase().IsSatisfiedWith("B"));
    EXPECT_FALSE(Alphabet::LowerCase().IsSatisfiedWith("3"));
    EXPECT_FALSE(Alphabet::LowerCase().IsSatisfiedWith("%"));
  }
  {
    EXPECT_FALSE(Alphabet::Numbers().IsSatisfiedWith("b"));
    EXPECT_FALSE(Alphabet::Numbers().IsSatisfiedWith("B"));
    EXPECT_TRUE(Alphabet::Numbers().IsSatisfiedWith("3"));
    EXPECT_FALSE(Alphabet::Numbers().IsSatisfiedWith("%"));
  }
  {
    EXPECT_FALSE(Alphabet::UpperAlphaNumeric().IsSatisfiedWith("b"));
    EXPECT_TRUE(Alphabet::UpperAlphaNumeric().IsSatisfiedWith("B"));
    EXPECT_TRUE(Alphabet::UpperAlphaNumeric().IsSatisfiedWith("3"));
    EXPECT_FALSE(Alphabet::UpperAlphaNumeric().IsSatisfiedWith("%"));
  }
  {
    EXPECT_FALSE(Alphabet::UpperCase().IsSatisfiedWith("b"));
    EXPECT_TRUE(Alphabet::UpperCase().IsSatisfiedWith("B"));
    EXPECT_FALSE(Alphabet::UpperCase().IsSatisfiedWith("3"));
    EXPECT_FALSE(Alphabet::UpperCase().IsSatisfiedWith("%"));
  }
}

TEST(AlphabetTest, ToStringShouldWork) {
  EXPECT_EQ(Alphabet("abc").ToString(), "contains only the characters `abc`");
  EXPECT_EQ(Alphabet("AbC").ToString(), "contains only the characters `AbC`");
  // TODO(darcybest): Consider escaping whitespace characters in string.
  EXPECT_EQ(Alphabet("A\tC").ToString(), "contains only the characters `A\tC`");
  EXPECT_EQ(Alphabet("AAA").ToString(), "contains only the characters `AAA`");
}

TEST(AlphabetTest, UnsatisfiedReasonShouldWork) {
  EXPECT_EQ(Alphabet("abc").UnsatisfiedReason("A"),
            "character at index 0 (which is `A`) is not a valid character "
            "(valid characters are `abc`)");
  // TODO: Consider a nicer message for the common cases.
  EXPECT_EQ(Alphabet::LowerCase().UnsatisfiedReason("abcXdef"),
            "character at index 3 (which is `X`) is not a valid character "
            "(valid characters are `abcdefghijklmnopqrstuvwxyz`)");
}

TEST(DistinctCharactersTest, IsSatisfiedWithShouldWork) {
  EXPECT_TRUE(DistinctCharacters().IsSatisfiedWith(""));
  EXPECT_TRUE(DistinctCharacters().IsSatisfiedWith("a"));
  EXPECT_TRUE(DistinctCharacters().IsSatisfiedWith("ab"));
  EXPECT_TRUE(DistinctCharacters().IsSatisfiedWith("abc"));
  EXPECT_FALSE(DistinctCharacters().IsSatisfiedWith("aa"));
  EXPECT_FALSE(DistinctCharacters().IsSatisfiedWith("aba"));
  EXPECT_FALSE(DistinctCharacters().IsSatisfiedWith("abcabc"));
  EXPECT_TRUE(DistinctCharacters().IsSatisfiedWith("abcABC"));
}

TEST(DistinctCharactersTest, ToStringShouldWork) {
  EXPECT_EQ(DistinctCharacters().ToString(), "has distinct characters");
}

TEST(DistinctCharactersTest, UnsatisfiedReasonShouldWork) {
  EXPECT_EQ(DistinctCharacters().UnsatisfiedReason("aa"),
            "character at index 1 (which is `a`) appears multiple times");
  EXPECT_EQ(DistinctCharacters().UnsatisfiedReason("abb"),
            "character at index 2 (which is `b`) appears multiple times");
  EXPECT_EQ(DistinctCharacters().UnsatisfiedReason("abca"),
            "character at index 3 (which is `a`) appears multiple times");
}

TEST(SimplePatternTest, IsSatisfiedWithShouldWork) {
  // See moriarty_internal::SimplePattern class for a more exhaustive set of
  // tests.
  Context context;
  AnalysisContext ctx("test", context.Variables(), context.Values());
  {
    EXPECT_TRUE(SimplePattern("[a-z]*").IsSatisfiedWith(ctx, ""));
    EXPECT_TRUE(SimplePattern("[a-z]*").IsSatisfiedWith(ctx, "a"));
    EXPECT_TRUE(SimplePattern("[a-z]*").IsSatisfiedWith(ctx, "abc"));
    EXPECT_FALSE(SimplePattern("[a-z]*").IsSatisfiedWith(ctx, "ABC"));
  }
  {
    EXPECT_FALSE(SimplePattern("[a-z]+").IsSatisfiedWith(ctx, ""));
    EXPECT_TRUE(SimplePattern("[a-z]+").IsSatisfiedWith(ctx, "a"));
    EXPECT_TRUE(SimplePattern("[a-z]+").IsSatisfiedWith(ctx, "abc"));
    EXPECT_FALSE(SimplePattern("[a-z]+").IsSatisfiedWith(ctx, "ABC"));
  }
  {
    EXPECT_TRUE(SimplePattern("[a-z]?").IsSatisfiedWith(ctx, ""));
    EXPECT_TRUE(SimplePattern("[a-z]?").IsSatisfiedWith(ctx, "a"));
    EXPECT_FALSE(SimplePattern("[a-z]?").IsSatisfiedWith(ctx, "abc"));
    EXPECT_FALSE(SimplePattern("[a-z]?").IsSatisfiedWith(ctx, "ABC"));
  }
  EXPECT_TRUE(SimplePattern("a|b").IsSatisfiedWith(ctx, "a"));
  EXPECT_TRUE(SimplePattern("a|b").IsSatisfiedWith(ctx, "b"));
  EXPECT_FALSE(SimplePattern("a|b").IsSatisfiedWith(ctx, "c"));
}

TEST(SimplePatternTest, IsSatisfiedWithIncludingVariablesShouldWork) {
  // See moriarty_internal::SimplePattern class for a more exhaustive set of
  // tests.
  Context context =
      Context().WithValue<MInteger>("N", 2).WithValue<MInteger>("X", 7);
  AnalysisContext ctx("test", context.Variables(), context.Values());
  EXPECT_TRUE(SimplePattern("[a-z]{N, X}").IsSatisfiedWith(ctx, "asdf"));
  EXPECT_TRUE(SimplePattern("[a-z]{2 * N}").IsSatisfiedWith(ctx, "asdf"));
  EXPECT_FALSE(SimplePattern("[a-z]{2 * N}").IsSatisfiedWith(ctx, "a"));
  EXPECT_TRUE(
      SimplePattern("[a-z]{max(N, X)}").IsSatisfiedWith(ctx, "abcdefg"));
  EXPECT_TRUE(SimplePattern("[a-z]{min(N, X)}").IsSatisfiedWith(ctx, "ab"));
  EXPECT_TRUE(SimplePattern("[a-z]{abs(-N)}").IsSatisfiedWith(ctx, "ab"));
  EXPECT_TRUE(
      SimplePattern("N{N}").IsSatisfiedWith(ctx, "NN"));  // Variable + char
}

TEST(SimplePatternTest, ToStringShouldWork) {
  EXPECT_EQ(SimplePattern("[a-z]*").ToString(),
            "has a simple pattern of `[a-z]*`");
  EXPECT_EQ(SimplePattern("[^a-z]?").ToString(),
            "has a simple pattern of `[^a-z]?`");
}

TEST(SimplePatternTest, UnsatisfiedReasonShouldWork) {
  EXPECT_EQ(SimplePattern("[a-z]*").UnsatisfiedReason("A"),
            "does not follow the simple pattern of `[a-z]*`");
  EXPECT_EQ(SimplePattern("[^a-z]?").UnsatisfiedReason("a"),
            "does not follow the simple pattern of `[^a-z]?`");
}

TEST(SimplePatternTest, GetDependenciesShouldWork) {
  EXPECT_THAT(SimplePattern("[a-z]{5}").GetDependencies(), IsEmpty());
  EXPECT_THAT(SimplePattern("[a-z]{N}").GetDependencies(), ElementsAre("N"));
  EXPECT_THAT(SimplePattern("[a-z]{N, X}").GetDependencies(),
              UnorderedElementsAre("N", "X"));
}

}  // namespace
}  // namespace moriarty
