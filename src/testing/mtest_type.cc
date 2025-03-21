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

#include "src/testing/mtest_type.h"

#include <stdint.h>

#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "absl/strings/substitute.h"
#include "src/contexts/librarian/analysis_context.h"
#include "src/contexts/librarian/printer_context.h"
#include "src/contexts/librarian/reader_context.h"
#include "src/contexts/librarian/resolver_context.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/minteger.h"

namespace moriarty_testing {

MTestType& MTestType::AddConstraint(moriarty::Exactly<TestType> constraint) {
  one_of_.ConstrainOptions(std::vector{constraint.GetValue()});
  return InternalAddConstraint(std::move(constraint));
}

MTestType& MTestType::AddConstraint(moriarty::OneOf<TestType> constraint) {
  one_of_.ConstrainOptions(constraint.GetOptions());
  return InternalAddConstraint(std::move(constraint));
}

MTestType& AddConstraint(LastDigit constraint) {
  if (last_digit_) last_digit_ = moriarty::MInteger();
  last_digit_.MergeFrom(constraint.GetDigit());
  return InternalAddConstraint(std::move(constraint));
}

MTestType& AddConstraint(NumberOfDigits constraint) {
  if (num_digits_) num_digits_ = moriarty::MInteger();
  num_digits_.MergeFrom(constraint.GetDigit());
  return InternalAddConstraint(std::move(constraint));
}

std::optional<TestType> MTestType::GetUniqueValueImpl(
    moriarty::librarian::AnalysisContext ctx) const override {
  if (one_of_.HasBeenConstrained()) return one_of_.GetUniqueValue();
  if (!last_digit_ || !last_digit_.GetUniqueValue(ctx)) return std::nullopt;
  if (!num_digits_ || !num_digits_.GetUniqueValue(ctx) != 1)
    return std::nullopt;
  return last_digit_.GetUniqueValue(ctx);  // A single digit number.
}

TestType MTestType::ReadImpl(moriarty::librarian::ReaderContext ctx) const {
  std::string token = ctx.ReadToken();
  std::stringstream is(token);

  int64_t value;
  if (!(is >> value)) throw std::runtime_error("Unable to read a TestType.");
  return value;
}

void MTestType::PrintImpl(moriarty::librarian::PrinterContext ctx,
                          const TestType& value) const {
  ctx.PrintToken(std::to_string(value));
}

// By default, return 123456789, but maybe slightly modified.
TestType MTestType::GenerateImpl(
    moriarty::librarian::ResolverContext ctx) const {
  if (one_of_.HasBeenConstrained())
    return one_of_.SelectOneOf([&](int n) { return ctx.RandomInteger(n); });

  int x = 0;
  if (ctx.RandomInteger(2) == 0) x++;  // Just to use the random engine.

  int last_digit = last_digit_ ? last_digit_.Generate(ctx) : 9;
  int num_digits =
      num_digits_
          ? MInteger(num_digits_).AddConstraint(Between(1, 9)).Generate(ctx)
          : 9;

  int result = 123456780 + last_digit;
  string s = std::to_string(result);
  while (s.size() > num_digits) s = s.substr(1);
  return TestType(std::stoll(s));
}

std::vector<MTestType> MTestType::ListEdgeCasesImpl(
    moriarty::librarian::AnalysisContext ctx) const {
  return std::vector<MTestType>({MTestType(moriarty::Exactly<TestType>(2)),
                                 MTestType(moriarty::Exactly<TestType>(3))});
}

}  // namespace moriarty_testing
