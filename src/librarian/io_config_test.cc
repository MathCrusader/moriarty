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

#include "src/librarian/io_config.h"

#include <sstream>
#include <string>

#include "gtest/gtest.h"
#include "src/librarian/errors.h"
#include "src/librarian/testing/gtest_helpers.h"

using ::moriarty_testing::SingleCall;
using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::ThrowsMessage;

namespace moriarty {
namespace {

template <typename Action>
std::string CaptureIOErrorMessage(Action&& action) {
  try {
    action();
  } catch (const IOError& e) {
    return e.Message();
  }
  ADD_FAILURE() << "Expected IOError to be thrown.";
  return "";
}

void ExpectMaxMessageLineWidth(const std::string& message, int max_width) {
  std::size_t start = 0;
  while (start <= message.size()) {
    std::size_t end = message.find('\n', start);
    if (end == std::string::npos) end = message.size();
    int line_width = static_cast<int>(end - start);
    EXPECT_LE(line_width, max_width)
        << "Offending line: " << message.substr(start, end - start)
        << "\nFull message:\n"
        << message;
    if (end == message.size()) break;
    start = end + 1;
  }
}

TEST(InputCursorProcessingTest, StrictnessFactoriesShouldSetExpectedModes) {
  std::istringstream strict_stream("");
  InputCursor strict = InputCursor::CreatePreciseStrictness(strict_stream);
  EXPECT_EQ(strict.GetWhitespaceStrictness(), WhitespaceStrictness::kPrecise);
  EXPECT_EQ(strict.GetNumericStrictness(), NumericStrictness::kPrecise);

  std::istringstream flexible_stream("");
  InputCursor flexible = InputCursor::CreateFlexibleStrictness(flexible_stream);
  EXPECT_EQ(flexible.GetWhitespaceStrictness(),
            WhitespaceStrictness::kFlexible);
  EXPECT_EQ(flexible.GetNumericStrictness(), NumericStrictness::kFlexible);
}

TEST(InputCursorProcessingTest, ReadTokenShouldRespectWhitespaceStrictness) {
  {
    std::istringstream strict_stream(" hello");
    InputCursor strict(strict_stream, WhitespaceStrictness::kPrecise);
    EXPECT_THROW((void)strict.ReadToken(), IOError);
  }

  {
    std::istringstream flexible_stream(" \n\tHello");
    InputCursor flexible =
        InputCursor::CreateFlexibleStrictness(flexible_stream);
    EXPECT_EQ(flexible.ReadToken(), "Hello");
    EXPECT_TRUE(flexible.IsEOF());
  }
}

TEST(InputCursorProcessingTest, PeekTokenShouldNotConsumeStream) {
  std::istringstream ss("A B");
  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

  EXPECT_EQ(cursor.PeekToken(), "A");
  EXPECT_EQ(cursor.ReadToken(), "A");
  cursor.ReadWhitespace(Whitespace::kSpace);
  EXPECT_EQ(cursor.PeekToken(), "B");
  EXPECT_EQ(cursor.ReadToken(), "B");
  EXPECT_EQ(cursor.PeekToken(), std::nullopt);
}

TEST(InputCursorProcessingTest, PeekTokenShouldRespectWhitespaceStrictness) {
  {
    std::istringstream ss(" A");
    InputCursor cursor(ss, WhitespaceStrictness::kPrecise);
    EXPECT_EQ(cursor.PeekToken(), std::nullopt);
    cursor.ReadWhitespace(Whitespace::kSpace);
    EXPECT_EQ(cursor.PeekToken(), "A");
    EXPECT_EQ(cursor.ReadToken(), "A");
  }

  {
    std::istringstream ss(" A");
    InputCursor cursor(ss, WhitespaceStrictness::kFlexible);
    EXPECT_EQ(cursor.PeekToken(), "A");
    EXPECT_EQ(cursor.ReadToken(), "A");
    EXPECT_TRUE(cursor.IsEOF());
  }
}

TEST(InputCursorProcessingTest, ReadWhitespaceShouldValidateInStrictMode) {
  std::istringstream ss(" \t\n");
  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

  EXPECT_NO_THROW(cursor.ReadWhitespace(Whitespace::kSpace));
  EXPECT_NO_THROW(cursor.ReadWhitespace(Whitespace::kTab));
  EXPECT_NO_THROW(cursor.ReadWhitespace(Whitespace::kNewline));
  EXPECT_TRUE(cursor.IsEOF());
}

TEST(InputCursorProcessingTest, ReadWhitespaceShouldBeNoopInFlexibleMode) {
  std::istringstream ss("X");
  InputCursor cursor(ss, WhitespaceStrictness::kFlexible);

  EXPECT_NO_THROW(cursor.ReadWhitespace(Whitespace::kSpace));
  EXPECT_NO_THROW(cursor.ReadWhitespace(Whitespace::kTab));
  EXPECT_NO_THROW(cursor.ReadWhitespace(Whitespace::kNewline));
  EXPECT_FALSE(cursor.IsEOF());
  EXPECT_EQ(cursor.ReadToken(), "X");
  EXPECT_TRUE(cursor.IsEOF());
}

TEST(InputCursorProcessingTest, ReadWhitespaceShouldHandleEofByStrictness) {
  {
    std::istringstream ss("");
    InputCursor cursor(ss, WhitespaceStrictness::kPrecise);
    EXPECT_THROW(cursor.ReadWhitespace(Whitespace::kSpace), IOError);
  }

  {
    std::istringstream ss("");
    InputCursor cursor(ss, WhitespaceStrictness::kFlexible);
    EXPECT_NO_THROW(cursor.ReadWhitespace(Whitespace::kSpace));
    EXPECT_TRUE(cursor.IsEOF());
  }
}

TEST(InputCursorProcessingTest, ReadEofShouldRespectWhitespaceStrictness) {
  {
    std::istringstream ss("abc \n\t");
    InputCursor cursor(ss, WhitespaceStrictness::kFlexible);

    EXPECT_EQ(cursor.ReadToken(), "abc");
    EXPECT_NO_THROW(cursor.ReadEof());
  }

  {
    std::istringstream ss("abc \n\t");
    InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

    EXPECT_EQ(cursor.ReadToken(), "abc");
    EXPECT_THROW(cursor.ReadEof(), IOError);
  }
}

TEST(InputCursorProcessingTest, IsEOFShouldReflectStreamState) {
  std::istringstream ss("x");
  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

  EXPECT_FALSE(cursor.IsEOF());
  EXPECT_EQ(cursor.ReadToken(), "x");
  EXPECT_TRUE(cursor.IsEOF());
}

TEST(InputCursorErrorMessageTest, ReadTokenShouldReportEOFClearly) {
  std::istringstream ss("");
  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

  EXPECT_THAT(
      SingleCall([&] { (void)cursor.ReadToken(); }),
      ThrowsMessage<IOError>(HasSubstr("Expected a token, but got EOF.")));
}

TEST(InputCursorErrorMessageTest,
     ReadTokenShouldReportUnexpectedWhitespaceClearly) {
  std::istringstream ss("\tX");
  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

  EXPECT_THAT(
      SingleCall([&] { (void)cursor.ReadToken(); }),
      ThrowsMessage<IOError>(HasSubstr("Expected a token, but got '\\t'.")));
}

TEST(InputCursorErrorMessageTest,
     ReadWhitespaceShouldReportExpectedWhitespaceInStrictMode) {
  std::istringstream ss("\n");
  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

  EXPECT_THAT(SingleCall([&] { cursor.ReadWhitespace(Whitespace::kSpace); }),
              ThrowsMessage<IOError>(Eq(
                  R"(Line 1, token 1 (col 1): Expected ' ', but got '\n'.

^)")));
}

TEST(InputCursorErrorMessageTest,
     ReadWhitespaceShouldReportNonWhitespaceCharacters) {
  std::istringstream ss("A");
  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

  EXPECT_THAT(
      SingleCall([&] { cursor.ReadWhitespace(Whitespace::kSpace); }),
      ThrowsMessage<IOError>(HasSubstr("Expected whitespace, but got 'A'.")));
}

TEST(InputCursorErrorMessageTest,
     ReadWhitespaceShouldReportRequestedCharacterAtEOF) {
  std::istringstream ss("");
  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

  EXPECT_THAT(
      SingleCall([&] { cursor.ReadWhitespace(Whitespace::kTab); }),
      ThrowsMessage<IOError>(HasSubstr("Expected '\\t', but got EOF.")));
}

TEST(InputCursorErrorMessageTest, ReadEofShouldReportMessageInStrictMode) {
  std::istringstream ss("x");
  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

  EXPECT_THAT(SingleCall([&] { cursor.ReadEof(); }),
              ThrowsMessage<IOError>(Eq(
                  R"(Line 1, token 1 (col 1): Expected EOF, but got more input.
x
^)")));
}

TEST(InputCursorErrorMessageTest,
     ReadEofInPreciseModeShouldPointAtNextWhitespace) {
  std::istringstream ss("A B");
  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

  EXPECT_EQ(cursor.ReadToken(), "A");
  EXPECT_THAT(SingleCall([&] { cursor.ReadEof(); }),
              ThrowsMessage<IOError>(Eq(
                  R"(Line 1, token 2 (col 2): Expected EOF, but got more input.
A B
 ^)")));
}

TEST(InputCursorErrorMessageTest, ReadEofInPreciseModeShouldPointAtNextToken) {
  std::istringstream ss("A B");
  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

  EXPECT_EQ(cursor.ReadToken(), "A");
  cursor.ReadWhitespace(Whitespace::kSpace);
  EXPECT_THAT(SingleCall([&] { cursor.ReadEof(); }),
              ThrowsMessage<IOError>(Eq(
                  R"(Line 1, token 2 (col 3): Expected EOF, but got more input.
A B
  ^)")));
}

TEST(InputCursorErrorMessageTest,
     ReadEofInFlexibleModeShouldPointAtNextTokenAfterWhitespace) {
  std::istringstream ss("A \n\tB");
  InputCursor cursor(ss, WhitespaceStrictness::kFlexible);

  EXPECT_EQ(cursor.ReadToken(), "A");
  EXPECT_THAT(SingleCall([&] { cursor.ReadEof(); }),
              ThrowsMessage<IOError>(Eq(
                  R"(Line 2, token 1 (col 2): Expected EOF, but got more input.
 B
 ^)")));
}

TEST(InputCursorErrorMessageTest, ThrowIOErrorShouldUseAtCursorCaretPosition) {
  std::istringstream ss("Hello world");
  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

  EXPECT_EQ(cursor.ReadToken(), "Hello");
  EXPECT_THAT(SingleCall([&] {
                cursor.ThrowIOError(
                    "boom", InputCursor::ErrorSnippetCaretPosition::kAtCursor);
              }),
              ThrowsMessage<IOError>(Eq(
                  R"(Line 1, token 2 (col 6): boom
Hello world
     ^)")));
}

TEST(InputCursorErrorMessageTest, ThrowIOErrorShouldSupportMultilinePrefix) {
  std::istringstream ss("Hello world");
  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

  EXPECT_EQ(cursor.ReadToken(), "Hello");
  EXPECT_THAT(SingleCall([&] {
                cursor.ThrowIOError(
                    "boom", InputCursor::ErrorSnippetCaretPosition::kAtCursor,
                    "Extra context:\n- detail one\n- detail two");
              }),
              ThrowsMessage<IOError>(Eq(
                  R"(Extra context:
- detail one
- detail two

Line 1, token 2 (col 6): boom
Hello world
     ^)")));
}

TEST(InputCursorErrorMessageTest,
     ThrowIOErrorShouldUseBeforePreviousReadCaretPosition) {
  std::istringstream ss("Hello world");
  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

  EXPECT_EQ(cursor.ReadToken(), "Hello");
  EXPECT_THAT(
      SingleCall([&] {
        cursor.ThrowIOError(
            "boom",
            InputCursor::ErrorSnippetCaretPosition::kBeforePreviousRead);
      }),
      ThrowsMessage<IOError>(Eq(
          R"(Line 1, token 1 (col 1): boom
Hello world
^~~~~)")));
}

TEST(InputCursorErrorMessageTest,
     ThrowIOErrorShouldTreatWhitespaceAsPreviousRead) {
  std::istringstream ss("Hello world");
  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

  EXPECT_EQ(cursor.ReadToken(), "Hello");
  cursor.ReadWhitespace(Whitespace::kSpace);
  EXPECT_THAT(
      SingleCall([&] {
        cursor.ThrowIOError(
            "boom",
            InputCursor::ErrorSnippetCaretPosition::kBeforePreviousRead);
      }),
      ThrowsMessage<IOError>(Eq(
          R"(Line 1, token 2 (col 6): boom
Hello world
     ^)")));
}

TEST(InputCursorErrorMessageTest,
     ThrowIOErrorShouldHighlightMultilineTokensAtA_D_E_H) {
  std::string multiline = "A BB CCC\nDDDD EEEEE\nFFFFFF GGGGGGG HHHHHHHH";

  {
    std::istringstream ss(multiline);
    InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

    EXPECT_EQ(cursor.ReadToken(), "A");
    EXPECT_THAT(
        SingleCall([&] {
          cursor.ThrowIOError(
              "boom",
              InputCursor::ErrorSnippetCaretPosition::kBeforePreviousRead);
        }),
        ThrowsMessage<IOError>(Eq(
            R"(Line 1, token 1 (col 1): boom
A BB CCC
^)")));
  }

  {
    std::istringstream ss(multiline);
    InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

    EXPECT_EQ(cursor.ReadToken(), "A");
    cursor.ReadWhitespace(Whitespace::kSpace);
    EXPECT_EQ(cursor.ReadToken(), "BB");
    cursor.ReadWhitespace(Whitespace::kSpace);
    EXPECT_EQ(cursor.ReadToken(), "CCC");
    cursor.ReadWhitespace(Whitespace::kNewline);
    EXPECT_EQ(cursor.ReadToken(), "DDDD");
    EXPECT_THAT(
        SingleCall([&] {
          cursor.ThrowIOError(
              "boom",
              InputCursor::ErrorSnippetCaretPosition::kBeforePreviousRead);
        }),
        ThrowsMessage<IOError>(Eq(
            R"(Line 2, token 1 (col 1): boom
DDDD EEEEE
^~~~)")));
  }

  {
    std::istringstream ss(multiline);
    InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

    EXPECT_EQ(cursor.ReadToken(), "A");
    cursor.ReadWhitespace(Whitespace::kSpace);
    EXPECT_EQ(cursor.ReadToken(), "BB");
    cursor.ReadWhitespace(Whitespace::kSpace);
    EXPECT_EQ(cursor.ReadToken(), "CCC");
    cursor.ReadWhitespace(Whitespace::kNewline);
    EXPECT_EQ(cursor.ReadToken(), "DDDD");
    cursor.ReadWhitespace(Whitespace::kSpace);
    EXPECT_EQ(cursor.ReadToken(), "EEEEE");
    EXPECT_THAT(
        SingleCall([&] {
          cursor.ThrowIOError(
              "boom",
              InputCursor::ErrorSnippetCaretPosition::kBeforePreviousRead);
        }),
        ThrowsMessage<IOError>(Eq(
            R"(Line 2, token 2 (col 6): boom
DDDD EEEEE
     ^~~~~)")));
  }

  {
    std::istringstream ss(multiline);
    InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

    EXPECT_EQ(cursor.ReadToken(), "A");
    cursor.ReadWhitespace(Whitespace::kSpace);
    EXPECT_EQ(cursor.ReadToken(), "BB");
    cursor.ReadWhitespace(Whitespace::kSpace);
    EXPECT_EQ(cursor.ReadToken(), "CCC");
    cursor.ReadWhitespace(Whitespace::kNewline);
    EXPECT_EQ(cursor.ReadToken(), "DDDD");
    cursor.ReadWhitespace(Whitespace::kSpace);
    EXPECT_EQ(cursor.ReadToken(), "EEEEE");
    cursor.ReadWhitespace(Whitespace::kNewline);
    EXPECT_EQ(cursor.ReadToken(), "FFFFFF");
    cursor.ReadWhitespace(Whitespace::kSpace);
    EXPECT_EQ(cursor.ReadToken(), "GGGGGGG");
    cursor.ReadWhitespace(Whitespace::kSpace);
    EXPECT_EQ(cursor.ReadToken(), "HHHHHHHH");
    EXPECT_THAT(
        SingleCall([&] {
          cursor.ThrowIOError(
              "boom",
              InputCursor::ErrorSnippetCaretPosition::kBeforePreviousRead);
        }),
        ThrowsMessage<IOError>(Eq(
            R"(Line 3, token 3 (col 16): boom
FFFFFF GGGGGGG HHHHHHHH
               ^~~~~~~~)")));
  }
}

TEST(InputCursorErrorMessageTest,
     MultilineReadWhitespaceShouldReportIncorrectNewlineExpectation) {
  std::istringstream ss("A BB CCC\nDDDD EEEEE\nFFFFFF GGGGGGG HHHHHHHH");
  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

  EXPECT_EQ(cursor.ReadToken(), "A");
  EXPECT_THAT(SingleCall([&] { cursor.ReadWhitespace(Whitespace::kNewline); }),
              ThrowsMessage<IOError>(Eq(
                  R"(Line 1, token 2 (col 2): Expected '\n', but got ' '.
A BB CCC
 ^)")));
}

TEST(InputCursorErrorMessageTest,
     MultilineReadWhitespaceShouldReportIncorrectSpaceExpectation) {
  std::istringstream ss("A BB CCC\nDDDD EEEEE\nFFFFFF GGGGGGG HHHHHHHH");
  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

  EXPECT_EQ(cursor.ReadToken(), "A");
  cursor.ReadWhitespace(Whitespace::kSpace);
  EXPECT_EQ(cursor.ReadToken(), "BB");
  cursor.ReadWhitespace(Whitespace::kSpace);
  EXPECT_EQ(cursor.ReadToken(), "CCC");
  EXPECT_THAT(SingleCall([&] { cursor.ReadWhitespace(Whitespace::kSpace); }),
              ThrowsMessage<IOError>(Eq(
                  R"(Line 1, token 4 (col 9): Expected ' ', but got '\n'.
A BB CCC
        ^)")));
}

TEST(InputCursorErrorMessageTest,
     MultilineReadEofShouldReportIncorrectEOFExpectation) {
  {  // Precise
    std::istringstream ss("A BB CCC\nDDDD EEEEE\nFFFFFF GGGGGGG HHHHHHHH");
    InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

    EXPECT_EQ(cursor.ReadToken(), "A");
    cursor.ReadWhitespace(Whitespace::kSpace);
    EXPECT_EQ(cursor.ReadToken(), "BB");
    cursor.ReadWhitespace(Whitespace::kSpace);
    EXPECT_EQ(cursor.ReadToken(), "CCC");
    cursor.ReadWhitespace(Whitespace::kNewline);
    EXPECT_EQ(cursor.ReadToken(), "DDDD");
    cursor.ReadWhitespace(Whitespace::kSpace);
    EXPECT_EQ(cursor.ReadToken(), "EEEEE");
    EXPECT_THAT(
        SingleCall([&] { cursor.ReadEof(); }),
        ThrowsMessage<IOError>(Eq(
            R"(Line 2, token 3 (col 11): Expected EOF, but got more input.
DDDD EEEEE
          ^)")));
  }
  {  // Flexible
    std::istringstream ss("A BB CCC\nDDDD EEEEE\nFFFFFF GGGGGGG HHHHHHHH");
    InputCursor cursor(ss, WhitespaceStrictness::kFlexible);

    EXPECT_EQ(cursor.ReadToken(), "A");
    EXPECT_EQ(cursor.ReadToken(), "BB");
    EXPECT_EQ(cursor.ReadToken(), "CCC");
    EXPECT_EQ(cursor.ReadToken(), "DDDD");
    EXPECT_EQ(cursor.ReadToken(), "EEEEE");
    EXPECT_THAT(
        SingleCall([&] { cursor.ReadEof(); }),
        ThrowsMessage<IOError>(Eq(
            R"(Line 3, token 1 (col 1): Expected EOF, but got more input.
FFFFFF GGGGGGG HHHHHHHH
^)")));
  }
}

TEST(InputCursorErrorMessageTest,
     ErrorMessageWidthShouldStayBoundedForSingleMassiveToken) {
  {
    std::string massive_token(1200, 'A');
    std::istringstream ss(massive_token + " B");
    InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

    EXPECT_EQ(cursor.ReadToken(), massive_token);
    std::string message = CaptureIOErrorMessage([&] { cursor.ReadEof(); });
    ExpectMaxMessageLineWidth(message, 100);
  }
  {
    std::string massive_token(1200, 'A');
    std::istringstream ss(massive_token + " B");
    InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

    std::string message = CaptureIOErrorMessage(
        [&] { cursor.ReadWhitespace(Whitespace::kSpace); });
    ExpectMaxMessageLineWidth(message, 100);
  }
}

TEST(InputCursorErrorMessageTest,
     ErrorMessageWidthShouldStayBoundedForManySmallTokens) {
  std::string input;
  constexpr int kTokenCount = 250;
  for (int i = 0; i < kTokenCount; i++) {
    if (i) input.push_back(' ');
    input.push_back('x');
  }

  std::istringstream ss(input);
  InputCursor cursor(ss, WhitespaceStrictness::kPrecise);

  for (int i = 0; i < kTokenCount - 1; i++) {
    EXPECT_EQ(cursor.ReadToken(), "x");
    cursor.ReadWhitespace(Whitespace::kSpace);
  }

  std::string message = CaptureIOErrorMessage([&] { cursor.ReadEof(); });
  ExpectMaxMessageLineWidth(message, 100);
}

}  // namespace
}  // namespace moriarty
