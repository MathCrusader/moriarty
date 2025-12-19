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

#include "src/simple_io.h"

#include <algorithm>
#include <cstdio>
#include <format>
#include <memory>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "src/constraints/numeric_constraints.h"
#include "src/context.h"
#include "src/librarian/errors.h"
#include "src/test_case.h"
#include "src/variables/minteger.h"

namespace moriarty {

namespace {
std::vector<TestCase> ReadTestCases(ReadContext ctx, SimpleIO simple_io,
                                    int number_of_test_cases);
void WriteTestCases(WriteContext ctx, SimpleIO simple_io,
                    std::span<const TestCase> test_cases);
}  // namespace

// -----------------------------------------------------------------------------
//  SimpleIO

SimpleIO& SimpleIO::AddLine(std::span<const std::string> tokens) {
  lines_per_test_case_.push_back(
      {std::vector<SimpleIOToken>(tokens.begin(), tokens.end())});
  return *this;
}

SimpleIO& SimpleIO::AddHeaderLine(std::span<const std::string> tokens) {
  lines_in_header_.push_back(
      {std::vector<SimpleIOToken>(tokens.begin(), tokens.end())});
  return *this;
}

SimpleIO& SimpleIO::AddFooterLine(std::span<const std::string> tokens) {
  lines_in_footer_.push_back(
      {std::vector<SimpleIOToken>(tokens.begin(), tokens.end())});
  return *this;
}

std::vector<SimpleIO::Line> SimpleIO::LinesInHeader() const {
  return lines_in_header_;
}

std::vector<SimpleIO::Line> SimpleIO::LinesPerTestCase() const {
  return lines_per_test_case_;
}

std::vector<SimpleIO::Line> SimpleIO::LinesInFooter() const {
  return lines_in_footer_;
}

SimpleIO& SimpleIO::WithNumberOfTestCasesInHeader() {
  has_number_of_test_cases_in_header_ = true;
  return *this;
}

bool SimpleIO::HasNumberOfTestCasesInHeader() const {
  return has_number_of_test_cases_in_header_;
}

WriterFn SimpleIO::Writer() const {
  return [*this](WriteContext ctx, std::span<const TestCase> test_cases) {
    return WriteTestCases(ctx, *this, test_cases);
  };
}

ReaderFn SimpleIO::Reader(int number_of_test_cases) const {
  return [=, *this](ReadContext ctx) {
    return ReadTestCases(ctx, *this, number_of_test_cases);
  };
}

namespace {

// -----------------------------------------------------------------------------
//  SimpleIOWriter

void WriteToken(WriteContext ctx, const SimpleIOToken& token,
                const TestCase& test_case) {
  if (std::holds_alternative<std::string>(token))
    ctx.WriteVariableFrom(std::get<std::string>(token), test_case);
  else
    ctx.WriteToken(std::string(std::get<StringLiteral>(token)));
}

// TODO: Clean up this function. It is pretty messy.
void WriteLine(WriteContext ctx, const SimpleIO::Line& line,
               const TestCase& test_case) {
  if (!line.num_lines) {
    for (int line_idx = 0; const SimpleIOToken& token : line.tokens) {
      if (line_idx++) ctx.WriteWhitespace(Whitespace::kSpace);
      WriteToken(ctx, token, test_case);
    }
    ctx.WriteWhitespace(Whitespace::kNewline);
    return;
  }

  int64_t line_count = line.num_lines->Evaluate(
      [&](std::string_view var) { return test_case.GetValue<MInteger>(var); });
  if (line_count < 0) {
    throw std::runtime_error(
        std::format("Number of lines in SimpleIO must be >= 0. Got: {} ({})",
                    line_count, line.num_lines->ToString()));
  }

  std::vector<std::string> output(line_count);
  for (int var_idx = -1; const SimpleIOToken& token : line.tokens) {
    var_idx++;
    std::stringstream ss;
    WriteToken(WriteContext(ctx, ss), token, test_case);
    std::string lines = ss.str();
    if (lines.empty() || lines.back() != '\n') lines += '\n';
    int newlines = std::ranges::count(lines, '\n');
    if (newlines != line_count) {
      throw std::runtime_error(std::format(
          "Expected {} lines in writeout of variable {}, but got {}",
          line_count, std::get<std::string>(token), newlines));
    }

    std::string line;
    int line_num = 0;
    try {
      for (; line_num < line_count && std::getline(ss, line); line_num++) {
        if (var_idx != 0) output[line_num] += ' ';
        output[line_num] += line;
      }
    } catch (const std::exception& e) {
      throw std::runtime_error(std::format(
          "Error writeing line {} of variable {} (missing '\\n'?): {}",
          line_num, std::get<std::string>(token), e.what()));
    }
  }

  for (std::string_view line : output) {
    ctx.WriteToken(line);
    ctx.WriteWhitespace(Whitespace::kNewline);
  }
}

void WriteLines(WriteContext ctx, std::span<const SimpleIO::Line> lines,
                const TestCase& test_case) {
  for (const SimpleIO::Line& line : lines) WriteLine(ctx, line, test_case);
}

void WriteLiteralOnlyLines(WriteContext ctx,
                           std::span<const SimpleIO::Line> lines) {
  for (const SimpleIO::Line& line : lines) {
    for (int line_idx = 0; const SimpleIOToken& token : line.tokens) {
      if (line_idx++) ctx.WriteWhitespace(Whitespace::kSpace);

      if (std::holds_alternative<std::string>(token))
        throw ConfigurationError("SimpleIO",
                                 "Cannot have variable in Header/Footer");
      ctx.WriteToken(std::string(std::get<StringLiteral>(token)));
    }
    ctx.WriteWhitespace(Whitespace::kNewline);
  }
}

void WriteTestCases(WriteContext ctx, SimpleIO simple_io,
                    std::span<const TestCase> test_cases) {
  if (simple_io.HasNumberOfTestCasesInHeader()) {
    ctx.WriteToken(std::to_string(test_cases.size()));
    ctx.WriteWhitespace(Whitespace::kNewline);
  }
  WriteLiteralOnlyLines(ctx, simple_io.LinesInHeader());

  for (const TestCase& test_case : test_cases) {
    WriteLines(ctx, simple_io.LinesPerTestCase(), test_case);
  }

  WriteLiteralOnlyLines(ctx, simple_io.LinesInFooter());
}

// -----------------------------------------------------------------------------
//  SimpleIOReader

// TODO: ReadVariable(MInteger(AtLeast(0)));
int64_t ReadNumTestCases(ReadContext ctx) {
  int64_t num_cases = ctx.ReadVariable(MInteger(AtLeast(0)), "num_cases");
  ctx.ReadWhitespace(Whitespace::kNewline);
  return num_cases;
}

void ReadLiteral(ReadContext ctx, const StringLiteral& literal) {
  std::string read_token = ctx.ReadToken();
  std::string expected = std::string(literal);
  if (read_token != expected) {
    ctx.ThrowIOError("Expected '{}', but got '{}'.", expected, read_token);
  }
}

void ReadToken(ReadContext ctx, const SimpleIOToken& token,
               TestCase& test_case) {
  try {
    if (std::holds_alternative<std::string>(token))
      ctx.ReadVariableTo(std::get<std::string>(token), test_case);
    else
      ReadLiteral(ctx, std::get<StringLiteral>(token));
  } catch (const IOError& e) {
    throw;
  } catch (const GenericMoriartyError& e) {
    ctx.ThrowIOError(e.what());
  }
}

void ReadLine(ReadContext ctx, const SimpleIO::Line& line,
              TestCase& test_case) {
  if (!line.num_lines) {
    for (int line_idx = 0; const SimpleIOToken& token : line.tokens) {
      if (line_idx++) ctx.ReadWhitespace(Whitespace::kSpace);
      ReadToken(ctx, token, test_case);
    }
    ctx.ReadWhitespace(Whitespace::kNewline);
    return;
  }

  int64_t line_count = line.num_lines->Evaluate(
      [&](std::string_view var) { return test_case.GetValue<MInteger>(var); });
  if (line_count < 0) {
    throw std::runtime_error(
        std::format("Number of lines in SimpleIO must be >= 0. Got: {} ({})",
                    line_count, line.num_lines->ToString()));
  }

  std::vector<std::unique_ptr<moriarty_internal::ChunkedReader>> readers;
  for (const auto& var : line.tokens) {
    if (std::holds_alternative<StringLiteral>(var)) {
      throw ConfigurationError("SimpleIO",
                               "Cannot have literal in multiline section");
    }
    readers.push_back(ctx.GetChunkedReader(std::get<std::string>(var),
                                           line_count, test_case));
  }

  for (int i = 0; i < line_count; i++) {
    for (int var_idx = 0; var_idx < readers.size(); var_idx++) {
      if (var_idx) ctx.ReadWhitespace(Whitespace::kSpace);
      readers[var_idx]->ReadNext();
    }
    ctx.ReadWhitespace(Whitespace::kNewline);
  }
  for (auto& reader : readers) {
    std::move(*reader).Finalize();
    reader.reset();
  }
}

void ReadLiteralOnlyLine(ReadContext ctx, const SimpleIO::Line& line) {
  for (int line_idx = 0; const SimpleIOToken& token : line.tokens) {
    if (line_idx++) ctx.ReadWhitespace(Whitespace::kSpace);

    if (std::holds_alternative<std::string>(token))
      throw ConfigurationError("SimpleIO", "Cannot have variable in Header");
    ReadLiteral(ctx, std::get<StringLiteral>(token));
  }
  ctx.ReadWhitespace(Whitespace::kNewline);
}

void ReadLiteralOnlyLines(ReadContext ctx,
                          std::span<const SimpleIO::Line> lines) {
  for (const SimpleIO::Line& line : lines) ReadLiteralOnlyLine(ctx, line);
}

void ReadLines(ReadContext ctx, std::span<const SimpleIO::Line> lines,
               TestCase& test_case) {
  for (const SimpleIO::Line& line : lines) ReadLine(ctx, line, test_case);
}

std::vector<TestCase> ReadTestCases(ReadContext ctx, SimpleIO simple_io,
                                    int number_of_test_casese) {
  int64_t num_cases = simple_io.HasNumberOfTestCasesInHeader()
                          ? ReadNumTestCases(ctx)
                          : number_of_test_casese;
  ReadLiteralOnlyLines(ctx, simple_io.LinesInHeader());
  std::vector<TestCase> test_cases;
  for (int i = 0; i < num_cases; i++) {
    test_cases.push_back(TestCase());
    ReadLines(ctx, simple_io.LinesPerTestCase(), test_cases.back());
  }
  ReadLiteralOnlyLines(ctx, simple_io.LinesInFooter());

  return test_cases;
}

}  // namespace

}  // namespace moriarty
