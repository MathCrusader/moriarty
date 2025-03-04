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
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "src/contexts/librarian/analysis_context.h"
#include "src/contexts/librarian/printer_context.h"
#include "src/contexts/librarian/reader_context.h"
#include "src/contexts/librarian/resolver_context.h"
#include "src/errors.h"
#include "src/property.h"
#include "src/util/status_macro/status_macros.h"
#include "src/variables/minteger.h"

// LINT.IfChange

namespace moriarty_testing {

MTestType::MTestType() {
  RegisterKnownProperty("size", &MTestType::WithSizeProperty);
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

// Acceptable `KnownProperty`s: "size":"small"/"large"
absl::Status MTestType::WithSizeProperty(moriarty::Property property) {
  // Size is the only known property
  ABSL_CHECK_EQ(property.category, "size");

  if (property.descriptor == "small")
    Is(kGeneratedValueSmallSize);
  else if (property.descriptor == "large")
    Is(kGeneratedValueLargeSize);
  else
    return absl::InvalidArgumentError(absl::Substitute(
        "Unknown property descriptor: $0", property.descriptor));

  return absl::OkStatus();
}

MTestType& MTestType::SetAdder(absl::string_view variable_name) {
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
    MORIARTY_ASSIGN_OR_RETURN(
        TestType subtract_me, ctx.GetValue<MTestType>(*adder_variable_name_),
        _ << "Unknown adder variable: " << *adder_variable_name_);
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

std::vector<std::string> MTestType::GetDependenciesImpl() const {
  return multiplier_.GetDependencies();
}

// Always returns pi. Does not directly depend on `rng`, but we generate a
// random number between 1 and 1 (aka, 1) to ensure the RandomEngine is
// available for use if we wanted to.
TestType MTestType::GenerateImpl(
    moriarty::librarian::ResolverContext ctx) const {
  TestType addition = 0;
  if (adder_variable_name_) {
    addition = ctx.GenerateVariable<MTestType>(*adder_variable_name_);
  }

  int64_t multiplier = multiplier_.Generate(ctx.ForSubVariable("multiplier"));
  return kGeneratedValue * multiplier + addition;
}

absl::StatusOr<std::vector<MTestType>> MTestType::GetDifficultInstancesImpl(
    moriarty::librarian::AnalysisContext ctx) const {
  return std::vector<MTestType>({MTestType().Is(2), MTestType().Is(3)});
}

}  // namespace moriarty_testing

// LINT.ThenChange(mtest_type2.cc)
