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

#include "src/internal/simple_pattern.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/internal/expressions.h"
#include "src/internal/random_engine.h"
#include "src/testing/gtest_helpers.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace moriarty_internal {

// Some PrintTo's for nicer debugging experience.
void PrintTo(PatternNode::SubpatternType type, std::ostream* os) {
  *os << (type == PatternNode::SubpatternType::kAnyOf ? "AnyOf" : "AllOf");
}

void PrintTo(const PatternNode& node, std::ostream* os, int depth = 0) {
  *os << std::string(depth, ' ') << node.pattern << " ";
  PrintTo(node.subpattern_type, os);
  *os << "\n";
  for (const PatternNode& subpattern : node.subpatterns)
    PrintTo(subpattern, os, depth + 1);
}

namespace {

using ::moriarty_testing::Context;
using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::Eq;
using ::testing::ExplainMatchResult;
using ::testing::Field;
using ::testing::FieldsAre;
using ::testing::Ge;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Le;
using ::testing::Matches;
using ::testing::Not;
using ::testing::Optional;
using ::testing::SizeIs;
using ::testing::Throws;
using ::testing::ThrowsMessage;

SimplePattern::RandFn Random() {
  RandomEngine random_engine({1, 2, 3, 4}, "v0.1");
  return [random_engine](int64_t lo, int64_t hi) mutable {
    return random_engine.RandInt(lo, hi);
  };
}

Expression::LookupFn EmptyLookup() {
  return [](std::string_view) -> int64_t {
    throw std::runtime_error("Empty lookup function");
  };
}

MATCHER_P(SimplePatternMatches, str, "") {
  std::string pattern{arg};  // Cast immediately to get better compile messages.

  try {
    (void)SimplePattern(pattern);
  } catch (const std::invalid_argument& e) {
    *result_listener << "Invalid Pattern: " << e.what();
    return false;
  }

  SimplePattern simple_pattern = SimplePattern(pattern);
  if (simple_pattern.Matches(str, EmptyLookup())) {
    *result_listener << "matches " << str;
    return true;
  }
  *result_listener << "does not match " << str;
  return false;
}

MATCHER_P2(SimplePatternMatches, str, context, "") {
  std::string pattern{arg};  // Cast immediately to get better compile messages.
  Context ctx{context};

  try {
    (void)SimplePattern(pattern);
  } catch (const std::invalid_argument& e) {
    *result_listener << "Invalid Pattern: " << e.what();
    return false;
  }

  auto lookup = [&ctx](std::string_view var) -> int64_t {
    return ctx.Values().Get<MInteger>(var);
  };
  SimplePattern simple_pattern = SimplePattern(pattern);
  if (simple_pattern.Matches(str, lookup)) {
    *result_listener << "matches " << str;
    return true;
  }
  *result_listener << "does not match " << str;
  return false;
}

MATCHER_P(IsInvalidSimplePattern, reason, "") {
  std::string pattern{arg};  // Cast immediately to get better compile messages.

  try {
    (void)SimplePattern(pattern);
    *result_listener << "Expected invalid pattern, but got a valid one: "
                     << pattern;
    return false;
  } catch (const std::invalid_argument& e) {
    if (Matches(testing::HasSubstr(reason))(e.what())) {
      *result_listener << "Invalid Pattern (in a good way): " << e.what();
      return true;
    }
    *result_listener << "Invalid Pattern, but not for the right reason: "
                     << e.what();
    return false;
  }
}

TEST(SimplePatternTest, BasicChecksShouldWork) {
  EXPECT_THAT("abc", SimplePatternMatches("abc"));
  EXPECT_THAT("a*", SimplePatternMatches("a"));
  EXPECT_THAT("a*b+", SimplePatternMatches("ab"));
  EXPECT_THAT("a|b|c", SimplePatternMatches("a"));
  EXPECT_THAT("a(bc)", SimplePatternMatches("abc"));
}

TEST(SimplePatternTest, CreateShouldRejectBadPatterns) {
  EXPECT_THAT("ab)", IsInvalidSimplePattern(")"));
  EXPECT_THAT("(*)", IsInvalidSimplePattern("*"));
  EXPECT_THAT("*", IsInvalidSimplePattern("*"));
}

TEST(SimplePatternTest, CreateShouldRejectEmptyPattern) {
  EXPECT_THAT("", IsInvalidSimplePattern("mpty"));
}

TEST(SimplePatternTest, PatternShouldMatchSimpleCase) {
  EXPECT_THAT("a", SimplePatternMatches("a"));
  EXPECT_THAT("abc", SimplePatternMatches("abc"));
}

TEST(SimplePatternTest, PatternShouldMatchSimpleOrCase) {
  EXPECT_THAT("a|b|c", SimplePatternMatches("a"));
  EXPECT_THAT("a|b|c", SimplePatternMatches("b"));
  EXPECT_THAT("a|b|c", SimplePatternMatches("c"));
}

TEST(SimplePatternTest, NestedPatternShouldMatch) {
  EXPECT_THAT("(a|b|c)(d|e|f)", SimplePatternMatches("ce"));
  EXPECT_THAT("(a|b|c)(d|e|f)", SimplePatternMatches("af"));
  EXPECT_THAT("(a|b|c)(d|e|f)", Not(SimplePatternMatches("bx")));
  EXPECT_THAT("(a|b|c)(d|e|f)", Not(SimplePatternMatches("xe")));
}

TEST(SimplePatternTest, DotWildcardDoesNotExist) {
  EXPECT_THAT(".", SimplePatternMatches("."));
  EXPECT_THAT(".", Not(SimplePatternMatches("a")));
}

TEST(SimplePatternTest, StarWildcardsShouldMatch) {
  EXPECT_THAT("a*", SimplePatternMatches(""));
  EXPECT_THAT("a*", SimplePatternMatches("a"));
  EXPECT_THAT("a*", SimplePatternMatches("aa"));
  EXPECT_THAT("a*", SimplePatternMatches("aaa"));

  EXPECT_THAT("a*", Not(SimplePatternMatches("x")));
  EXPECT_THAT("a*", Not(SimplePatternMatches("aax")));
}

TEST(SimplePatternTest, PlusWildcardsShouldMatch) {
  EXPECT_THAT("a+", Not(SimplePatternMatches("")));
  EXPECT_THAT("a+", SimplePatternMatches("a"));
  EXPECT_THAT("a+", SimplePatternMatches("aa"));
  EXPECT_THAT("a+", SimplePatternMatches("aaa"));

  EXPECT_THAT("a+", Not(SimplePatternMatches("x")));
  EXPECT_THAT("a+", Not(SimplePatternMatches("aax")));
}

TEST(SimplePatternTest, QuestionMarkWildcardsShouldMatch) {
  EXPECT_THAT("a?", SimplePatternMatches(""));
  EXPECT_THAT("a?", SimplePatternMatches("a"));
  EXPECT_THAT("a?", Not(SimplePatternMatches("aa")));

  EXPECT_THAT("a?", Not(SimplePatternMatches("x")));
  EXPECT_THAT("a?", Not(SimplePatternMatches("aax")));
}

TEST(SimplePatternTest, ExactLengthWildcardsShouldMatch) {
  EXPECT_THAT("a{2}", Not(SimplePatternMatches("")));
  EXPECT_THAT("a{2}", Not(SimplePatternMatches("a")));
  EXPECT_THAT("a{2}", SimplePatternMatches("aa"));
  EXPECT_THAT("a{2}", Not(SimplePatternMatches("aaa")));

  EXPECT_THAT("a{2}", Not(SimplePatternMatches("x")));
  EXPECT_THAT("a{2}", Not(SimplePatternMatches("aax")));
}

TEST(SimplePatternTest, RangeLengthWildcardsShouldMatch) {
  EXPECT_THAT("a{1, 2}", Not(SimplePatternMatches("")));
  EXPECT_THAT("a{1, 2}", SimplePatternMatches("a"));
  EXPECT_THAT("a{1, 2}", SimplePatternMatches("aa"));
  EXPECT_THAT("a{1, 2}", Not(SimplePatternMatches("aaa")));

  EXPECT_THAT("a{1, 2}", Not(SimplePatternMatches("x")));
  EXPECT_THAT("a{1, 2}", Not(SimplePatternMatches("aax")));
}
TEST(SimplePatternTest, UpperBoundWildcardsShouldMatch) {
  EXPECT_THAT("a{,2}", SimplePatternMatches(""));
  EXPECT_THAT("a{,2}", SimplePatternMatches("a"));
  EXPECT_THAT("a{,2}", SimplePatternMatches("aa"));
  EXPECT_THAT("a{,2}", Not(SimplePatternMatches("aaa")));

  EXPECT_THAT("a{,2}", Not(SimplePatternMatches("x")));
  EXPECT_THAT("a{,2}", Not(SimplePatternMatches("aax")));
}

TEST(SimplePatternTest, LowerBoundWildcardsShouldMatch) {
  EXPECT_THAT("a{2,}", Not(SimplePatternMatches("")));
  EXPECT_THAT("a{2,}", Not(SimplePatternMatches("a")));
  EXPECT_THAT("a{2,}", SimplePatternMatches("aa"));
  EXPECT_THAT("a{2,}", SimplePatternMatches("aaa"));

  EXPECT_THAT("a{2,}", Not(SimplePatternMatches("x")));
  EXPECT_THAT("a{2,}", Not(SimplePatternMatches("aax")));
}

TEST(SimplePatternTest, CharacterSetsSimpleCaseShouldWork) {
  EXPECT_THAT("[abc]", SimplePatternMatches("a"));
  EXPECT_THAT("[abc]", SimplePatternMatches("b"));
  EXPECT_THAT("[abc]", SimplePatternMatches("c"));
  EXPECT_THAT("[abc]", Not(SimplePatternMatches("d")));
  EXPECT_THAT("[abc]", Not(SimplePatternMatches("ab")));
}

TEST(SimplePatternTest, ConcatenatedCharacterSetsSimpleCaseShouldWork) {
  EXPECT_THAT("[abc][def]", SimplePatternMatches("ad"));
  EXPECT_THAT("[abc][def]", SimplePatternMatches("bf"));
  EXPECT_THAT("[abc][def]", SimplePatternMatches("cd"));
  EXPECT_THAT("[abc][def]", Not(SimplePatternMatches("ax")));
  EXPECT_THAT("[abc][def]", Not(SimplePatternMatches("dd")));
}

TEST(SimplePatternTest, CharacterSetsWithRangesSimpleCaseShouldWork) {
  EXPECT_THAT("[a-f]", SimplePatternMatches("a"));
  EXPECT_THAT("[a-f]", SimplePatternMatches("b"));
  EXPECT_THAT("[a-f]", SimplePatternMatches("f"));
  EXPECT_THAT("[a-f]", Not(SimplePatternMatches("g")));
  EXPECT_THAT("[a-f]", Not(SimplePatternMatches("x")));
}

TEST(SimplePatternTest,
     CharacterSetsWithRangesAndWildcardsSimpleCaseShouldWork) {
  EXPECT_THAT("[a-f]*", SimplePatternMatches(""));
  EXPECT_THAT("[a-f]*", SimplePatternMatches("aaa"));
  EXPECT_THAT("[a-f]*", SimplePatternMatches("ba"));
  EXPECT_THAT("[a-f]*", SimplePatternMatches("deadbeef"));
  EXPECT_THAT("[a-f]*", Not(SimplePatternMatches("xxx")));
  EXPECT_THAT("[a-f]*", Not(SimplePatternMatches("0")));
}

TEST(SimplePatternTest, RecursiveGroupingsShouldWork) {
  EXPECT_THAT("((hello|bye)world)", SimplePatternMatches("helloworld"));
  EXPECT_THAT("((hello|bye)world)", SimplePatternMatches("byeworld"));
  EXPECT_THAT("((hello|bye)world)", Not(SimplePatternMatches("hello")));
  EXPECT_THAT("((hello|bye)world)", Not(SimplePatternMatches("bye")));
  EXPECT_THAT("((hello|bye)world)", Not(SimplePatternMatches("world")));
}

TEST(SimplePatternTest, GroupingsAndAnOrClauseShouldWork) {
  EXPECT_THAT("((hello|bye)|other)", Not(SimplePatternMatches("helloother")));
  EXPECT_THAT("((hello|bye)|other)", Not(SimplePatternMatches("byeother")));
  EXPECT_THAT("((hello|bye)|other)", SimplePatternMatches("hello"));
  EXPECT_THAT("((hello|bye)|other)", SimplePatternMatches("bye"));
  EXPECT_THAT("((hello|bye)|other)", SimplePatternMatches("other"));
}

TEST(SimplePatternTest, GroupingWithWildcardsShouldFail) {
  EXPECT_THAT("(ac)*", IsInvalidSimplePattern("*"));
}

TEST(SimplePatternTest, InvalidSyntaxShouldFail) {
  EXPECT_THAT("|", IsInvalidSimplePattern("|"));
  EXPECT_THAT("*", IsInvalidSimplePattern("*"));
  EXPECT_THAT("ab)", IsInvalidSimplePattern(")"));
  EXPECT_THAT("[", IsInvalidSimplePattern("["));
  EXPECT_THAT("]", IsInvalidSimplePattern("]"));
}

TEST(SimplePatternTest, SquareBracketsShouldWork) {
  EXPECT_THAT("[a][b][X][Y][Z][@][!]", SimplePatternMatches("abXYZ@!"));
  EXPECT_THAT("[(][)][*][[][]][?][+]", SimplePatternMatches("()*[]?+"));
}

TEST(SimplePatternTest, NonEscapedSpacesInCharSetShouldBeIgnored) {
  EXPECT_THAT("a[b c]d", SimplePatternMatches("abd"));
  EXPECT_THAT("a[b c]d", SimplePatternMatches("acd"));
  EXPECT_THAT("a[b c]d", Not(SimplePatternMatches("a d")));
}

TEST(SimplePatternTest, NonEscapedSpacesOutsideCharSetShouldBeIgnored) {
  EXPECT_THAT("a bc", SimplePatternMatches("abc"));
  EXPECT_THAT("a bc", Not(SimplePatternMatches("a bc")));
}

TEST(SimplePatternTest, EscapedSpacesShouldWork) {
  EXPECT_THAT(R"(a[b\ c]d)", SimplePatternMatches("abd"));
  EXPECT_THAT(R"(a[b\ c]d)", SimplePatternMatches("acd"));
  EXPECT_THAT(R"(a[b\ c]d)", SimplePatternMatches("a d"));
}

TEST(SimplePatternTest, EscapedBackslashShouldWork) {
  EXPECT_THAT(R"(a[b\\c]d)", SimplePatternMatches("abd"));
  EXPECT_THAT(R"(a[b\\c]d)", SimplePatternMatches("acd"));
  EXPECT_THAT(R"(a[b\\c]d)", SimplePatternMatches(R"(a\d)"));
}

TEST(SimplePatternTest, InvalidEscapeCharactersShouldFail) {
  EXPECT_THAT(R"(abc\)", IsInvalidSimplePattern("escape"));
  EXPECT_THAT(R"(\n)", IsInvalidSimplePattern("escape"));
  EXPECT_THAT(R"(\r)", IsInvalidSimplePattern("escape"));
  EXPECT_THAT(R"(\t)", IsInvalidSimplePattern("escape"));
  EXPECT_THAT(R"(\x)", IsInvalidSimplePattern("escape"));
  EXPECT_THAT(R"(\a)", IsInvalidSimplePattern("escape"));
  EXPECT_THAT(R"(\b)", IsInvalidSimplePattern("escape"));
}

TEST(SimplePatternTest, SimpleVariablesInRangeSizesShouldWork) {
  EXPECT_THAT("a{N}",
              SimplePatternMatches("a", Context().WithValue<MInteger>("N", 1)));
  EXPECT_THAT("a{N}", Not(SimplePatternMatches(
                          "aa", Context().WithValue<MInteger>("N", 1))));

  EXPECT_THAT("a{N}", Not(SimplePatternMatches(
                          "a", Context().WithValue<MInteger>("N", 2))));
  EXPECT_THAT("a{N}", SimplePatternMatches(
                          "aa", Context().WithValue<MInteger>("N", 2)));
  EXPECT_THAT(
      "a{N, M}",
      SimplePatternMatches(
          "aa",
          Context().WithValue<MInteger>("N", 1).WithValue<MInteger>("M", 2)));
  EXPECT_THAT(
      "a{N, M - 1}",
      Not(SimplePatternMatches(
          "aa",
          Context().WithValue<MInteger>("N", 1).WithValue<MInteger>("M", 2))));
}

TEST(SimplePatternTest, FunctionsInRangeSizesShouldWork) {
  EXPECT_THAT("a{abs(N)}", SimplePatternMatches(
                               "a", Context().WithValue<MInteger>("N", -1)));

  Context ctx =
      Context().WithValue<MInteger>("N", 1).WithValue<MInteger>("M", 2);
  EXPECT_THAT("a{max(M, N)}", AllOf(Not(SimplePatternMatches("a", ctx)),
                                    SimplePatternMatches("aa", ctx)));
  EXPECT_THAT("a{1, min(M, N)}", AllOf(SimplePatternMatches("a", ctx),
                                       Not(SimplePatternMatches("aa", ctx))));
  EXPECT_THAT("a{max(M, N), 2}", AllOf(Not(SimplePatternMatches("a", ctx)),
                                       SimplePatternMatches("aa", ctx)));
  EXPECT_THAT(
      "a{min(M, N), max(N, M)}",
      AllOf(SimplePatternMatches("a", ctx), SimplePatternMatches("aa", ctx)));
}

// GeneratedStringsAre() [for use with GoogleTest]
//
// Checks if the subpatterns of a PatternNode are as expected.
//
// Example usage:
//
// EXPECT_THAT(SimplePattern("regex"),
//             GeneratedStringsAre(SizeIs(Le(10)));
MATCHER_P(GeneratedStringsAre, matcher, "") {
  std::string str{arg};  // Cast immediately to get better compile messages.
  SimplePattern p = SimplePattern(str);

  for (int tries = 0; tries < 10; tries++) {
    std::string value = p.Generate(EmptyLookup(), Random());

    if (!Matches(matcher)(value)) {
      return ExplainMatchResult(matcher, value, result_listener);
    }
    if (!p.Matches(value, EmptyLookup())) {
      *result_listener << "Generated value does not match pattern: " << value;
      return false;
    }
  }
  return true;
}

MATCHER_P2(GeneratedStringsAre, matcher, context, "") {
  std::string str{arg};  // Cast immediately to get better compile messages.
  Context ctx{context};
  SimplePattern p = SimplePattern(str);

  auto lookup = [&ctx](std::string_view var) -> int64_t {
    return ctx.Values().Get<MInteger>(var);
  };
  for (int tries = 0; tries < 10; tries++) {
    std::string value = p.Generate(lookup, Random());

    if (!Matches(matcher)(value)) {
      return ExplainMatchResult(matcher, value, result_listener);
    }
    if (!p.Matches(value, lookup)) {
      *result_listener << "Generated value does not match pattern: " << value;
      return false;
    }
  }
  return true;
}

MATCHER_P2(GeneratedValuesWithRestrictionsAre, matcher, restricted_alphabet,
           "") {
  std::string str{arg};  // Cast immediately to get better compile messages.
  std::string restricted_alphabet_copy{restricted_alphabet};
  SimplePattern p = SimplePattern(str);

  for (int tries = 0; tries < 10; tries++) {
    std::string value = p.GenerateWithRestrictions(restricted_alphabet_copy,
                                                   EmptyLookup(), Random());
    if (!Matches(matcher)(value)) {
      return ExplainMatchResult(matcher, value, result_listener);
    }
    if (!p.Matches(value, EmptyLookup())) {
      *result_listener << "Generated value does not match pattern: " << value;
      return false;
    }
  }
  return true;
}

TEST(SimplePatternTest, SimpleGenerationWorks) {
  SimplePattern p = SimplePattern("a");
  for (int tries = 0; tries < 10; tries++) {
    EXPECT_EQ(p.Generate(EmptyLookup(), Random()), "a");
  }
}

TEST(SimplePatternTest, GenerationWithOrClauseShouldWork) {
  EXPECT_THAT("a|b|c", GeneratedStringsAre(AnyOf("a", "b", "c")));
}

TEST(SimplePatternTest, GenerationWithComplexOrClauseShouldWork) {
  EXPECT_THAT("a|bb|ccc", GeneratedStringsAre(AnyOf("a", "bb", "ccc")));
}

TEST(SimplePatternTest, GenerationWithConcatenationShouldWork) {
  EXPECT_THAT("abcde", GeneratedStringsAre("abcde"));
}

TEST(SimplePatternTest, GenerationWithLengthRangesGivesProperLengths) {
  EXPECT_THAT("a{4, 8}", GeneratedStringsAre(SizeIs(AllOf(Ge(4), Le(8)))));
  EXPECT_THAT("a{,8}", GeneratedStringsAre(SizeIs(Le(8))));
  EXPECT_THAT("a{8}", GeneratedStringsAre(SizeIs(8)));
}

TEST(SimplePatternTest, GenerationWithNestedSubExpressionsShouldWork) {
  EXPECT_THAT("a(b|c)(d|e)",
              GeneratedStringsAre(AnyOf("abd", "abe", "acd", "ace")));
}

TEST(SimplePatternTest, GenerationWithLargeWildcardShouldThrowError) {
  SimplePattern p1 = SimplePattern("a+");
  EXPECT_THAT([&] { (void)p1.Generate(EmptyLookup(), Random()); },
              ThrowsMessage<std::runtime_error>(
                  AllOf(HasSubstr("generate"), HasSubstr("+"))));

  SimplePattern p2 = SimplePattern("a*");
  EXPECT_THAT([&] { (void)p2.Generate(EmptyLookup(), Random()); },
              ThrowsMessage<std::runtime_error>(
                  AllOf(HasSubstr("generate"), HasSubstr("*"))));
}

TEST(SimplePatternTest, GenerationWithSmallWildcardShouldWork) {
  EXPECT_THAT("a?b", GeneratedStringsAre(AnyOf("ab", "b")));
}

TEST(SimplePatternTest, GenerationWithAlphabetRestrictionsShouldWork) {
  // Restricting the alphabet to only 'f's should only be able to generate one
  // string.
  EXPECT_THAT("[a-z]{8}", GeneratedValuesWithRestrictionsAre("ffffffff", "f"));

  EXPECT_THAT("[a-z]{2}", GeneratedValuesWithRestrictionsAre(
                              AnyOf("ff", "fx", "xf", "xx"), "fx"));
}

TEST(SimplePatternTest,
     GenerationWithVeryStrongRestrictionsShouldAttemptEmptyString) {
  // Restricting the alphabet to only 'x' means that only the empty string is
  // valid.
  EXPECT_THAT("[a-f]{0, 10}", GeneratedValuesWithRestrictionsAre("", "x"));

  // If the pattern doesn't allow the empty string, ensure it fails.
  SimplePattern p = SimplePattern("[a-f]{1, 10}");
  EXPECT_THAT(
      [&] {
        (void)p.GenerateWithRestrictions(/*restricted_alphabet = */ "x",
                                         EmptyLookup(), Random());
      },
      ThrowsMessage<std::invalid_argument>(
          HasSubstr("No valid characters for generation, but empty string is "
                    "not allowed.")));
}

TEST(SimplePatternTest, GeneratingVariablesInRangeSizesShouldWork) {
  EXPECT_THAT("a{N}",
              GeneratedStringsAre("a", Context().WithValue<MInteger>("N", 1)));
  EXPECT_THAT("a{N}",
              GeneratedStringsAre("aa", Context().WithValue<MInteger>("N", 2)));

  EXPECT_THAT(
      "a{N, M}",
      GeneratedStringsAre(
          AnyOf("a", "aa"),
          Context().WithValue<MInteger>("N", 1).WithValue<MInteger>("M", 2)));
}

TEST(SimplePatternTest, GeneratingWithFunctionsInRangeSizesShouldWork) {
  EXPECT_THAT("a{abs(N)}",
              GeneratedStringsAre("a", Context().WithValue<MInteger>("N", -1)));

  Context ctx =
      Context().WithValue<MInteger>("N", 1).WithValue<MInteger>("M", 2);
  EXPECT_THAT("a{max(M, N)}", GeneratedStringsAre("aa", ctx));
  EXPECT_THAT("a{1, min(M, N)}", GeneratedStringsAre("a", ctx));
  EXPECT_THAT("a{max(M, N), 2}", GeneratedStringsAre("aa", ctx));
  EXPECT_THAT("a{min(M, N-1), max(N, M)}",
              GeneratedStringsAre(AnyOf("", "a", "aa"), ctx));
}

// TODO: Add tests for Generation failures when values are not known or produce
// invalid ranges.

// -----------------------------------------------------------------------------
//  Tests below here are for more internal-facing functions. Only functions
//  above are for the external API.

// SubpatternsAre() [for use with GoogleTest]
//
// Checks if the subpatterns of a PatternNode are as expected.
//
// Example usage:
//
// EXPECT_THAT(your_code(),
// SubpatternsAre(PatternNode::SubpatternType::kAnyOf,
//                                         {"a", "b|c", "d"}));
MATCHER_P2(SubpatternsAre, expected_type, expected_subpatterns, "") {
  if (arg.subpattern_type != expected_type) {
    return ExplainMatchResult(Eq(expected_type), arg.subpattern_type,
                              result_listener);
  }

  if (arg.subpatterns.size() != expected_subpatterns.size()) {
    return ExplainMatchResult(SizeIs(expected_subpatterns.size()),
                              arg.subpatterns, result_listener);
  }

  for (int i = 0; i < arg.subpatterns.size(); i++) {
    if (arg.subpatterns[i].pattern != expected_subpatterns[i]) {
      return ExplainMatchResult(Eq(expected_subpatterns[i]),
                                arg.subpatterns[i].pattern, result_listener);
    }
  }

  return true;
}

MATCHER_P(AcceptsOnly, good_chars, "") {
  RepeatedCharSet r{arg};  // Convert immediately to get a nicer compile message
  std::string good_chars_copy = good_chars;

  for (char c : good_chars_copy) {
    if (!r.IsValidCharacter(c)) {
      *result_listener << "Failed to accept character: " << c;
      return false;
    }
  }

  for (int c = 0; c < 255; c++) {
    if (good_chars_copy.find(char(c)) == std::string::npos) {
      if (r.IsValidCharacter(char(c))) {
        *result_listener << "Unexpected accepted character: " << c;
        return false;
      }
    }
  }
  return true;
}

MATCHER_P(AcceptsAllBut, bad_chars, "") {
  RepeatedCharSet r{arg};  // Convert immediately to get a nicer compile message
  std::string bad_chars_copy = bad_chars;

  for (char c : bad_chars_copy) {
    if (r.IsValidCharacter(c)) {
      *result_listener << "Accepted character: " << c;
      return false;
    }
  }

  for (int c = 0; c < 128; c++) {
    if (bad_chars_copy.find(char(c)) == std::string::npos) {
      if (!r.IsValidCharacter(char(c))) {
        *result_listener << "Unexpected unaccepted character: " << c;
        return false;
      }
    }
  }
  return true;
}

TEST(RepeatedCharSetTest, DefaultShouldNotAcceptAnyChars) {
  EXPECT_THAT(RepeatedCharSet(), AcceptsOnly(""));
}

TEST(RepeatedCharSetTest, AddedCharactersShouldBeAccepted) {
  RepeatedCharSet r;
  EXPECT_TRUE(r.Add('a'));
  EXPECT_TRUE(r.Add('b'));
  EXPECT_THAT(r, AcceptsOnly("ab"));
}

TEST(RepeatedCharSetTest, DefaultLengthShouldBeZeroAndInfinite) {
  EXPECT_THAT(RepeatedCharSet().Extremes(EmptyLookup()),
              FieldsAre(0, std::numeric_limits<int64_t>::max()));
}

TEST(RepeatedCharSetTest, FlippingDefaultShouldAcceptEverything) {
  RepeatedCharSet r;
  r.FlipValidCharacters();
  EXPECT_THAT(r, AcceptsAllBut(""));
}

TEST(RepeatedCharSetTest, FlippingShouldTurnOffAddedCharacters) {
  RepeatedCharSet r;
  EXPECT_TRUE(r.Add('a'));
  r.FlipValidCharacters();
  EXPECT_THAT(r, AcceptsAllBut("a"));
}

TEST(RepeatedCharSetTest, AddingAgainAfterFlipShouldBeOk) {
  RepeatedCharSet r;
  EXPECT_TRUE(r.Add('a'));
  r.FlipValidCharacters();
  EXPECT_THAT(r, AcceptsAllBut("a"));
  EXPECT_TRUE(r.Add('a'));
  EXPECT_THAT(r, AcceptsAllBut(""));
}

TEST(RepeatedCharSetTest, LongestValidPrefixShouldSucceedForLengthZero) {
  EXPECT_EQ(RepeatedCharSet().LongestValidPrefix("a", EmptyLookup()), 0);
}

TEST(RepeatedCharSetTest, LongestValidPrefixShouldSucceedForLengthOneGood) {
  RepeatedCharSet r;
  r.SetRange(Expression("1"), Expression("1"));
  EXPECT_TRUE(r.Add('a'));
  EXPECT_THAT(r.LongestValidPrefix("a", EmptyLookup()), Optional(1));
}

TEST(RepeatedCharSetTest, LongestValidPrefixShouldFailForLengthOneBad) {
  RepeatedCharSet r;
  r.SetRange(Expression("1"), Expression("1"));
  EXPECT_TRUE(r.Add('a'));
  EXPECT_EQ(r.LongestValidPrefix("b", EmptyLookup()), std::nullopt);
}

TEST(RepeatedCharSetTest, LongestValidPrefixShouldSucceedForLongerStrings) {
  RepeatedCharSet r;
  r.SetRange(Expression("1"), Expression("4"));
  EXPECT_TRUE(r.Add('a'));
  EXPECT_THAT(r.LongestValidPrefix("aaaaaaaaaaa", EmptyLookup()), Optional(4));
}

TEST(RepeatedCharSetTest, LongestValidPrefixShouldFailForShorterStrings) {
  RepeatedCharSet r;
  r.SetRange(Expression("3"), Expression("4"));
  EXPECT_TRUE(r.Add('a'));
  EXPECT_EQ(r.LongestValidPrefix("aa", EmptyLookup()), std::nullopt);
}

TEST(RepeatedCharSetTest, NegativeMinRangeShouldBeOk) {
  RepeatedCharSet r;
  EXPECT_TRUE(r.Add('a'));
  r.SetRange(Expression("-5"), Expression("3"));
  EXPECT_THAT(r.Extremes(EmptyLookup()), FieldsAre(0, 3));
}

// FIXME: Determine if we can crash sooner on cases like these.
// TEST(RepeatedCharSetTest, InvalidRangesShouldFail) {
//   EXPECT_THAT([] { RepeatedCharSet().SetRange(10, 5); },
//               Throws<std::invalid_argument>());
//   // Min is clamped to 0, so this is invalid.
//   EXPECT_THAT([] { RepeatedCharSet().SetRange(-5, -3); },
//               Throws<std::invalid_argument>());
// }

TEST(RepeatedCharSetTest, NegativeMinZeroMaxRangeShouldBeOk) {
  RepeatedCharSet r;
  r.SetRange(Expression("-5"), Expression("0"));
  EXPECT_THAT(r.Extremes(EmptyLookup()), FieldsAre(0, 0));
}

TEST(RepeatedCharSetTest, AddingANegativeCharShouldFail) {
  RepeatedCharSet r;
  EXPECT_THAT([&] { (void)r.Add(static_cast<char>(-1)); },
              Throws<std::invalid_argument>());
}

TEST(RepeatedCharSetTest, FlipShouldNotAcceptNegativeChars) {
  RepeatedCharSet r;
  r.FlipValidCharacters();
  EXPECT_FALSE(r.IsValidCharacter(static_cast<char>(-1)));
}

TEST(RepeatedCharSetTest, FlipShouldAcceptNullChar) {
  RepeatedCharSet r;
  r.FlipValidCharacters();
  EXPECT_TRUE(r.IsValidCharacter(static_cast<char>(0)));
}

TEST(RepeatedCharSetTest, AddingCharactersMultipleTimesShouldReturnFalse) {
  RepeatedCharSet r;
  EXPECT_TRUE(r.Add('a'));
  EXPECT_FALSE(r.Add('a'));
}

TEST(ParseCharSetTest, EmptyCharacterSetsShouldFail) {
  EXPECT_THAT([] { (void)CharacterSetPrefixLength(""); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("Empty")));
}

TEST(ParseCharSetTest, SingleCharacterCharacterSetsShouldBeOk) {
  EXPECT_EQ(CharacterSetPrefixLength("a"), 1);
  EXPECT_EQ(CharacterSetPrefixLength("ab"), 1);
  EXPECT_EQ(CharacterSetPrefixLength("a["), 1);
}

TEST(ParseCharSetTest, TypicalCasesShouldWork) {
  EXPECT_EQ(CharacterSetPrefixLength("[a]"), 3);
  EXPECT_EQ(CharacterSetPrefixLength("[ab]"), 4);
}

TEST(ParseCharSetTest, CharactersAfterCloseShouldBeIgnored) {
  EXPECT_EQ(CharacterSetPrefixLength("[a]xx"), 3);
  EXPECT_EQ(CharacterSetPrefixLength("[ab]xx"), 4);
}

TEST(ParseCharSetTest, OpenSquareBraceShouldBeAccepted) {
  EXPECT_EQ(CharacterSetPrefixLength("[[]"), 3);
}

TEST(ParseCharSetTest, CloseSquareBraceShouldBeAccepted) {
  EXPECT_EQ(CharacterSetPrefixLength("[]]"), 3);
}

TEST(ParseCharSetTest, OpenThenCloseSquareBraceShouldBeAccepted) {
  EXPECT_EQ(CharacterSetPrefixLength("[[]]"), 4);
}

TEST(ParseCharSetTest, CloseThenOpenSquareBraceShouldTakeEarlierOne) {
  EXPECT_EQ(CharacterSetPrefixLength("[][]"), 2);
}

TEST(ParseCharSetTest, AnyDuplicateCharacterShouldForceTheEarlierOne) {
  EXPECT_EQ(CharacterSetPrefixLength("[(][)][*][[][]][?][+]"), 3);
  EXPECT_EQ(CharacterSetPrefixLength("[)][*][[][]][?][+]"), 3);
  EXPECT_EQ(CharacterSetPrefixLength("[*][[][]][?][+]"), 3);
  EXPECT_EQ(CharacterSetPrefixLength("[[][]][?][+]"), 3);
  EXPECT_EQ(CharacterSetPrefixLength("[]][?][+]"), 3);
  EXPECT_EQ(CharacterSetPrefixLength("[?][+]"), 3);
  EXPECT_EQ(CharacterSetPrefixLength("[+]"), 3);
}

TEST(ParseCharSetTest, MissingCloseBraceShouldFail) {
  EXPECT_THAT([] { (void)CharacterSetPrefixLength("[hi"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("]")));
}

TEST(ParseCharSetTest, SpecialCharacterAtStartShouldFail) {
  EXPECT_THAT([] { (void)CharacterSetPrefixLength("]"); },
              Throws<std::invalid_argument>());
  EXPECT_THAT([] { (void)CharacterSetPrefixLength("("); },
              Throws<std::invalid_argument>());
  EXPECT_THAT([] { (void)CharacterSetPrefixLength(")"); },
              Throws<std::invalid_argument>());
  EXPECT_THAT([] { (void)CharacterSetPrefixLength("{"); },
              Throws<std::invalid_argument>());
  EXPECT_THAT([] { (void)CharacterSetPrefixLength("}"); },
              Throws<std::invalid_argument>());
  EXPECT_THAT([] { (void)CharacterSetPrefixLength("^"); },
              Throws<std::invalid_argument>());
  EXPECT_THAT([] { (void)CharacterSetPrefixLength("?"); },
              Throws<std::invalid_argument>());
  EXPECT_THAT([] { (void)CharacterSetPrefixLength("*"); },
              Throws<std::invalid_argument>());
  EXPECT_THAT([] { (void)CharacterSetPrefixLength("+"); },
              Throws<std::invalid_argument>());
  EXPECT_THAT([] { (void)CharacterSetPrefixLength("-"); },
              Throws<std::invalid_argument>());
  EXPECT_THAT([] { (void)CharacterSetPrefixLength("|"); },
              Throws<std::invalid_argument>());
}

TEST(ParseCharSetTest, EmptyCharSetBodyShouldFail) {
  EXPECT_THAT([] { (void)ParseCharacterSetBody(""); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("Empty")));
}

TEST(ParseCharSetTest, DuplicateCharactersInCharSetBodyShouldFail) {
  EXPECT_THAT([] { (void)ParseCharacterSetBody("aa"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("multiple")));
}

TEST(ParseCharSetTest, SquareBracesShouldBeOk) {
  EXPECT_THAT(ParseCharacterSetBody("]"), AcceptsOnly("]"));
  EXPECT_THAT(ParseCharacterSetBody("["), AcceptsOnly("["));
}

TEST(ParseCharSetTest, SquareBracesShouldBeInTheRightOrder) {
  EXPECT_THAT(ParseCharacterSetBody("[]"), AcceptsOnly("[]"));
  EXPECT_THAT([] { (void)ParseCharacterSetBody("]["); },
              ThrowsMessage<std::invalid_argument>(
                  testing::AllOf(HasSubstr("]"), HasSubstr("["))));
}

TEST(ParseCharSetTest, SpecialCharacterAsBodyShouldBeOk) {
  EXPECT_THAT(ParseCharacterSetBody("["), AcceptsOnly("["));
  EXPECT_THAT(ParseCharacterSetBody("]"), AcceptsOnly("]"));
  EXPECT_THAT(ParseCharacterSetBody("("), AcceptsOnly("("));
  EXPECT_THAT(ParseCharacterSetBody(")"), AcceptsOnly(")"));
  EXPECT_THAT(ParseCharacterSetBody("{"), AcceptsOnly("{"));
  EXPECT_THAT(ParseCharacterSetBody("}"), AcceptsOnly("}"));
  EXPECT_THAT(ParseCharacterSetBody("^"), AcceptsOnly("^"));
  EXPECT_THAT(ParseCharacterSetBody("?"), AcceptsOnly("?"));
  EXPECT_THAT(ParseCharacterSetBody("*"), AcceptsOnly("*"));
  EXPECT_THAT(ParseCharacterSetBody("+"), AcceptsOnly("+"));
  EXPECT_THAT(ParseCharacterSetBody("-"), AcceptsOnly("-"));
  EXPECT_THAT(ParseCharacterSetBody("|"), AcceptsOnly("|"));
}

TEST(ParseCharSetTest, SingleCaretShouldNotBeTreatedAsNegation) {
  RepeatedCharSet r = ParseCharacterSetBody("^");
  EXPECT_TRUE(r.IsValidCharacter('^'));
  EXPECT_FALSE(r.IsValidCharacter('a'));
}

TEST(ParseCharSetTest, StartingCaretShouldNotConsiderLaterCaretDuplicate) {
  EXPECT_THAT(ParseCharacterSetBody("^a"), AcceptsAllBut("a"));
  EXPECT_THAT(ParseCharacterSetBody("^ac"), AcceptsAllBut("ac"));
  EXPECT_THAT(ParseCharacterSetBody("^^"), AcceptsAllBut("^"));
}

TEST(ParseCharSetTest, TrailingNegativeSignShouldBeTreatedAsChar) {
  EXPECT_THAT(ParseCharacterSetBody("abc-"), AcceptsOnly("abc-"));
  EXPECT_THAT(ParseCharacterSetBody("-"), AcceptsOnly("-"));
}

TEST(ParseCharSetTest, NegativeSignInMiddleShouldBeTreatedAsRange) {
  EXPECT_THAT(ParseCharacterSetBody("m-q"), AcceptsOnly("mnopq"));
}

TEST(ParseCharSetTest, ValidRangeTypesShouldWork) {
  EXPECT_THAT(ParseCharacterSetBody("a-z"),
              AcceptsOnly("abcdefghijklmnopqrstuvwxyz"));
  EXPECT_THAT(ParseCharacterSetBody("A-Z"),
              AcceptsOnly("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
  EXPECT_THAT(ParseCharacterSetBody("0-9"), AcceptsOnly("0123456789"));
  EXPECT_THAT(
      ParseCharacterSetBody("a-zA-Z"),
      AcceptsOnly("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"));
  EXPECT_THAT(
      ParseCharacterSetBody("0-9a-zA-Z"),
      AcceptsOnly(
          "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"));
  EXPECT_THAT(
      ParseCharacterSetBody("a-zA-Z0-9"),
      AcceptsOnly(
          "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"));

  // Some partial
  EXPECT_THAT(ParseCharacterSetBody("d-g"), AcceptsOnly("defg"));
  EXPECT_THAT(ParseCharacterSetBody("A-C"), AcceptsOnly("ABC"));
  EXPECT_THAT(ParseCharacterSetBody("0-3"), AcceptsOnly("0123"));
}

TEST(ParseCharSetTest, InvalidRangeShouldFail) {
  EXPECT_THAT([] { (void)ParseCharacterSetBody("9-0"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("'-'")));
  EXPECT_THAT([] { (void)ParseCharacterSetBody("Z-A"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("'-'")));
  EXPECT_THAT([] { (void)ParseCharacterSetBody("z-a"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("'-'")));
  EXPECT_THAT([] { (void)ParseCharacterSetBody("a-A"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("'-'")));
  EXPECT_THAT([] { (void)ParseCharacterSetBody("A-a"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("'-'")));
  EXPECT_THAT([] { (void)ParseCharacterSetBody("A-z"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("'-'")));
  EXPECT_THAT([] { (void)ParseCharacterSetBody("A-?"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("'-'")));
}

TEST(ParseCharSetTest, DotsShouldNotBeTreatedAsWildcards) {
  EXPECT_THAT(ParseCharacterSetBody("."), AcceptsOnly("."));
  EXPECT_THAT(ParseCharacterSetBody(".ab"), AcceptsOnly(".ab"));
}

TEST(ParseCharSetTest, NegativeAtTheEndPlusRangesShouldNotCountAsDuplicate) {
  EXPECT_THAT(ParseCharacterSetBody("a-"), AcceptsOnly("a-"));
  EXPECT_THAT(ParseCharacterSetBody("a-d-"), AcceptsOnly("abcd-"));  // No dup
}

TEST(ParseCharSetTest, NegativeCharsShouldBeInvalid) {
  EXPECT_THAT(
      [] {
        (void)ParseCharacterSetBody(std::string(1, static_cast<char>(-1)));
      },
      Throws<std::invalid_argument>());
}

TEST(ParseRepetitionTest, EmptyRepetitionShouldPass) {
  EXPECT_EQ(RepetitionPrefixLength(""), 0);
}

TEST(ParseRepetitionTest, SingleRepetitionCharacterShouldReturnOne) {
  EXPECT_EQ(RepetitionPrefixLength("*"), 1);
  EXPECT_EQ(RepetitionPrefixLength("?"), 1);
  EXPECT_EQ(RepetitionPrefixLength("+"), 1);
}

TEST(ParseRepetitionTest, SingleNonSpecialCharacterShouldReturnZero) {
  EXPECT_EQ(RepetitionPrefixLength("a"), 0);
  EXPECT_EQ(RepetitionPrefixLength("abc"), 0);
  EXPECT_EQ(RepetitionPrefixLength("3"), 0);
}

TEST(ParseRepetitionTest, TypicalCasesShouldWork) {
  EXPECT_EQ(RepetitionPrefixLength("{1}"), 3);
  EXPECT_EQ(RepetitionPrefixLength("{1,2}"), 5);
  EXPECT_EQ(RepetitionPrefixLength("{1,}"), 4);
  EXPECT_EQ(RepetitionPrefixLength("{,2}"), 4);
}

TEST(ParseRepetitionTest, CharactersAfterCloseShouldBeIgnored) {
  EXPECT_EQ(RepetitionPrefixLength("{1}xx"), 3);
  EXPECT_EQ(RepetitionPrefixLength("{1,2}xx"), 5);
}

TEST(ParseRepetitionTest, MissingCloseBraceShouldFail) {
  EXPECT_THAT([] { (void)RepetitionPrefixLength("{1"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("No '}' found")));
}

TEST(ParseRepetitionTest, SpecialCharacterAtStartShouldReturnZero) {
  EXPECT_EQ(RepetitionPrefixLength("}"), 0);
  EXPECT_EQ(RepetitionPrefixLength("("), 0);
  EXPECT_EQ(RepetitionPrefixLength(")"), 0);
  EXPECT_EQ(RepetitionPrefixLength("["), 0);
  EXPECT_EQ(RepetitionPrefixLength("]"), 0);
  EXPECT_EQ(RepetitionPrefixLength("^"), 0);
  EXPECT_EQ(RepetitionPrefixLength("-"), 0);
  EXPECT_EQ(RepetitionPrefixLength("|"), 0);
}

// Helper for functions to evaluate the internal Expressions.
std::pair<std::optional<int64_t>, std::optional<int64_t>> Eval(
    RepetitionRange r) {
  return {
      r.min_length ? std::optional{r.min_length->Evaluate(EmptyLookup())}
                   : std::nullopt,
      r.max_length ? std::optional{r.max_length->Evaluate(EmptyLookup())}
                   : std::nullopt,
  };
}

TEST(ParseRepetitionTest, EmptyRepetitionShouldGiveLengthOne) {
  EXPECT_THAT(Eval(ParseRepetitionBody("")), FieldsAre(1, 1));
}

TEST(ParseRepetitionTest, SpecialCharactersShouldParseProperly) {
  EXPECT_THAT(Eval(ParseRepetitionBody("?")),
              FieldsAre(std::nullopt, Optional(1)));
  EXPECT_THAT(Eval(ParseRepetitionBody("+")),
              FieldsAre(Optional(1), std::nullopt));
  EXPECT_THAT(Eval(ParseRepetitionBody("*")),
              FieldsAre(std::nullopt, std::nullopt));
}

TEST(ParseRepetitionTest, SimpleCasesShouldWork) {
  EXPECT_THAT(Eval(ParseRepetitionBody("{3}")),
              FieldsAre(Optional(3), Optional(3)));
  EXPECT_THAT(Eval(ParseRepetitionBody("{3,14}")),
              FieldsAre(Optional(3), Optional(14)));
  EXPECT_THAT(Eval(ParseRepetitionBody("{3,}")),
              FieldsAre(Optional(3), std::nullopt));
  EXPECT_THAT(Eval(ParseRepetitionBody("{,3}")),
              FieldsAre(std::nullopt, Optional(3)));
  EXPECT_THAT(Eval(ParseRepetitionBody("{,}")),
              FieldsAre(std::nullopt, std::nullopt));
}

TEST(ParseRepetitionTest, MultipleCommasShouldFail) {
  EXPECT_THAT(
      [] { (void)ParseRepetitionBody("{3,4,5}"); },
      ThrowsMessage<std::invalid_argument>(HasSubstr("repetition block")));
}

TEST(ParseRepetitionTest, MissingBracesShouldFail) {
  EXPECT_THAT([] { (void)ParseRepetitionBody("{1,2"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("}")));
  EXPECT_THAT([] { (void)ParseRepetitionBody("1,2}"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("{")));
  EXPECT_THAT([] { (void)ParseRepetitionBody(",1}"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("{")));
  EXPECT_THAT([] { (void)ParseRepetitionBody("{1,"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("}")));
}

TEST(ParseRepetitionTest, NonIntegerShouldFail) {
  EXPECT_THAT(
      [] { (void)ParseRepetitionBody("{1.5,2}"); },
      ThrowsMessage<std::invalid_argument>(HasSubstr("repetition block")));
}

TEST(ParseRepetitionTest, HexShouldFail) {
  EXPECT_THAT(
      [] { (void)ParseRepetitionBody("{0x00,0x10}"); },
      ThrowsMessage<std::invalid_argument>(HasSubstr("repetition block")));
}

TEST(ParseRepetitionTest, SpecialCharacterAtStartShouldFail) {
  EXPECT_THAT(
      [] { (void)ParseRepetitionBody("}"); },
      ThrowsMessage<std::invalid_argument>(HasSubstr("repetition character")));
  EXPECT_THAT(
      [] { (void)ParseRepetitionBody("^"); },
      ThrowsMessage<std::invalid_argument>(HasSubstr("repetition character")));
  EXPECT_THAT(
      [] { (void)ParseRepetitionBody("("); },
      ThrowsMessage<std::invalid_argument>(HasSubstr("repetition character")));
  EXPECT_THAT(
      [] { (void)ParseRepetitionBody("|"); },
      ThrowsMessage<std::invalid_argument>(HasSubstr("repetition character")));
  EXPECT_THAT(
      [] { (void)ParseRepetitionBody("["); },
      ThrowsMessage<std::invalid_argument>(HasSubstr("repetition character")));
  EXPECT_THAT(
      [] { (void)ParseRepetitionBody(")"); },
      ThrowsMessage<std::invalid_argument>(HasSubstr("repetition character")));
  EXPECT_THAT(
      [] { (void)ParseRepetitionBody("]"); },
      ThrowsMessage<std::invalid_argument>(HasSubstr("repetition character")));
  EXPECT_THAT(
      [] { (void)ParseRepetitionBody("-"); },
      ThrowsMessage<std::invalid_argument>(HasSubstr("repetition character")));
}

TEST(ParseRepetitionTest, OtherCharacterAtStartShouldFail) {
  EXPECT_THAT(
      [] { (void)ParseRepetitionBody("a"); },
      ThrowsMessage<std::invalid_argument>(HasSubstr("repetition character")));
  EXPECT_THAT(
      [] { (void)ParseRepetitionBody("abc"); },
      ThrowsMessage<std::invalid_argument>(HasSubstr("Expected { and }")));
}

TEST(ParseRepeatedCharSetTest, EmptyStringShouldFail) {
  EXPECT_THAT([] { (void)ParseRepeatedCharSetPrefix(""); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("mpty")));
}

TEST(ParseRepeatedCharSetTest, SpecialCharacterAtStartShouldFail) {
  EXPECT_THAT([] { (void)ParseRepeatedCharSetPrefix("{"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("{")));
  EXPECT_THAT([] { (void)ParseRepeatedCharSetPrefix("}"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("}")));
  EXPECT_THAT([] { (void)ParseRepeatedCharSetPrefix("^"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("^")));
  EXPECT_THAT([] { (void)ParseRepeatedCharSetPrefix("("); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("(")));
  EXPECT_THAT([] { (void)ParseRepeatedCharSetPrefix(")"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr(")")));
  EXPECT_THAT([] { (void)ParseRepeatedCharSetPrefix("|"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("|")));
  EXPECT_THAT([] { (void)ParseRepeatedCharSetPrefix("["); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("[")));
  EXPECT_THAT([] { (void)ParseRepeatedCharSetPrefix("]"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("]")));
  EXPECT_THAT([] { (void)ParseRepeatedCharSetPrefix("-"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("-")));
  EXPECT_THAT([] { (void)ParseRepeatedCharSetPrefix("?"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("?")));
  EXPECT_THAT([] { (void)ParseRepeatedCharSetPrefix("+"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("+")));
  EXPECT_THAT([] { (void)ParseRepeatedCharSetPrefix("*"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("*")));
}

TEST(ParseRepeatedCharSetTest, RepetitionPartFirstShouldFail) {
  EXPECT_THAT([] { (void)ParseRepeatedCharSetPrefix("{3,1}"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("{")));
  EXPECT_THAT([] { (void)ParseRepeatedCharSetPrefix("{,1}"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("{")));
  EXPECT_THAT([] { (void)ParseRepeatedCharSetPrefix("*"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("*")));
}

TEST(ParseRepeatedCharSetTest,
     FirstCharacterWithoutRepetitionShouldReturnSingleItem) {
  EXPECT_THAT(ParseRepeatedCharSetPrefix("a"),
              Field(&PatternNode::pattern, "a"));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("abc"),
              Field(&PatternNode::pattern, "a"));
}

TEST(ParseRepeatedCharSetTest,
     FirstCharacterWithRepetitionShouldReturnSingleItemWithRepetition) {
  EXPECT_THAT(ParseRepeatedCharSetPrefix("a*"),
              Field(&PatternNode::pattern, "a*"));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("a+bc"),
              Field(&PatternNode::pattern, "a+"));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("a{1,3}bc"),
              Field(&PatternNode::pattern, "a{1,3}"));
}

TEST(ParseRepeatedCharSetTest,
     FirstCharacterWithoutRepetitionShouldReturnJustCharSet) {
  EXPECT_THAT(ParseRepeatedCharSetPrefix("[a]"),
              Field(&PatternNode::pattern, "[a]"));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("[ab]c"),
              Field(&PatternNode::pattern, "[ab]"));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("[ab]c*"),
              Field(&PatternNode::pattern, "[ab]"));
}

TEST(ParseRepeatedCharSetTest,
     CharacterSetWithRepetitionShouldReturnWithRepetition) {
  EXPECT_THAT(ParseRepeatedCharSetPrefix("[a]*"),
              Field(&PatternNode::pattern, "[a]*"));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("[ab]?c"),
              Field(&PatternNode::pattern, "[ab]?"));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("[abc]{1,3}de"),
              Field(&PatternNode::pattern, "[abc]{1,3}"));
}

TEST(ParseRepeatedCharSetTest, CharacterSetParserShouldNotSetSubpattern) {
  EXPECT_THAT(ParseRepeatedCharSetPrefix("a"),
              Field(&PatternNode::subpatterns, IsEmpty()));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("[ab]"),
              Field(&PatternNode::subpatterns, IsEmpty()));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("a*"),
              Field(&PatternNode::subpatterns, IsEmpty()));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("[ab]?c"),
              Field(&PatternNode::subpatterns, IsEmpty()));
}

TEST(ParseRepeatedCharSetTest,
     CharacterSetShouldNotIncludeSquareBracketsByDefault) {
  PatternNode p = ParseRepeatedCharSetPrefix("[a]");
  // Somewhat testing an internal implementation detail since this may not be
  // the top node in the tree.
  EXPECT_THAT(p.repeated_character_set, Optional(AcceptsOnly("a")));
}

TEST(ParseScopePrefixTest, EmptyStringShouldFail) {
  EXPECT_THAT([] { (void)ParseScopePrefix(""); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("mpty")));
}

TEST(ParseScopePrefixTest, SingleCloseBracketShouldFail) {
  EXPECT_THAT([] { (void)ParseScopePrefix(")"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("mpty scope")));
}

TEST(ParseScopePrefixTest, SpecialCharacterAtStartShouldFail) {
  EXPECT_THAT([] { (void)ParseScopePrefix("{"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("{")));
  EXPECT_THAT([] { (void)ParseScopePrefix("}"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("}")));
  EXPECT_THAT([] { (void)ParseScopePrefix("^"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("^")));
  EXPECT_THAT([] { (void)ParseScopePrefix("|"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("|")));
  EXPECT_THAT([] { (void)ParseScopePrefix("["); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("[")));
  EXPECT_THAT([] { (void)ParseScopePrefix("]"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("]")));
  EXPECT_THAT([] { (void)ParseScopePrefix("-"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("-")));
  EXPECT_THAT([] { (void)ParseScopePrefix("?"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("?")));
  EXPECT_THAT([] { (void)ParseScopePrefix("+"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("+")));
  EXPECT_THAT([] { (void)ParseScopePrefix("*"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("*")));
}

TEST(ParseScopePrefixTest, UnmatchedOpenBraceShouldFail) {
  EXPECT_THAT([] { (void)ParseScopePrefix("("); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("mpty scope")));
  EXPECT_THAT([] { (void)ParseScopePrefix("(abc"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("')'")));
  EXPECT_THAT([] { (void)ParseScopePrefix("(a(bc)"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("')'")));
}

TEST(ParseScopePrefixTest, NestedScopesShouldBeOk) {
  EXPECT_THAT(ParseScopePrefix("(ab(cd))"),
              Field(&PatternNode::pattern, "(ab(cd))"));
  EXPECT_THAT(ParseScopePrefix("a(b(cd))"),
              Field(&PatternNode::pattern, "a(b(cd))"));
}

TEST(ParseScopePrefixTest, SingleCharacterShouldWork) {
  EXPECT_THAT(ParseScopePrefix("a"),
              SubpatternsAre(PatternNode::SubpatternType::kAllOf,
                             std::vector<std::string_view>({"a"})));
}

TEST(ParseScopePrefixTest, ParsingFlatScopeShouldGetPatternCorrect) {
  EXPECT_THAT(ParseScopePrefix("ab"), Field(&PatternNode::pattern, "ab"));

  EXPECT_THAT(ParseScopePrefix("a*b+"), Field(&PatternNode::pattern, "a*b+"));
}

TEST(ParseScopePrefixTest,
     ParsingFlatScopeWithCloseBracketShouldGetPatternCorrect) {
  EXPECT_THAT(ParseScopePrefix("ab)cd"), Field(&PatternNode::pattern, "ab"));

  EXPECT_THAT(ParseScopePrefix("a*b+)c?d*"),
              Field(&PatternNode::pattern, "a*b+"));
}

TEST(ParseScopePrefixTest, ParsingFlatScopeShouldGetSubpatternsCorrect) {
  EXPECT_THAT(ParseScopePrefix("ab"),
              SubpatternsAre(PatternNode::SubpatternType::kAllOf,
                             std::vector<std::string_view>({"a", "b"})));

  EXPECT_THAT(ParseScopePrefix("a*b+"),
              SubpatternsAre(PatternNode::SubpatternType::kAllOf,
                             std::vector<std::string_view>({"a*", "b+"})));
}

TEST(ParseScopePrefixTest,
     ParsingFlatScopeWithEndBraceShouldGetSubpatternsCorrect) {
  EXPECT_THAT(ParseScopePrefix("ab)cd"),
              SubpatternsAre(PatternNode::SubpatternType::kAllOf,
                             std::vector<std::string_view>({"a", "b"})));

  EXPECT_THAT(ParseScopePrefix("a*b+)c?d*"),
              SubpatternsAre(PatternNode::SubpatternType::kAllOf,
                             std::vector<std::string_view>({"a*", "b+"})));
}

TEST(ParseScopePrefixTest, ParsingScopeWithOrClauseShouldGetPatternCorrect) {
  EXPECT_THAT(ParseScopePrefix("ab|cd"),
              SubpatternsAre(PatternNode::SubpatternType::kAnyOf,
                             std::vector<std::string_view>({"ab", "cd"})));

  EXPECT_THAT(ParseScopePrefix("a*b+|c?d*"),
              SubpatternsAre(PatternNode::SubpatternType::kAnyOf,
                             std::vector<std::string_view>({"a*b+", "c?d*"})));
}

TEST(ParseScopePrefixTest, ParsingScopeWithEmptyOrClausesShouldFail) {
  EXPECT_THAT([] { (void)ParseScopePrefix("|"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("mpty or-block")));
  EXPECT_THAT([] { (void)ParseScopePrefix("||"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("mpty or-block")));
  EXPECT_THAT([] { (void)ParseScopePrefix("|a"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("mpty or-block")));
  EXPECT_THAT([] { (void)ParseScopePrefix("a|"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("mpty or-block")));
  EXPECT_THAT([] { (void)ParseScopePrefix("a||b"); },
              ThrowsMessage<std::invalid_argument>(HasSubstr("mpty or-block")));
}

TEST(ParseScopePrefixTest,
     ParsingScopeWithOrClauseAndEndBraceShouldGetSubpatternsCorrect) {
  EXPECT_THAT(ParseScopePrefix("ab|cd)ef"),
              SubpatternsAre(PatternNode::SubpatternType::kAnyOf,
                             std::vector<std::string_view>({"ab", "cd"})));

  EXPECT_THAT(ParseScopePrefix("a*b+|c?d*)ef"),
              SubpatternsAre(PatternNode::SubpatternType::kAnyOf,
                             std::vector<std::string_view>({"a*b+", "c?d*"})));
}

TEST(ParseScopePrefixTest, SpecialCharactersCanBeUsedInsideSquareBrackets) {
  EXPECT_THAT(
      ParseScopePrefix("[(][)][*][[][]][?][+]"),
      SubpatternsAre(PatternNode::SubpatternType::kAllOf,
                     std::vector<std::string_view>(
                         {"[(]", "[)]", "[*]", "[[]", "[]]", "[?]", "[+]"})));
}

TEST(ParseScopePrefixTest, NestingShouldGetSubpatternsCorrect) {
  EXPECT_THAT(
      ParseScopePrefix("a(bc(de|fg))"),
      SubpatternsAre(PatternNode::SubpatternType::kAllOf,
                     std::vector<std::string_view>({"a", "(bc(de|fg))"})));

  EXPECT_THAT(ParseScopePrefix("(abc(de|fg))"),
              SubpatternsAre(PatternNode::SubpatternType::kAllOf,
                             std::vector<std::string_view>({"(abc(de|fg))"})));
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
