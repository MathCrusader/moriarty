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

#include "src/contexts/internal/basic_istream_context.h"

#include <cassert>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <format>
#include <istream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include "src/librarian/errors.h"
#include "src/librarian/io_config.h"
#include "src/librarian/policies.h"
#include "src/librarian/util/debug_string.h"
#include "src/librarian/util/ref.h"

namespace moriarty {
namespace moriarty_internal {

BasicIStreamContext::BasicIStreamContext(Ref<InputCursor> input)
    : input_(input) {}

void BasicIStreamContext::ThrowIOError(std::string_view message) const {
  throw IOError(input_.get(), message);
}

namespace {

bool IsEOF(std::istream& is) {
  if (is.eof()) return true;
  return is.peek() == EOF;
}

std::string ReadableChar(char c) {
  switch (c) {
    case '\n':
      return "\\n";
    case '\t':
      return "\\t";
    case '\r':
      return "\\r";
  }
  if (std::isprint(c)) return std::string(1, c);
  return "ASCII=" + std::to_string(static_cast<int>(c));
}

char WhitespaceAsChar(Whitespace whitespace) {
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

void RegisterNewline(char c, InputCursor& cursor) {
  if (c != '\n') return;
  cursor.line_num++;
  cursor.col_num = 0;
  cursor.token_num_line = 0;
}

void StripLeadingWhitespace(std::istream& is, InputCursor& cursor) {
  while (is) {
    int c = is.peek();
    if (c == EOF) break;
    if (!std::isspace(c)) break;
    cursor.col_num++;
    is.get();
    RegisterNewline(c, cursor);
  }
}

}  // namespace

std::string BasicIStreamContext::ReadToken() {
  std::istream& is = GetIStream();

  if (GetWhitespaceStrictness() == WhitespaceStrictness::kFlexible)
    StripLeadingWhitespace(is, GetCursor());

  if (IsEOF(is)) ThrowIOError("Expected a token, but got EOF.");

  if (GetWhitespaceStrictness() == WhitespaceStrictness::kPrecise) {
    char c = is.peek();
    if (!std::isprint(c) || std::isspace(c)) {
      ThrowIOError(
          std::format("Expected a token, but got '{}'.", ReadableChar(c)));
    }
  }

  if (!is) ThrowIOError("Failed to read from the input stream.");

  GetCursor().token_num_file++;
  GetCursor().token_num_line++;
  // At this point, we are not at EOF and there is no leading whitespace.
  std::string token;
  is >> token;

  // I don't think this is necessary, but it is here for safety.
  if (!is) ThrowIOError("Failed to read from the input stream.");

  GetCursor().AddReadItem(token);
  GetCursor().col_num += token.length();

  return token;
}

std::optional<std::string> BasicIStreamContext::PeekToken() {
  std::istream& is = GetIStream();
  InputCursor& cursor = GetCursor();

  std::streampos original_pos = is.tellg();
  InputCursor original_cursor = cursor;

  auto restore = [&]() {
    is.clear();  // Clear EOF flag.
    is.seekg(original_pos);
    cursor = original_cursor;
  };

  if (GetWhitespaceStrictness() == WhitespaceStrictness::kFlexible)
    StripLeadingWhitespace(is, cursor);

  if (IsEOF(is)) {
    restore();
    return std::nullopt;
  }

  if (GetWhitespaceStrictness() == WhitespaceStrictness::kPrecise) {
    char c = is.peek();
    if (!std::isprint(c) || std::isspace(c)) {
      restore();
      return std::nullopt;
    }
  }

  if (!is) {
    restore();
    return std::nullopt;
  }

  // At this point, we are not at EOF and there is no leading whitespace.
  std::string token;
  is >> token;

  restore();
  return token;
}

void BasicIStreamContext::ReadEof() {
  std::istream& is = GetIStream();

  if (GetWhitespaceStrictness() == WhitespaceStrictness::kFlexible)
    StripLeadingWhitespace(is, GetCursor());

  if (!IsEOF(is)) ThrowIOError("Expected EOF, but got more input.");
}

void BasicIStreamContext::ReadWhitespace(Whitespace whitespace) {
  std::istream& is = GetIStream();

  if (IsEOF(is)) {
    ThrowIOError(std::format("Expected '{}', but got EOF.",
                             ReadableChar(WhitespaceAsChar(whitespace))));
  }
  GetCursor().col_num++;
  char c = is.get();
  if (!std::isspace(c)) {
    ThrowIOError(
        std::format("Expected whitespace, but got '{}'.", ReadableChar(c)));
  }
  RegisterNewline(c, GetCursor());

  GetCursor().AddReadItem(std::string(1, c));

  if (GetWhitespaceStrictness() == WhitespaceStrictness::kFlexible) return;

  if (c != WhitespaceAsChar(whitespace)) {
    ThrowIOError(std::format("Expected '{}', but got '{}'.",
                             ReadableChar(WhitespaceAsChar(whitespace)),
                             ReadableChar(c)));
  }
}

int64_t BasicIStreamContext::ReadInteger() {
  std::string read_token = ReadToken();

  std::string_view token = read_token;
  if (GetNumericStrictness() == NumericStrictness::kFlexible &&
      read_token.size() > 2 && read_token[0] == '+' &&
      std::isdigit(read_token[1])) {
    // std::from_chars() does not allow leading '+'.
    token.remove_prefix(1);
  }

  int64_t value = 0;
  std::from_chars_result result = std::from_chars(
      token.data(), token.data() + token.size(), value, /* base = */ 10);
  if (result.ec != std::errc{} || result.ptr != token.data() + token.size()) {
    ThrowIOError(std::format("Expected an integer, but got '{}'.",
                             librarian::DebugString(read_token)));
  }

  if (GetNumericStrictness() == NumericStrictness::kPrecise) {
    if (value == 0 && read_token.size() != 1) {
      ThrowIOError(std::format("Expected a (strict) integer, but got '{}'.",
                               librarian::DebugString(read_token)));
    }
    if (value != 0 && read_token[0] == '0') {
      ThrowIOError(std::format("Expected a (strict) integer, but got '{}'.",
                               librarian::DebugString(read_token)));
    }
    if (value != 0 && (read_token[0] == '-' && read_token[1] == '0')) {
      ThrowIOError(std::format("Expected a (strict) integer, but got '{}'.",
                               librarian::DebugString(read_token)));
    }
  }

  return value;
}

namespace {

std::optional<double> ParseStrictReal(std::string_view token,
                                      int required_digits) {
  // Check for valid characters.
  int digit_idx = -1;
  int non_zero_count = 0;
  for (int i = token[0] == '-' ? 1 : 0; i < token.size(); i++) {
    char c = token[i];
    if (c == '.') {
      if (digit_idx != -1) return std::nullopt;  // Multiple decimal points.
      digit_idx = i;
    } else if (c != '0') {
      non_zero_count++;
    } else if (std::isdigit(c)) {
      // Do nothing, valid digit.
    } else {
      return std::nullopt;  // Invalid character.
    }
  }

  if (digit_idx == -1) return std::nullopt;  // No decimal point.
  if (token.size() - digit_idx - 1 != required_digits)
    return std::nullopt;  // Wrong precision.
  if (non_zero_count == 0 && token[0] == '-') return std::nullopt;  // -0.0
  if (token.starts_with("00") || token.starts_with("-00"))
    return std::nullopt;  // Leading zeroes.
  if (token.starts_with(".") || token.starts_with("-."))
    return std::nullopt;  // Missing digit before decimal point.

  double result;
  auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(),
                                   result, std::chars_format::fixed);
  if (ec != std::errc{}) return std::nullopt;
  if (ptr != token.data() + token.size())
    return std::nullopt;  // Not all characters were consumed.

