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
#include <format>
#include <functional>
#include <istream>
#include <string>

#include "src/librarian/io_config.h"
#include "src/librarian/policies.h"

namespace moriarty {
namespace moriarty_internal {

BasicIStreamContext::BasicIStreamContext(
    std::reference_wrapper<std::istream> is,
    moriarty::WhitespaceStrictness strictness)
    : is_(is), strictness_(strictness) {
  is_.get().exceptions(std::istream::failbit | std::istream::badbit);
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

void StripLeadingWhitespace(std::istream& is) {
  if (!is.eof()) is >> std::ws;
}

}  // namespace

std::string BasicIStreamContext::ReadToken() {
  std::istream& is = is_.get();

  if (strictness_ == WhitespaceStrictness::kFlexible)
    StripLeadingWhitespace(is);

  if (IsEOF(is))
    throw std::runtime_error("Attempted to read a token, but got EOF.");

  if (strictness_ == WhitespaceStrictness::kPrecise) {
    char c = is.peek();
    if (!std::isprint(c) || std::isspace(c)) {
      throw std::runtime_error(
          std::format("Expected token, but got '{}'.", ReadableChar(c)));
    }
  }

  // At this point, we are not at EOF and there is no leading whitespace.
  std::string token;
  is >> token;
  return token;
}

void BasicIStreamContext::ReadEof() {
  if (strictness_ == WhitespaceStrictness::kFlexible)
    StripLeadingWhitespace(is_.get());

  if (!IsEOF(is_.get())) {
    throw std::runtime_error("Expected EOF, but got more input.");
  }
}

void BasicIStreamContext::ReadWhitespace(Whitespace whitespace) {
  if (IsEOF(is_.get())) {
    throw std::runtime_error(
        std::format("Attempted to read '{}', but got EOF.",
                    ReadableChar(WhitespaceAsChar(whitespace))));
  }
  char c = is_.get().get();
  if (!std::isspace(c)) {
    throw std::runtime_error(
        std::format("Expected whitespace, but got '{}'.", ReadableChar(c)));
  }

  if (strictness_ == WhitespaceStrictness::kFlexible) return;

  if (c != WhitespaceAsChar(whitespace)) {
    throw std::runtime_error(std::format(
        "Expected '{}', but got '{}'.",
        ReadableChar(WhitespaceAsChar(whitespace)), ReadableChar(c)));
  }
}

}  // namespace moriarty_internal
}  // namespace moriarty
