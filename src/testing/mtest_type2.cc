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

#include "src/testing/mtest_type2.h"

#include <stdint.h>

#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/substitute.h"
#include "src/contexts/librarian/analysis_context.h"
#include "src/contexts/librarian/printer_context.h"
#include "src/contexts/librarian/reader_context.h"
#include "src/contexts/librarian/resolver_context.h"
#include "src/errors.h"
#include "src/variables/minteger.h"

// LINT.IfChange

namespace moriarty_testing {

TestType2 MTestType2::ReadImpl(moriarty::librarian::ReaderContext ctx) const {
  std::string token = ctx.ReadToken();
  std::stringstream is(token);

  int64_t value;
  if (!(is >> value)) throw std::runtime_error("Unable to read a TestType.");
  return value;
}

void MTestType2::PrintImpl(moriarty::librarian::PrinterContext ctx,
                           const TestType2& value) const {
  ctx.PrintToken(std::to_string(value));
}

absl::Status MTestType2::MergeFromImpl(const MTestType2& other) {
  merged_ = true;
  return absl::OkStatus();
}

bool MTestType2::WasMerged() const { return merged_; }

MTestType2& MTestType2::SetAdder(std::string_view variable_name) {
  adder_variable_name_ = variable_name;
  return *this;
}

// My value is multiplied by this value
MTestType2& MTestType2::SetMultiplier(moriarty::MInteger multiplier) {
  multiplier_ = multiplier;
  return *this;
}

// I am == "multiplier * value + other_variable", so to be valid,
//   (valid - other_variable) / multiplier
// must be an integer.
absl::Status MTestType2::IsSatisfiedWithImpl(
    moriarty::librarian::AnalysisContext ctx, const TestType2& value) const {
  TestType2 val = value;
  if (adder_variable_name_) {
    TestType2 subtract_me = ctx.GetValue<MTestType2>(*adder_variable_name_);
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
        absl::Status status1 = multiplier_.IsSatisfiedWith(ctx, div);
        if (!status1.ok() && !moriarty::IsUnsatisfiedConstraintError(status1))
          return status1;

        absl::Status status2 = multiplier_.IsSatisfiedWith(ctx, val / div);
        if (!status2.ok() && !moriarty::IsUnsatisfiedConstraintError(status2))
          return status2;

        if (status1.ok() || status2.ok()) found_divisor = true;
      }
    }
    if (!found_divisor)
      return moriarty::UnsatisfiedConstraintError(
          absl::Substitute("$0 is not a multiple of any valid multiplier.",
                           static_cast<int>(val)));
  }

  return absl::OkStatus();
}

std::vector<std::string> MTestType2::GetDependenciesImpl() const {
  return multiplier_.GetDependencies();
}

// Always returns pi. Does not directly depend on `rng`, but we generate a
// random number between 1 and 1 (aka, 1) to ensure the RandomEngine is
// available for use if we wanted to.
TestType2 MTestType2::GenerateImpl(
    moriarty::librarian::ResolverContext ctx) const {
  TestType2 addition = 0;
  if (adder_variable_name_) {
    addition = ctx.GenerateVariable<MTestType2>(*adder_variable_name_);
  }

  int64_t multiplier = multiplier_.Generate(ctx.ForSubVariable("multiplier"));
  return kGeneratedValue * multiplier + addition;
}

absl::StatusOr<std::vector<MTestType2>> MTestType2::GetDifficultInstancesImpl(
    moriarty::librarian::AnalysisContext ctx) const {
  return std::vector<MTestType2>({MTestType2().Is(2), MTestType2().Is(3)});
}

}  // namespace moriarty_testing

// LINT.ThenChange(mtest_type.cc)
