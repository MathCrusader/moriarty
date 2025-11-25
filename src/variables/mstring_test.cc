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

#include "src/variables/mstring.h"

#include <string_view>
#include <unordered_set>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/constraints/container_constraints.h"
#include "src/constraints/numeric_constraints.h"
#include "src/constraints/string_constraints.h"
#include "src/librarian/errors.h"
#include "src/librarian/mvariable.h"
#include "src/librarian/testing/gtest_helpers.h"

namespace moriarty {
namespace {

using ::moriarty_testing::Context;
using ::moriarty_testing::GeneratedValuesAre;
using ::moriarty_testing::GenerateEdgeCases;
using ::moriarty_testing::GenerateSameValues;
using ::moriarty_testing::GenerateThrowsGenerationError;
using ::moriarty_testing::IsNotSatisfiedWith;
using ::moriarty_testing::IsSatisfiedWith;
using ::moriarty_testing::Print;
using ::moriarty_testing::Read;
using ::moriarty_testing::ThrowsImpossibleToSatisfy;
using ::testing::AllOf;
using ::testing::Contains;
using ::testing::Ge;
using ::testing::IsSupersetOf;
using ::testing::Le;
using ::testing::MatchesRegex;
using ::testing::Not;
using ::testing::SizeIs;
using ::testing::Throws;

TEST(MStringTest, TypenameIsCorrect) {
  EXPECT_EQ(MString().Typename(), "MString");
}

TEST(MStringTest, PrintShouldSucceed) {
  EXPECT_EQ(Print(MString(), "value!"), "value!");
  EXPECT_EQ(Print(MString(), ""), "");
  EXPECT_EQ(Print(MString(), "multiple tokens"), "multiple tokens");
}

TEST(MStringTest, SingleTokenReadShouldSucceed) {
  EXPECT_EQ(Read(MString(), "123"), "123");
  EXPECT_EQ(Read(MString(), "abc"), "abc");
}

TEST(MStringTest, InputWithTokenWithWhitespaceAfterShouldReadToken) {
  EXPECT_EQ(Read(MString(), "world "), "world");
  EXPECT_EQ(Read(MString(), "you should ignore some of this"), "you");
}

TEST(MStringTest, ReadATokenWithLeadingWhitespaceShouldFail) {
  EXPECT_THROW((void)Read(MString(), " spacebefore"), IOError);
}

TEST(MStringTest, ReadAtEofShouldFail) {
  EXPECT_THROW((void)Read(MString(), ""), IOError);
}

TEST(MStringTest, IsSatisfiedWithShouldAcceptAllMStringsForDefault) {
  // Default string should accept all strings.
  EXPECT_THAT(MString(), IsSatisfiedWith(""));
  EXPECT_THAT(MString(), IsSatisfiedWith("hello"));
  EXPECT_THAT(MString(), IsSatisfiedWith("blah blah blah"));
}

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

TEST(MStringTest, GenerateShouldSuccessfullyComplete) {
  EXPECT_THAT(MString(Length(Between(4, 11)), Alphabet("abc")),
              GeneratedValuesAre(SizeIs(AllOf(Ge(4), Le(11)))));
  EXPECT_THAT(MString(Length(4), Alphabet("abc")),
              GeneratedValuesAre(SizeIs(4)));
}

TEST(MStringTest, RepeatedLengthCallsShouldBeIntersectedTogether) {
  auto gen_given_length1 = [](int lo, int hi) {
    return MString(Length(Between(lo, hi)), Alphabet("abcdef"));
  };
  auto gen_given_length2 = [](int lo1, int hi1, int lo2, int hi2) {
    return MString(Length(Between(lo1, hi1)), Length(Between(lo2, hi2)),
                   Alphabet("abcdef"));
  };

  // All possible valid intersections
  EXPECT_TRUE(
      GenerateSameValues(gen_given_length2(0, 30, 1, 10),
                         gen_given_length1(1, 10)));  // First is superset

  EXPECT_TRUE(
      GenerateSameValues(gen_given_length2(1, 10, 0, 30),
                         gen_given_length1(1, 10)));  // Second is superset

  EXPECT_TRUE(
      GenerateSameValues(gen_given_length2(0, 10, 1, 30),
                         gen_given_length1(1, 10)));  // First on the left

  EXPECT_TRUE(
      GenerateSameValues(gen_given_length2(1, 30, 0, 10),
                         gen_given_length1(1, 10)));  // First on the right

  EXPECT_TRUE(GenerateSameValues(gen_given_length2(1, 8, 8, 10),
                                 gen_given_length1(8, 8)));  // Singleton Range
}

TEST(MStringTest, InvalidLengthShouldFail) {
  EXPECT_THAT(MString(Length(-1), Alphabet("a")),
              GenerateThrowsGenerationError(".length", Context()));
  EXPECT_THAT(MString(Length(AtMost(10), AtLeast(20)), Alphabet("a")),
              GenerateThrowsGenerationError(".length", Context()));
}

TEST(MStringTest, LengthZeroProcudesTheEmptyString) {
  EXPECT_THAT(MString(Length(0), Alphabet("abc")),
              GeneratedValuesAre(SizeIs(0)));
}

TEST(MStringTest, AlphabetIsRequiredForGenerate) {
  EXPECT_THAT(MString(Length(10)),
              GenerateThrowsGenerationError("", Context()));
}

TEST(MStringTest, MergeFromCorrectlyMergesOnLength) {
  // The alphabet is irrelevant for the tests
  auto get_str = [](int lo, int hi) {
    return MString(Length(Between(lo, hi)), Alphabet("abc"));
  };

  // All possible valid intersection of length
  EXPECT_TRUE(GenerateSameValues(get_str(0, 30).MergeFrom(get_str(1, 10)),
                                 get_str(1, 10)));  // First is a superset
  EXPECT_TRUE(GenerateSameValues(get_str(1, 10).MergeFrom(get_str(0, 30)),
                                 get_str(1, 10)));  // Second is a superset
  EXPECT_TRUE(GenerateSameValues(get_str(0, 10).MergeFrom(get_str(1, 30)),
                                 get_str(1, 10)));  // First on left
  EXPECT_TRUE(GenerateSameValues(get_str(1, 30).MergeFrom(get_str(0, 10)),
                                 get_str(1, 10)));  // First on right
  EXPECT_TRUE(GenerateSameValues(get_str(1, 8).MergeFrom(get_str(8, 10)),
                                 get_str(8, 8)));  // Singleton range

  EXPECT_THAT(get_str(1, 6).MergeFrom(get_str(10, 20)),
              GenerateThrowsGenerationError(".length", Context()));
}

TEST(MStringTest, MergeFromCorrectlyMergesOnAlphabet) {
  auto string_with_alphabet = [](std::string_view alphabet) {
    return MString(Length(20), Alphabet(alphabet));
  };

  // Intersections of alphabets
  EXPECT_TRUE(GenerateSameValues(
      string_with_alphabet("abcdef").MergeFrom(string_with_alphabet("abc")),
      string_with_alphabet("abc")));  // First is a superset
  EXPECT_TRUE(GenerateSameValues(
      string_with_alphabet("abc").MergeFrom(string_with_alphabet("abcdef")),
      string_with_alphabet("abc")));  // Second is a superset
  EXPECT_TRUE(GenerateSameValues(
      string_with_alphabet("ab").MergeFrom(string_with_alphabet("bc")),
      string_with_alphabet("b")));  // Non-empty intersection

  EXPECT_THAT(
      [&] { string_with_alphabet("ab").MergeFrom(string_with_alphabet("cd")); },
      ThrowsImpossibleToSatisfy("ab"));
}

TEST(MStringTest, LengthIsSatisfied) {
  // Includes small and large ranges
  for (int size = 0; size < 40; size++) {
    EXPECT_THAT(MString(Length(Between(size / 2, size)), Alphabet("abc")),
                GeneratedValuesAre(SizeIs(AllOf(Ge(size / 2), Le(size)))));
  }
  for (int size = 900; size < 940; size++) {
    EXPECT_THAT(MString(Length(Between(size / 2, size)), Alphabet("abc")),
                GeneratedValuesAre(SizeIs(AllOf(Ge(size / 2), Le(size)))));
  }
}

TEST(MStringTest, AlphabetIsSatisfied) {
  EXPECT_THAT(MString(Length(Between(10, 20)), Alphabet("a")),
              GeneratedValuesAre(MatchesRegex("a*")));
  EXPECT_THAT(MString(Length(Between(10, 20)), Alphabet("abc")),
              GeneratedValuesAre(MatchesRegex("[abc]*")));
}

TEST(MStringTest, AlphabetNotGivenInSortedOrderIsFine) {
  EXPECT_TRUE(
      GenerateSameValues(MString(Length(Between(10, 20)), Alphabet("abc")),
                         MString(Length(Between(10, 20)), Alphabet("cab"))));
}

TEST(MStringTest, DuplicateLettersInAlphabetAreIgnored) {
  // a appears 90% of the alphabet. We should still see ~50% of the time in the
  // input. With 10,000 samples, I'm putting the cutoff at 60%. Anything over
  // 60% is extremely unlikely to happen.
  EXPECT_THAT(MString(Length(10000), Alphabet("aaaaaaaaab")),
              GeneratedValuesAre(Contains('a').Times(Le(6000))));
}

TEST(MStringTest, IsSatisfiedWithShouldAcceptAllMStringsOfCorrectLength) {
  EXPECT_THAT(MString(Length(5)), IsSatisfiedWith("abcde"));
  EXPECT_THAT(MString(Length(Between(4, 6))), IsSatisfiedWith("abcde"));
  EXPECT_THAT(MString(Length(Between(4, 5))), IsSatisfiedWith("abcde"));
  EXPECT_THAT(MString(Length(Between(5, 6))), IsSatisfiedWith("abcde"));
  EXPECT_THAT(MString(Length(Between(5, 5))), IsSatisfiedWith("abcde"));

  EXPECT_THAT(MString(Length(Between(3, 4))),
              IsNotSatisfiedWith("abcde", "length"));
  EXPECT_THAT(MString(Length(Between(1, 1000))),
              IsNotSatisfiedWith("", "length"));
}

TEST(MStringTest, IsSatisfiedWithShouldCheckTheAlphabetIfSet) {
  EXPECT_THAT(MString(Alphabet("abcdefghij")), IsSatisfiedWith("abcde"));
  EXPECT_THAT(MString(Alphabet("edbca")), IsSatisfiedWith("abcde"));

  EXPECT_THAT(MString(Alphabet("abcd")), IsNotSatisfiedWith("abcde", "`abcd`"));
}

TEST(MStringTest, IsSatisfiedWithWithInvalidLengthShouldFail) {
  EXPECT_THAT(MString(Length(AtLeast(20), AtMost(10))),
              IsNotSatisfiedWith("abcde", "length"));
}

TEST(MStringTest, IsSatisfiedWithShouldCheckForDistinctCharacters) {
  EXPECT_THAT(MString(DistinctCharacters()), IsSatisfiedWith("abcdef"));
  EXPECT_THAT(MString(Alphabet("abcdef"), DistinctCharacters()),
              IsSatisfiedWith("cbf"));

  EXPECT_THAT(MString(Alphabet("abcdef"), DistinctCharacters()),
              IsNotSatisfiedWith("cc", "multiple times"));
}

TEST(MStringTest, DistinctCharactersWorksInTheSimpleCase) {
  EXPECT_THAT(MString(Length(Between(1, 26)), Alphabet::Letters(),
                      DistinctCharacters()),
              GeneratedValuesAre(Not(HasDuplicateLetter())));
}

TEST(MStringTest, DistinctCharactersRequiresAShortLength) {
  EXPECT_THAT(
      MString(Length(Between(5, 5)), Alphabet("abc"), DistinctCharacters()),
      GenerateThrowsGenerationError(".length", Context()));

  // Most of the range is too large, the only way to succeed is for it to make a
  // string of length 10.
  EXPECT_THAT(MString(Length(Between(10, 1000000)), Alphabet::Numbers(),
                      DistinctCharacters()),
              GeneratedValuesAre(Not(HasDuplicateLetter())));
}

TEST(MStringTest, MergingSimplePatternsIntoAnMStringWithoutShouldWork) {
  MString constraints = MString(SimplePattern("[abc]{10, 20}"));
  MString().MergeFrom(constraints);  // No crash = good.
}

TEST(MStringTest, MergingTwoIdenticalSimplePatternsTogetherShouldWork) {
  MString constraints = MString(SimplePattern("[abc]{10, 20}"));

  MString(SimplePattern("[abc]{10, 20}"))
      .MergeFrom(constraints);  // No crash = good.
}

TEST(MStringTest, MergingTwoDifferentSimplePatternsTogetherShouldWork) {
  MString constraints = MString(SimplePattern("[abc]{10, 20}"));
  MString(SimplePattern("xxxxx")).MergeFrom(constraints);  // No crash = good.
}

TEST(MStringTest,
     MergingTwoDifferentSimplePatternsTogetherShouldGenerateIfCompatible) {
  EXPECT_THAT(
      MString(SimplePattern("[cd]{10, 20}"), SimplePattern("[cd]{5, 15}")),
      GeneratedValuesAre(MatchesRegex("[cd]{10,15}")));

  // Note, we don't know which simple pattern the generated value doesn't match.
  EXPECT_THAT(
      MString(SimplePattern("[abc]{1, 10}"), SimplePattern("[abc]{15}")),
      GenerateThrowsGenerationError("", Context()));
}

TEST(MStringTest, GenerateWithoutSimplePatternOrLengthOrAlphabetShouldFail) {
  // No simple pattern and no alphabet
  EXPECT_THAT(MString(), GenerateThrowsGenerationError("", Context()));
  EXPECT_THAT([] { MString(Alphabet("")); },
              ThrowsImpossibleToSatisfy("only the characters ``"));

  // Has Alphabet, but not simple pattern or length.
  EXPECT_THAT(MString(), GenerateThrowsGenerationError("", Context()));
}

TEST(MStringTest, SimplePatternWorksForGeneration) {
  EXPECT_THAT(MString(SimplePattern("[abc]{10, 20}")),
              GeneratedValuesAre(MatchesRegex("[abc]{10,20}")));
}

TEST(MStringTest, IsSatisfiedWithShouldCheck) {
  EXPECT_THAT(MString(SimplePattern("[abc]{10, 20}")),
              IsSatisfiedWith("abcabcabca"));
  EXPECT_THAT(MString(SimplePattern("[abc]{10, 20}")),
              IsNotSatisfiedWith("ABCABCABCA", "simple pattern"));
}

TEST(MStringTest, SimplePatternWithWildcardsShouldFailGeneration) {
  EXPECT_THAT(MString(SimplePattern("a*")),
              GenerateThrowsGenerationError("", Context()));
  EXPECT_THAT(MString(SimplePattern("a+")),
              GenerateThrowsGenerationError("", Context()));
}

TEST(MStringTest, SimplePatternWithWildcardsShouldWorkForIsSatisfiedWith) {
  EXPECT_THAT(MString(SimplePattern("a*")), IsSatisfiedWith("aaaaaa"));
  EXPECT_THAT(MString(SimplePattern("a+")), IsSatisfiedWith("aaaaaa"));

  EXPECT_THAT(MString(SimplePattern("a*")), IsSatisfiedWith(""));
  EXPECT_THAT(MString(SimplePattern("a+")),
              IsNotSatisfiedWith("", "simple pattern"));
}

TEST(MStringTest, SimplePatternShouldRespectAlphabets) {
  // Odds are that random cases should generate at least some `a`s. But our
  // alphabet is only "b", so the only correct answer is the string "b".
  EXPECT_THAT(MString(SimplePattern("a{0, 123456}b"), Alphabet("b")),
              GeneratedValuesAre("b"));
}

TEST(MStringTest, ListEdgeCasesContainsLengthCases) {
  EXPECT_THAT(GenerateEdgeCases(MString(Length(Between(0, 10)), Alphabet("a"))),
              IsSupersetOf({"", "a", "aaaaaaaaaa"}));
}

TEST(MStringTest, ListEdgeCasesNoLengthFails) {
  EXPECT_THAT([] { (void)GenerateEdgeCases(MString()); },
              Throws<ConfigurationError>());
}

// FIXME: Decide how we want to test ToString.
// TEST(MStringTest, ToStringWorks) {
//   EXPECT_THAT(MString().ToString(), HasSubstr("MString"));
//   EXPECT_THAT(MString(Alphabet("abx")).ToString(), HasSubstr("abx"));
//   EXPECT_THAT(MString(Length(Between(1, 10))).ToString(), HasSubstr("[1,
//   10]")); EXPECT_THAT(MString(DistinctCharacters()).ToString(),
//               HasSubstr("istinct"));  // [D|d]istinct
//   EXPECT_THAT(MString(SimplePattern("[abc]{10, 20}")).ToString(),
//               HasSubstr("[abc]{10,20}"));
// }

}  // namespace
}  // namespace moriarty
