// Copyright 2025 Darcy Best
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

#ifndef MORIARTY_LIBRARIAN_IO_CONFIG_H_
#define MORIARTY_LIBRARIAN_IO_CONFIG_H_

#include <istream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "src/librarian/policies.h"
#include "src/librarian/util/ref.h"

namespace moriarty {

// Various kinds of whitespace characters.
enum class Whitespace { kSpace, kTab, kNewline };

// Information about where the input cursor is in the input stream.
struct InputCursor {
  enum class ErrorSnippetCaretPosition {
    kAtCursor,
    kBeforePreviousRead,
  };

  // Creates an InputCursor with the strictest settings.
  static InputCursor CreatePreciseStrictness(Ref<std::istream> is);

  // Creates an InputCursor with the most flexible settings.
  static InputCursor CreateFlexibleStrictness(Ref<std::istream> is);

  InputCursor(Ref<std::istream> is,
              WhitespaceStrictness whitespace = WhitespaceStrictness::kPrecise,
              NumericStrictness numeric = NumericStrictness::kPrecise);

  [[nodiscard]] WhitespaceStrictness GetWhitespaceStrictness() const;
  [[nodiscard]] NumericStrictness GetNumericStrictness() const;

  [[nodiscard]] std::string ReadToken();
  [[nodiscard]] std::optional<std::string> PeekToken();
  void ReadEof();
  void ReadWhitespace(Whitespace whitespace);
  [[noreturn]] void ThrowIOError(std::string_view message,
                                 ErrorSnippetCaretPosition caret_position,
                                 std::string_view multiline_prefix = "");
  bool IsEOF();

 private:
  struct CursorMetadata {
    int line_num = 1;        // 1-based
    int col_num = 0;         // 0-based
    int token_num_file = 0;  // 0-based
    int token_num_line = 0;  // 0-based
    int read_start_col = -1;
    int read_length = 0;
  };

  struct ErrorMessageMetadata {
    int message_line_num = 1;
    int message_col_num = 1;
    int message_token_num_line = 1;
    int message_token_num_file = 1;
    int caret_col = 0;
    int highlight_length = 1;
  };

  [[nodiscard]] std::string FormatIOErrorMessage(
      std::string_view message, ErrorSnippetCaretPosition caret_position,
      std::string_view multiline_prefix) const;

  [[nodiscard]] std::string GetErrorContextSnippet(
      int chars_before, int chars_after,
      ErrorSnippetCaretPosition caret_position) const;

  static char WhitespaceAsChar(Whitespace whitespace);
  static std::string ReadableChar(char c);
  static char RenderChar(char c);
  static std::string RenderForSnippet(std::string_view raw);

  void RegisterReadChar(char c);
  ErrorMessageMetadata ResolveErrorMessageMetadata(
      ErrorSnippetCaretPosition caret_position) const;
  void RegisterReadSnapshot(int start_line_num, int start_col_num,
                            int token_num_line, int token_num_file,
                            int read_length);
  void StripLeadingWhitespace();
  std::pair<std::string, bool> ReadLookAhead(int max_chars) const;

  WhitespaceStrictness whitespace_strictness_ = WhitespaceStrictness::kPrecise;
  NumericStrictness numeric_strictness_ = NumericStrictness::kPrecise;
  CursorMetadata cursor_metadata_;
  CursorMetadata previous_read_metadata_;

  Ref<std::istream> is_;
  std::string current_line_read_;
};

}  // namespace moriarty

#endif  // MORIARTY_LIBRARIAN_IO_CONFIG_H_
