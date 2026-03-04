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

#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <format>
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
  input_.get().ThrowIOError(
      message, InputCursor::ErrorSnippetCaretPosition::kBeforePreviousRead);
}

void BasicIStreamContext::ThrowIOErrorWithContext(
    std::string_view message, std::string_view context) const {
  input_.get().ThrowIOError(
      message, InputCursor::ErrorSnippetCaretPosition::kBeforePreviousRead,
      context);
}

std::string BasicIStreamContext::ReadToken() { return GetCursor().ReadToken(); }

std::optional<std::string> BasicIStreamContext::PeekToken() {
  return GetCursor().PeekToken();
}

void BasicIStreamContext::ReadEof() { GetCursor().ReadEof(); }

void BasicIStreamContext::ReadWhitespace(Whitespace whitespace) {
  GetCursor().ReadWhitespace(whitespace);
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
      ThrowIOError("Expected a (strict) integer, but got '{}'.",
                   librarian::DebugString(read_token));
    }
    if (value != 0 && read_token[0] == '0') {
      ThrowIOError("Expected a (strict) integer, but got '{}'.",
                   librarian::DebugString(read_token));
    }
    if (value != 0 && (read_token[0] == '-' && read_token[1] == '0')) {
      ThrowIOError("Expected a (strict) integer, but got '{}'.",
                   librarian::DebugString(read_token));
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
        std::format("RandReal({}): num_digits must be greater than 0 when "
                    "in precise numeric mode.",
                    num_digits));
  }
  std::string read_token = ReadToken();

  if (GetNumericStrictness() == NumericStrictness::kPrecise) {
    std::optional<double> result = ParseStrictReal(read_token, num_digits);
    if (!result.has_value()) {
      ThrowIOError(
          "Expected a real number with {} digits after the decimal point, "
          "but got {}.",
          num_digits, librarian::DebugString(read_token));
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
    ThrowIOError("Expected a real number, but got '{}'.",
                 librarian::DebugString(read_token));
  }
  if (std::isinf(result) || std::isnan(result)) {
    ThrowIOError("Expected a real number, but got '{}'.",
                 librarian::DebugString(read_token));
  }

  return result;
}

WhitespaceStrictness BasicIStreamContext::GetWhitespaceStrictness() const {
  return input_.get().GetWhitespaceStrictness();
}

NumericStrictness BasicIStreamContext::GetNumericStrictness() const {
  return input_.get().GetNumericStrictness();
}

InputCursor& BasicIStreamContext::GetCursor() { return input_; }

}  // namespace moriarty_internal
}  // namespace moriarty
