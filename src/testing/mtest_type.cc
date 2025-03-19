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

#include "absl/status/status.h"
#include "absl/strings/substitute.h"
#include "src/contexts/librarian/analysis_context.h"
#include "src/contexts/librarian/printer_context.h"
#include "src/contexts/librarian/reader_context.h"
#include "src/contexts/librarian/resolver_context.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/minteger.h"

// LINT.IfChange

namespace moriarty_testing {

MTestType& MTestType::AddConstraint(moriarty::Exactly<TestType> constraint) {
  one_of_.ConstrainOptions(std::vector{constraint.GetValue()});
  return InternalAddConstraint(std::move(constraint));
}

MTestType& MTestType::AddConstraint(moriarty::OneOf<TestType> constraint) {
  one_of_.ConstrainOptions(constraint.GetOptions());
  return InternalAddConstraint(std::move(constraint));
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

absl::Status MTestType::MergeFromImpl(const MTestType& other) {
  merged_ = true;
  // SetMultiplier(other.mu)
  // multiplier_.MergeFrom(other.multiplier_);
  return absl::OkStatus();
}

bool MTestType::WasMerged() const { return merged_; }

MTestType& MTestType::SetAdder(std::string_view variable_name) {
  adder_variable_name_ = variable_name;
  return *this;
}

// My value is multiplied by this value
MTestType& MTestType::SetMultiplier(moriarty::MInteger multiplier) {
  multiplier_ = multiplier;
  return *this;
}

// I am == "multiplier * value + other_variable", so to be valid,
//   (valid - other_variable) / multiplier
// must be an integer.
absl::Status MTestType::IsSatisfiedWithImpl(
    moriarty::librarian::AnalysisContext ctx, const TestType& value) const {
  TestType val = value;
  if (adder_variable_name_) {
    TestType subtract_me = ctx.GetValue<MTestType>(*adder_variable_name_);
    val = val - subtract_me;
  }

  // 0 is a multiple of all numbers!
  if (val == 0) return absl::OkStatus();

  {
    // value must be a multiple of `multiplier`. So I'll go through all my
    // factors and one of those must be in the range...
    if (val < 0) val = -val;
    bool found_divisor = false;
    for (int div = 1; div * div <= val; div++) {
      if (val % div == 0) {
        if (multiplier_.IsSatisfiedWith(ctx, div) ||
            multiplier_.IsSatisfiedWith(ctx, val / div)) {
          found_divisor = true;
        }
      }
    }
    if (!found_divisor)
      return absl::InvalidArgumentError(
          absl::Substitute("$0 is not a multiple of any valid multiplier.",
                           static_cast<int>(val)));
  }

  return absl::OkStatus();
}

// Always returns pi. Does not directly depend on `rng`, but we generate a
// random number between 1 and 1 (aka, 1) to ensure the RandomEngine is
// available for use if we wanted to.
TestType MTestType::GenerateImpl(
    moriarty::librarian::ResolverContext ctx) const {
  if (one_of_.HasBeenConstrained())
    return one_of_.SelectOneOf([&](int n) { return ctx.RandomInteger(n); });

  TestType addition = 0;
  if (adder_variable_name_) {
    addition = ctx.GenerateVariable<MTestType>(*adder_variable_name_);
  }

  int64_t multiplier = multiplier_.Generate(ctx.ForSubVariable("multiplier"));
  return kGeneratedValue * multiplier + addition;
}

std::vector<MTestType> MTestType::ListEdgeCasesImpl(
    moriarty::librarian::AnalysisContext ctx) const {
  return std::vector<MTestType>(
      {MTestType().AddConstraint(moriarty::Exactly<TestType>(2)),
       MTestType().AddConstraint(moriarty::Exactly<TestType>(3))});
}

}  // namespace moriarty_testing

// LINT.ThenChange(mtest_type2.cc)
