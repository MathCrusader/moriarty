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

#include <cctype>
#include <format>
#include <stdexcept>

#include "src/librarian/errors.h"

namespace moriarty {

InputCursor InputCursor::CreatePreciseStrictness(Ref<std::istream> is) {
  return InputCursor(is, WhitespaceStrictness::kPrecise,
                     NumericStrictness::kPrecise);
}

InputCursor InputCursor::CreateFlexibleStrictness(Ref<std::istream> is) {
  return InputCursor(is, WhitespaceStrictness::kFlexible,
                     NumericStrictness::kFlexible);
}

InputCursor::InputCursor(Ref<std::istream> is, WhitespaceStrictness whitespace,
                         NumericStrictness numeric)
    : whitespace_strictness_(whitespace),
      numeric_strictness_(numeric),
      is_(is) {}

WhitespaceStrictness InputCursor::GetWhitespaceStrictness() const {
  return whitespace_strictness_;
}

NumericStrictness InputCursor::GetNumericStrictness() const {
  return numeric_strictness_;
}

std::string InputCursor::ReadToken() {
  if (whitespace_strictness_ == WhitespaceStrictness::kFlexible)
    StripLeadingWhitespace();

  if (IsEOF())
    ThrowIOError("Expected a token, but got EOF.",
                 ErrorSnippetCaretPosition::kAtCursor);

  std::istream& is = is_.get();

  if (whitespace_strictness_ == WhitespaceStrictness::kPrecise) {
    int c = is.peek();
    if (c == EOF || !std::isprint(static_cast<unsigned char>(c)) ||
        std::isspace(static_cast<unsigned char>(c))) {
      ThrowIOError(std::format("Expected a token, but got '{}'.",
                               ReadableChar(static_cast<char>(c))),
                   ErrorSnippetCaretPosition::kAtCursor);
    }
  }

  if (!is) {
    ThrowIOError("Failed to read from the input stream.",
                 ErrorSnippetCaretPosition::kAtCursor);
  }

  int token_start_line = cursor_metadata_.line_num;
  int token_start_col = cursor_metadata_.col_num;
  int token_num_file_for_read = cursor_metadata_.token_num_file + 1;
  int token_num_line_for_read = cursor_metadata_.token_num_line + 1;
  cursor_metadata_.token_num_file++;
  cursor_metadata_.token_num_line++;

  std::string token;
  is >> token;

  if (!is) {
    ThrowIOError("Failed to read from the input stream.",
                 ErrorSnippetCaretPosition::kAtCursor);
  }

  RegisterReadSnapshot(token_start_line, token_start_col,
                       token_num_line_for_read, token_num_file_for_read,
                       static_cast<int>(token.length()));
  cursor_metadata_.col_num += token.length();
  current_line_read_ += token;

  return token;
}

std::optional<std::string> InputCursor::PeekToken() {
  std::istream& is = is_.get();
  std::streampos original_pos = is.tellg();
  if (original_pos == std::streampos(-1)) return std::nullopt;

  InputCursor original_cursor = *this;

  auto restore = [&]() {
    is.clear();
    is.seekg(original_pos);
    *this = original_cursor;
  };

  if (whitespace_strictness_ == WhitespaceStrictness::kFlexible)
    StripLeadingWhitespace();

  if (IsEOF()) {
    restore();
    return std::nullopt;
  }

  if (whitespace_strictness_ == WhitespaceStrictness::kPrecise) {
    int c = is.peek();
    if (c == EOF || !std::isprint(static_cast<unsigned char>(c)) ||
        std::isspace(static_cast<unsigned char>(c))) {
      restore();
      return std::nullopt;
    }
  }

  if (!is) {
    restore();
    return std::nullopt;
  }

  std::string token;
  is >> token;
  restore();
  return token;
}

void InputCursor::ReadEof() {
  if (whitespace_strictness_ == WhitespaceStrictness::kFlexible)
    StripLeadingWhitespace();
  if (!IsEOF())
    ThrowIOError("Expected EOF, but got more input.",
                 ErrorSnippetCaretPosition::kAtCursor);
}

void InputCursor::ReadWhitespace(Whitespace whitespace) {
  if (whitespace_strictness_ == WhitespaceStrictness::kFlexible) return;

  std::istream& is = is_.get();

  if (IsEOF()) {
    ThrowIOError(std::format("Expected '{}', but got EOF.",
                             ReadableChar(WhitespaceAsChar(whitespace))),
                 ErrorSnippetCaretPosition::kAtCursor);
  }

  char c = static_cast<char>(is.peek());

  if (!std::isspace(static_cast<unsigned char>(c))) {
    ThrowIOError(
        std::format("Expected whitespace, but got '{}'.", ReadableChar(c)),
        ErrorSnippetCaretPosition::kAtCursor);
  }

  if (c != WhitespaceAsChar(whitespace)) {
    ThrowIOError(std::format("Expected '{}', but got '{}'.",
                             ReadableChar(WhitespaceAsChar(whitespace)),
                             ReadableChar(c)),
                 ErrorSnippetCaretPosition::kAtCursor);
  }

  RegisterReadSnapshot(cursor_metadata_.line_num, cursor_metadata_.col_num,
                       cursor_metadata_.token_num_line + 1,
                       cursor_metadata_.token_num_file + 1, 1);
  char read = static_cast<char>(is.get());
  RegisterReadChar(read);
}

std::string InputCursor::FormatIOErrorMessage(
    std::string_view message, ErrorSnippetCaretPosition caret_position,
    std::string_view multiline_prefix) const {
  ErrorMessageMetadata metadata = ResolveErrorMessageMetadata(caret_position);
  std::string header = std::format(
      "Line {}, token {} (col {}): {}", metadata.message_line_num,
      metadata.message_token_num_line, metadata.message_col_num, message);
  std::string snippet = GetErrorContextSnippet(30, 30, caret_position);
  if (multiline_prefix.empty()) return std::format("{}\n{}", header, snippet);

  while (multiline_prefix.size() > 1 && multiline_prefix.back() == '\n')
    multiline_prefix.remove_suffix(1);
  return std::format("{}\n\n{}\n{}", multiline_prefix, header, snippet);
}

std::string InputCursor::GetErrorContextSnippet(
    int chars_before, int chars_after,
    ErrorSnippetCaretPosition caret_position) const {
  ErrorMessageMetadata metadata = ResolveErrorMessageMetadata(caret_position);
  int caret_col = metadata.caret_col;
  int highlight_length = metadata.highlight_length;
  if (highlight_length < 1) highlight_length = 1;
  if (caret_col < 0) caret_col = 0;
  if (caret_col > static_cast<int>(current_line_read_.size())) {
    caret_col = static_cast<int>(current_line_read_.size());
  }

  int snippet_start_col = caret_col - chars_before;
  if (snippet_start_col < 0) snippet_start_col = 0;

  int desired_end_col = caret_col + chars_after;
  int highlight_end_col = caret_col + highlight_length;
  if (desired_end_col < highlight_end_col) desired_end_col = highlight_end_col;
  int required_lookahead =
      desired_end_col - static_cast<int>(current_line_read_.size());
  if (required_lookahead < 0) required_lookahead = 0;

  auto [lookahead, lookahead_trimmed] = ReadLookAhead(required_lookahead);

  std::string full_line = current_line_read_;
  full_line += lookahead;

  int snippet_end_col = desired_end_col;
  if (snippet_end_col > static_cast<int>(full_line.size())) {
    snippet_end_col = static_cast<int>(full_line.size());
  }
  if (snippet_end_col < snippet_start_col) snippet_end_col = snippet_start_col;

  bool left_trimmed = snippet_start_col > 0;
  bool right_trimmed = false;
  if (snippet_end_col < static_cast<int>(full_line.size()))
    right_trimmed = true;
  if (snippet_end_col == static_cast<int>(full_line.size()) &&
      lookahead_trimmed)
    right_trimmed = true;

  std::string rendered = RenderForSnippet(std::string_view(full_line).substr(
      snippet_start_col, snippet_end_col - snippet_start_col));

  std::string line;
  if (left_trimmed) line += "...";
  line += rendered;
  if (right_trimmed) line += "...";

  int rendered_caret_col =
      (caret_col - snippet_start_col) + (left_trimmed ? 3 : 0);
  if (rendered_caret_col < 0) rendered_caret_col = 0;
  int visible_highlight_start = caret_col;
  if (visible_highlight_start < snippet_start_col)
    visible_highlight_start = snippet_start_col;
  int visible_highlight_end = highlight_end_col;
  if (visible_highlight_end > snippet_end_col)
    visible_highlight_end = snippet_end_col;
  int visible_highlight_len = visible_highlight_end - visible_highlight_start;
  if (visible_highlight_len < 1) visible_highlight_len = 1;

  std::string marker = std::string(rendered_caret_col, ' ') + "^";
  if (visible_highlight_len > 1) {
    marker += std::string(visible_highlight_len - 1, '~');
  }

  return std::format("{}\n{}", line, marker);
}

char InputCursor::WhitespaceAsChar(Whitespace whitespace) {
  switch (whitespace) {
    case Whitespace::kNewline:
      return '\n';
    case Whitespace::kTab:
      return '\t';
    case Whitespace::kSpace:
      return ' ';
  }
  throw std::logic_error("Invalid whitespace enum value");
}

std::string InputCursor::ReadableChar(char c) {
  switch (c) {
    case '\n':
      return "\\n";
    case '\t':
      return "\\t";
    case '\r':
      return "\\r";
  }
  if (std::isprint(static_cast<unsigned char>(c))) return std::string(1, c);
  return "ASCII=" + std::to_string(static_cast<unsigned char>(c));
}

char InputCursor::RenderChar(char c) {
  if (c == '\t') return ' ';
  if (std::isprint(static_cast<unsigned char>(c))) return c;
  return '?';
}

std::string InputCursor::RenderForSnippet(std::string_view raw) {
  std::string rendered;
  rendered.reserve(raw.size());
  for (char c : raw) rendered.push_back(RenderChar(c));
  return rendered;
}

[[noreturn]] void InputCursor::ThrowIOError(
    std::string_view message, ErrorSnippetCaretPosition caret_position,
    std::string_view multiline_prefix) {
  throw IOError(
      FormatIOErrorMessage(message, caret_position, multiline_prefix));
}

void InputCursor::RegisterReadChar(char c) {
  if (c == '\n') {
    cursor_metadata_.line_num++;
    cursor_metadata_.col_num = 0;
    cursor_metadata_.token_num_line = 0;
    current_line_read_.clear();
    return;
  }
  cursor_metadata_.col_num++;
  current_line_read_.push_back(c);
}

void InputCursor::RegisterReadSnapshot(int start_line_num, int start_col_num,
                                       int token_num_line, int token_num_file,
                                       int read_length) {
  previous_read_metadata_.line_num = start_line_num;
  previous_read_metadata_.col_num = start_col_num;
  previous_read_metadata_.token_num_line = token_num_line;
  previous_read_metadata_.token_num_file = token_num_file;
  previous_read_metadata_.read_start_col = start_col_num;
  previous_read_metadata_.read_length = read_length;
}

InputCursor::ErrorMessageMetadata InputCursor::ResolveErrorMessageMetadata(
    ErrorSnippetCaretPosition caret_position) const {
  ErrorMessageMetadata metadata;
  if (caret_position == ErrorSnippetCaretPosition::kBeforePreviousRead &&
      previous_read_metadata_.read_start_col >= 0) {
    metadata.message_line_num = previous_read_metadata_.line_num;
    metadata.message_col_num = previous_read_metadata_.col_num + 1;
    metadata.message_token_num_line = previous_read_metadata_.token_num_line;
    metadata.message_token_num_file = previous_read_metadata_.token_num_file;
    if (previous_read_metadata_.line_num == cursor_metadata_.line_num) {
      metadata.caret_col = previous_read_metadata_.read_start_col;
      metadata.highlight_length = previous_read_metadata_.read_length;
    } else {
      metadata.caret_col = cursor_metadata_.col_num;
      metadata.highlight_length = 1;
    }
    return metadata;
  }

  metadata.message_line_num = cursor_metadata_.line_num;
  metadata.message_col_num = cursor_metadata_.col_num + 1;
  metadata.message_token_num_line = cursor_metadata_.token_num_line + 1;
  metadata.message_token_num_file = cursor_metadata_.token_num_file + 1;
  metadata.caret_col = cursor_metadata_.col_num;
  metadata.highlight_length = 1;
  return metadata;
}

bool InputCursor::IsEOF() {
  std::istream& is = is_.get();
  if (is.eof()) return true;
  return is.peek() == EOF;
}

void InputCursor::StripLeadingWhitespace() {
  std::istream& is = is_.get();
  while (is) {
    int c = is.peek();
    if (c == EOF) break;
    if (!std::isspace(static_cast<unsigned char>(c))) break;
    char read = static_cast<char>(is.get());
    RegisterReadChar(read);
  }
}

std::pair<std::string, bool> InputCursor::ReadLookAhead(int max_chars) const {
  if (max_chars <= 0) return {"", false};

  std::istream& is = is_.get();
  std::streampos original_pos = is.tellg();
  if (original_pos == std::streampos(-1)) return {"", false};

  std::ios::iostate original_state = is.rdstate();
  is.clear();

  std::string right;
  right.reserve(max_chars);
  while (static_cast<int>(right.size()) < max_chars) {
    int next = is.get();
    if (next == EOF) break;
    char c = static_cast<char>(next);
    if (c == '\n' || c == '\r') break;
    right.push_back(RenderChar(c));
  }

  bool right_trimmed = false;
  if (static_cast<int>(right.size()) == max_chars) {
    int next = is.peek();
    right_trimmed = (next != EOF && next != '\n' && next != '\r');
  }

  is.clear();
  is.seekg(original_pos);
  is.clear(original_state);
  return {right, right_trimmed};
}

}  // namespace moriarty
