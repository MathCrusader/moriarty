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
#include <cstdint>
#include <format>
#include <functional>
#include <istream>
#include <string>
#include <string_view>

#include "src/librarian/errors.h"
#include "src/librarian/io_config.h"
#include "src/librarian/policies.h"
#include "src/librarian/util/debug_string.h"

namespace moriarty {
namespace moriarty_internal {

BasicIStreamContext::BasicIStreamContext(
    std::reference_wrapper<InputCursor> input)
    : input_(input) {}

void BasicIStreamContext::ThrowIOError(std::string_view message) const {
  throw IOError(input_.get(), message);
}

namespace {

bool IsEOF(std::istream& is) {
  if (is.eof()) return true;
  (void)is.peek();
  return is.eof();
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
  assert(false);
}

void RegisterNewline(char c, InputCursor& cursor) {
  if (c != '\n') return;
  cursor.line_num++;
  cursor.col_num = 0;
  cursor.token_num_line = 0;
}

void StripLeadingWhitespace(std::istream& is, InputCursor& cursor) {
  while (is) {
    char c = is.peek();
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

  if (GetStrictness() == WhitespaceStrictness::kFlexible)
    StripLeadingWhitespace(is, GetCursor());

  if (IsEOF(is)) ThrowIOError("Expected a token, but got EOF.");

  if (GetStrictness() == WhitespaceStrictness::kPrecise) {
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

void BasicIStreamContext::ReadEof() {
  std::istream& is = GetIStream();

  if (GetStrictness() == WhitespaceStrictness::kFlexible)
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

  if (GetStrictness() == WhitespaceStrictness::kFlexible) return;

  if (c != WhitespaceAsChar(whitespace)) {
    ThrowIOError(std::format("Expected '{}', but got '{}'.",
                             ReadableChar(WhitespaceAsChar(whitespace)),
                             ReadableChar(c)));
  }
}

int64_t BasicIStreamContext::ReadInteger() {
  std::string read_token = ReadToken();
  bool negative = read_token[0] == '-';
  std::string_view token = read_token;
  if (negative) token.remove_prefix(1);

  for (char c : token) {
    if (!std::isdigit(c))
      ThrowIOError(std::format("Expected an integer, but got '{}'.",
                               librarian::DebugString(read_token)));
  }
  if (token.empty())
    ThrowIOError(std::format("Expected an integer, but got '{}'.",
                             librarian::DebugString(read_token)));
  // 2^63 == 9223372036854775808, which has 19 digits.
  if (token.size() > 19) {
    ThrowIOError(std::format("Value does not fit into a 64-bit integer '{}'.",
                             librarian::DebugString(read_token)));
  }
  if (token.size() == 19) {
    if (negative && token == "9223372036854775808")
      return std::numeric_limits<int64_t>::min();

    if (token > "9223372036854775807")
      ThrowIOError(std::format("Value does not fit into a 64-bit integer '{}'.",
                               librarian::DebugString(read_token)));
  }

  // TODO: Add checks for strict mode (-0, leading zeros)
  return std::stoll(read_token);
}

WhitespaceStrictness BasicIStreamContext::GetStrictness() const {
  return input_.get().strictness;
}

std::istream& BasicIStreamContext::GetIStream() { return input_.get().is; }

InputCursor& BasicIStreamContext::GetCursor() { return input_; }

}  // namespace moriarty_internal
}  // namespace moriarty