  return result;
}

}  // namespace

double BasicIStreamContext::ReadReal(int num_digits) {
  if (GetNumericStrictness() == NumericStrictness::kPrecise &&
      num_digits <= 0) {
    throw std::invalid_argument(
        "num_digits must be greater than 0 when specifying precise mode.");
  }
  std::string read_token = ReadToken();

  if (GetNumericStrictness() == NumericStrictness::kPrecise) {
    std::optional<double> result = ParseStrictReal(read_token, num_digits);
    if (!result.has_value()) {
      ThrowIOError(std::format(
          "Expected a real number with {} digits after the decimal point, "
          "but got '{}'.",
          num_digits, librarian::DebugString(read_token)));
    }
    return *result;
  }

  std::string_view token = read_token;

  // Small quirk: std::from_chars does not allow a leading '+'.
  if (token[0] == '+' && token.size() > 1 &&
      (std::isdigit(token[1]) || token[1] == '.')) {
    token.remove_prefix(1);
  }

  double result;
  auto [ptr, ec] =
      std::from_chars(token.data(), token.data() + token.size(), result);
  if (ec != std::errc{} || ptr != token.data() + token.size()) {
    ThrowIOError(std::format("Expected a real number, but got '{}'.",
                             librarian::DebugString(read_token)));
  }
  if (std::isinf(result) || std::isnan(result)) {
    ThrowIOError(std::format("Expected a real number, but got '{}'.",
                             librarian::DebugString(read_token)));
  }

  return result;
}

WhitespaceStrictness BasicIStreamContext::GetWhitespaceStrictness() const {
  return input_.get().whitespace_strictness;
}

NumericStrictness BasicIStreamContext::GetNumericStrictness() const {
  return input_.get().numeric_strictness;
}

std::istream& BasicIStreamContext::GetIStream() { return input_.get().is; }

InputCursor& BasicIStreamContext::GetCursor() { return input_; }

}  // namespace moriarty_internal
}  // namespace moriarty
