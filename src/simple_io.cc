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
#include "src/test_case.h"
#include "src/variables/minteger.h"

namespace moriarty {

namespace {
std::vector<ConcreteTestCase> ReadTestCases(ImportContext ctx,
                                            SimpleIO simple_io,
                                            int number_of_test_cases);
void PrintTestCases(ExportContext ctx, SimpleIO simple_io,
                    std::span<const ConcreteTestCase> test_cases);
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

ExportFn SimpleIO::Exporter() const {
  return
      [*this](ExportContext ctx, std::span<const ConcreteTestCase> test_cases) {
        return PrintTestCases(ctx, *this, test_cases);
      };
}

ImportFn SimpleIO::Importer(int number_of_test_cases) const {
  return [=, *this](ImportContext ctx) {
    return ReadTestCases(ctx, *this, number_of_test_cases);
  };
}

namespace {

// -----------------------------------------------------------------------------
//  SimpleIOExporter

void PrintToken(ExportContext ctx, const SimpleIOToken& token,
                const ConcreteTestCase& test_case) {
  if (std::holds_alternative<std::string>(token))
    ctx.PrintVariableFrom(std::get<std::string>(token), test_case);
  else
    ctx.PrintToken(std::string(std::get<StringLiteral>(token)));
}

// TODO: Clean up this function. It is pretty messy.
void PrintLine(ExportContext ctx, const SimpleIO::Line& line,
               const ConcreteTestCase& test_case) {
  if (!line.num_lines) {
    for (int line_idx = 0; const SimpleIOToken& token : line.tokens) {
      if (line_idx++) ctx.PrintWhitespace(Whitespace::kSpace);
      PrintToken(ctx, token, test_case);
    }
    ctx.PrintWhitespace(Whitespace::kNewline);
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
    PrintToken(ExportContext(ctx, ss), token, test_case);
    std::string lines = ss.str();
    if (lines.empty() || lines.back() != '\n') lines += '\n';
    int newlines = std::count(lines.begin(), lines.end(), '\n');
    if (newlines != line_count) {
      throw std::runtime_error(std::format(
          "Expected {} lines in printout of variable {}, but got {}",
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
          "Error printing line {} of variable {} (missing '\\n'?): {}",
          line_num, std::get<std::string>(token), e.what()));
    }
  }

  for (std::string_view line : output) {
    ctx.PrintToken(line);
    ctx.PrintWhitespace(Whitespace::kNewline);
  }
}

void PrintLines(ExportContext ctx, std::span<const SimpleIO::Line> lines,
                const ConcreteTestCase& test_case) {
  for (const SimpleIO::Line& line : lines) PrintLine(ctx, line, test_case);
}

void PrintLiteralOnlyLines(ExportContext ctx,
                           std::span<const SimpleIO::Line> lines) {
  for (const SimpleIO::Line& line : lines) {
    for (int line_idx = 0; const SimpleIOToken& token : line.tokens) {
      if (line_idx++) ctx.PrintWhitespace(Whitespace::kSpace);

      if (std::holds_alternative<std::string>(token))
        throw std::runtime_error("Cannot have variable in Header/Footer");
      ctx.PrintToken(std::string(std::get<StringLiteral>(token)));
    }
    ctx.PrintWhitespace(Whitespace::kNewline);
  }
}

void PrintTestCases(ExportContext ctx, SimpleIO simple_io,
                    std::span<const ConcreteTestCase> test_cases) {
  if (simple_io.HasNumberOfTestCasesInHeader()) {
    ctx.PrintToken(std::to_string(test_cases.size()));
    ctx.PrintWhitespace(Whitespace::kNewline);
  }
  PrintLiteralOnlyLines(ctx, simple_io.LinesInHeader());

  for (const ConcreteTestCase& test_case : test_cases) {
    PrintLines(ctx, simple_io.LinesPerTestCase(), test_case);
  }

  PrintLiteralOnlyLines(ctx, simple_io.LinesInFooter());
}

// -----------------------------------------------------------------------------
//  SimpleIOImporter

// TODO: ReadVariable(MInteger(AtLeast(0)));
int64_t ReadNumTestCases(ImportContext ctx) {
  int64_t num_cases = ctx.ReadVariable(MInteger(AtLeast(0)), "num_cases");
  ctx.ReadWhitespace(Whitespace::kNewline);
  return num_cases;
}

void ReadLiteral(ImportContext ctx, const StringLiteral& literal) {
  std::string read_token = ctx.ReadToken();
  std::string expected = std::string(literal);
  if (read_token != expected) {
    ctx.ThrowIOError(
        std::format("Expected '{}', but got '{}'.", expected, read_token));
  }
}

void ReadToken(ImportContext ctx, const SimpleIOToken& token,
               ConcreteTestCase& test_case) {
  if (std::holds_alternative<std::string>(token))
    ctx.ReadVariableTo(std::get<std::string>(token), test_case);
  else
    ReadLiteral(ctx, std::get<StringLiteral>(token));
}

void ReadLine(ImportContext ctx, const SimpleIO::Line& line,
              ConcreteTestCase& test_case) {
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

  std::vector<std::unique_ptr<moriarty_internal::PartialReader>> readers;
  for (const auto& var : line.tokens) {
    if (std::holds_alternative<StringLiteral>(var)) {
      throw std::runtime_error("Cannot have literal in multiline section");
    }
    readers.push_back(ctx.GetPartialReader(std::get<std::string>(var),
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

void ReadLiteralOnlyLine(ImportContext ctx, const SimpleIO::Line& line) {
  for (int line_idx = 0; const SimpleIOToken& token : line.tokens) {
    if (line_idx++) ctx.ReadWhitespace(Whitespace::kSpace);

    if (std::holds_alternative<std::string>(token))
      throw std::runtime_error("Cannot have variable in Header");
    ReadLiteral(ctx, std::get<StringLiteral>(token));
  }
  ctx.ReadWhitespace(Whitespace::kNewline);
}

void ReadLiteralOnlyLines(ImportContext ctx,
                          std::span<const SimpleIO::Line> lines) {
  for (const SimpleIO::Line& line : lines) ReadLiteralOnlyLine(ctx, line);
}

void ReadLines(ImportContext ctx, std::span<const SimpleIO::Line> lines,
               ConcreteTestCase& test_case) {
  for (const SimpleIO::Line& line : lines) ReadLine(ctx, line, test_case);
}

std::vector<ConcreteTestCase> ReadTestCases(ImportContext ctx,
                                            SimpleIO simple_io,
                                            int number_of_test_casese) {
  int64_t num_cases = simple_io.HasNumberOfTestCasesInHeader()
                          ? ReadNumTestCases(ctx)
                          : number_of_test_casese;
  ReadLiteralOnlyLines(ctx, simple_io.LinesInHeader());
  std::vector<ConcreteTestCase> test_cases;
  for (int i = 0; i < num_cases; i++) {
    test_cases.push_back(ConcreteTestCase());
    ReadLines(ctx, simple_io.LinesPerTestCase(), test_cases.back());
  }
  ReadLiteralOnlyLines(ctx, simple_io.LinesInFooter());

  return test_cases;
}

}  // namespace

}  // namespace moriarty
