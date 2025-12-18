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

#ifndef MORIARTY_SIMPLE_IO_H_
#define MORIARTY_SIMPLE_IO_H_

#include <concepts>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "src/context.h"
#include "src/internal/expressions.h"

namespace moriarty {

// StringLiteral
//
// A string literal. Stores an std::string and can cast to an std::string.
class StringLiteral {
 public:
  explicit StringLiteral(std::string_view literal) : literal_(literal) {}
  explicit operator std::string() const { return literal_; }

  bool operator==(const StringLiteral& other) const {
    return literal_ == other.literal_;
  }

 private:
  std::string literal_;
};

// Token
//
// Each token in SimpleIO must be one of the following:
//  - `std::string`  : The name of a variable
//  - `StringLiteral`: An exact string to be read/write.
using SimpleIOToken = std::variant<std::string, StringLiteral>;

// SimpleIO
//
// For many situations, we just simply need the test cases to be read/write from
// a stream in a predictable way. `SimpleIO` works on tokens and lines. Each
// line is a sequence of tokens. The corresponding `Reader()` and `Writer()`
// will decide how the tokens are separated on each line.
class SimpleIO {
 public:
  // AddLine()
  //
  // For each test case, all tokens here will be with a single space
  // between them, followed by '\n'.
  //
  // For example:
  //   SimpleIO()
  //       .AddLine("N", "X")
  //       .AddLine(StringLiteral("Hello"), "P")
  //       .AddLine("A");
  //
  // states that each test case has 3 lines: (1) the variables `N` and `X`,
  // (2) the string "Hello" and the variable `P`, (3) the variable `A`.
  //
  // See also `AddLine(std::span<const std::string>)`.
  template <typename... Tokens>
    requires(std::convertible_to<Tokens, SimpleIOToken> && ...)
  SimpleIO& AddLine(Tokens&&... token);

  // AddLine()
  //
  // Each variable will be written over `number_of_lines_expression` lines. They
  // will then be zipped together.
  //
  // For example:
  //   AddMultilineSection(3, "N", "X"); // N = [1, 2, 3], X = [11, 22, 33]
  //
  // Writes:
  //   1 11
  //   2 22
  //   3 33
  template <typename... Tokens>
    requires(std::convertible_to<Tokens, SimpleIOToken> && ...)
  SimpleIO& AddMultilineSection(std::string_view number_of_lines_expression,
                                Tokens&&... token);

  // AddHeaderLine()
  //
  // These lines appear before all test cases. Similar format to
  // `AddLine()`.
  //
  // See also `AddHeaderLine(std::span<const std::string>)`.
  template <typename... Tokens>
    requires(std::convertible_to<Tokens, SimpleIOToken> && ...)
  SimpleIO& AddHeaderLine(Tokens&&... token);

  // AddFooterLine()
  //
  // These lines appear after all test cases. Similar format to `AddLine()`.
  //
  // See also `AddFooterLine(std::span<const std::string>)`.
  template <typename... Tokens>
    requires(std::convertible_to<Tokens, SimpleIOToken> && ...)
  SimpleIO& AddFooterLine(Tokens&&... token);

  // AddLine() for use when you don't know how many tokens you have and there
  // are no StringLiterals. Prefer to use `AddLine(Tokens... tokens).`
  SimpleIO& AddLine(std::span<const std::string> tokens);

  // AddHeaderLine() for use when you don't know how many tokens you have and
  // there are no StringLiterals. Prefer to use `AddHeaderLine(Tokens...
  // tokens).`
  SimpleIO& AddHeaderLine(std::span<const std::string> tokens);

  // AddFooterLine() for use when you don't know how many tokens you have and
  // there are no StringLiterals. Prefer to use `AddFooterLine(Tokens...
  // tokens).`
  SimpleIO& AddFooterLine(std::span<const std::string> tokens);

  // WithNumberOfTestCasesInHeader()
  //
  // The first line of the header (regardless of other calls to
  // `AddHeaderLine()`) will be a line containing a single integer, the number
  // of test cases.
  SimpleIO& WithNumberOfTestCasesInHeader();

  // Writer()
  //
  // Creates a SimpleIOWriter from the configuration provided by this class.
  // This can be passed into `Moriarty::WriteTestCases()`.
  [[nodiscard]] WriterFn Writer() const;

  // Reader()
  //
  // Creates a SimpleIOReader from the configuration provided by this class.
  // This can be passed into `Moriarty::ReadTestCases()`.
  [[nodiscard]] ReaderFn Reader(int number_of_test_cases = 1) const;

  // Access the lines
  struct Line {
    std::vector<SimpleIOToken> tokens;
    std::optional<Expression> num_lines;  // Set for multiline sections
  };
  [[nodiscard]] std::vector<Line> LinesInHeader() const;
  [[nodiscard]] std::vector<Line> LinesPerTestCase() const;
  [[nodiscard]] std::vector<Line> LinesInFooter() const;
  [[nodiscard]] bool HasNumberOfTestCasesInHeader() const;

 private:
  std::vector<Line> lines_in_header_;
  std::vector<Line> lines_per_test_case_;
  std::vector<Line> lines_in_footer_;

  bool has_number_of_test_cases_in_header_ = false;

  template <typename... Tokens>
  std::vector<SimpleIOToken> GetTokens(Tokens&&... token);
};

// -----------------------------------------------------------------------------
//  Template implementation below

template <typename... Tokens>
std::vector<SimpleIOToken> SimpleIO::GetTokens(Tokens&&... token) {
  std::vector<SimpleIOToken> line;
  line.reserve(sizeof...(Tokens));
  (line.push_back(std::forward<Tokens>(token)), ...);
  return line;
}

template <typename... Tokens>
  requires(std::convertible_to<Tokens, SimpleIOToken> && ...)
SimpleIO& SimpleIO::AddLine(Tokens&&... token) {
  lines_per_test_case_.push_back({GetTokens(std::forward<Tokens>(token)...)});
  return *this;
}

template <typename... Tokens>
  requires(std::convertible_to<Tokens, SimpleIOToken> && ...)
SimpleIO& SimpleIO::AddMultilineSection(
    std::string_view number_of_lines_expression, Tokens&&... token) {
  lines_per_test_case_.push_back({GetTokens(std::forward<Tokens>(token)...),
                                  Expression(number_of_lines_expression)});
  return *this;
}

template <typename... Tokens>
  requires(std::convertible_to<Tokens, SimpleIOToken> && ...)
SimpleIO& SimpleIO::AddHeaderLine(Tokens&&... token) {
  lines_in_header_.push_back({GetTokens(std::forward<Tokens>(token)...)});
  return *this;
}

template <typename... Tokens>
  requires(std::convertible_to<Tokens, SimpleIOToken> && ...)
SimpleIO& SimpleIO::AddFooterLine(Tokens&&... token) {
  lines_in_footer_.push_back({GetTokens(std::forward<Tokens>(token)...)});
  return *this;
}

}  // namespace moriarty

#endif  // MORIARTY_SIMPLE_IO_H_
