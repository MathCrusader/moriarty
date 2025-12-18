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

#include "src/variables/mreal.h"

#include <cstdint>
#include <format>
#include <optional>

#include "src/constraints/numeric_constraints.h"
#include "src/contexts/librarian_context.h"
#include "src/librarian/errors.h"
#include "src/types/real.h"
#include "src/variables/minteger.h"

namespace moriarty {

MReal& MReal::AddConstraint(Exactly<double> constraint) {
  double value = constraint.GetValue();
  if (!numeric_one_of_.Mutable().ConstrainOptions(value)) {
    throw ImpossibleToSatisfy(ToString(), constraint.ToString());
  }

  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kBounds;
  constraints.bounds.AtLeast(value).AtMost(value);
  return InternalAddExactlyConstraint(std::move(constraint));
}

MReal& MReal::AddConstraint(Exactly<int64_t> constraint) {
  int64_t value = constraint.GetValue();
  if (!numeric_one_of_.Mutable().ConstrainOptions(value)) {
    throw ImpossibleToSatisfy(ToString(), constraint.ToString());
  }

  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kBounds;
  constraints.bounds.AtLeast(value).AtMost(value);
  return InternalAddConstraint(std::move(constraint));
}

MReal& MReal::AddConstraint(Exactly<Real> constraint) {
  Real value = constraint.GetValue();
  if (!numeric_one_of_.Mutable().ConstrainOptions(value)) {
    throw ImpossibleToSatisfy(ToString(), constraint.ToString());
  }

  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kBounds;
  constraints.bounds.AtLeast(value).AtMost(value);
  return AddConstraint(librarian::ExactlyNumeric(value));
}

MReal& MReal::AddConstraint(Exactly<std::string> constraint) {
  return AddConstraint(librarian::ExactlyNumeric(constraint.GetValue()));
}

MReal& MReal::AddConstraint(librarian::ExactlyNumeric constraint) {
  if (!numeric_one_of_.Mutable().ConstrainOptions(constraint)) {
    throw ImpossibleToSatisfy(ToString(), constraint.ToString());
  }
  return InternalAddConstraint(std::move(constraint));
}

MReal& MReal::AddConstraint(OneOf<int64_t> constraint) {
  if (!numeric_one_of_.Mutable().ConstrainOptions(constraint.GetOptions())) {
    throw ImpossibleToSatisfy(ToString(), constraint.ToString());
  }
  return InternalAddConstraint(std::move(constraint));
}

MReal& MReal::AddConstraint(OneOf<Real> constraint) {
  return AddConstraint(librarian::OneOfNumeric(constraint.GetOptions()));
}

MReal& MReal::AddConstraint(OneOf<std::string> constraint) {
  return AddConstraint(librarian::OneOfNumeric(constraint.GetOptions()));
}

MReal& MReal::AddConstraint(librarian::OneOfNumeric constraint) {
  if (!numeric_one_of_.Mutable().ConstrainOptions(constraint)) {
    throw ImpossibleToSatisfy(ToString(), constraint.ToString());
  }
  return InternalAddConstraint(std::move(constraint));
}

MReal& MReal::AddConstraint(Between constraint) {
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kBounds;
  constraints.bounds.Intersect(constraint.GetRange());
  return InternalAddConstraint(std::move(constraint));
}

MReal& MReal::AddConstraint(AtMost constraint) {
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kBounds;
  constraints.bounds.Intersect(constraint.GetRange());
  return InternalAddConstraint(std::move(constraint));
}

MReal& MReal::AddConstraint(AtLeast constraint) {
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kBounds;
  constraints.bounds.Intersect(constraint.GetRange());
  return InternalAddConstraint(std::move(constraint));
}

MReal& MReal::AddConstraint(MRealFormat constraint) {
  format_ = constraint;
  return *this;
}

const Range& MReal::CoreConstraints::Bounds() const { return data_->bounds; }

double MReal::GenerateImpl(librarian::GenerateVariableContext ctx) const {
  if (GetOneOf().HasBeenConstrained()) {
    return GetOneOf().SelectOneOf([&](int n) { return ctx.RandomInteger(n); });
  }
  if (numeric_one_of_->HasBeenConstrained()) {
    return ctx
        .RandomElement(numeric_one_of_->GetOptionsLookup(
            [&](std::string_view var) -> int64_t {
              return ctx.GenerateVariable<MInteger>(var);
            }))
        .GetApproxValue();
  }

  std::optional<Range::ExtremeValues<Real>> extremes =
      core_constraints_.Bounds().RealExtremes(
          [&](std::string_view var) -> int64_t {
            return ctx.GenerateVariable<MInteger>(var);
          });
  if (!extremes) {
    throw GenerationError(
        ctx.GetVariableName(),
        std::format("No real number satisfies: {}", ToString()),
        RetryPolicy::kAbort);
  }

  return ctx.RandomReal(extremes->min, extremes->max);
}

double MReal::ReadImpl(librarian::ReadVariableContext ctx) const {
  return ctx.ReadReal(Format().GetDigits());
}

void MReal::WriteImpl(librarian::WriteVariableContext ctx,
                      const double& value) const {
  ctx.WriteToken(std::format("{:.{}f}", value, Format().GetDigits()));
}

std::optional<double> MReal::GetUniqueValueImpl(
    librarian::AnalyzeVariableContext ctx) const {
  if (auto dbl_one_of = GetOneOf().GetUniqueValue()) return *dbl_one_of;

  try {
    if (auto numeric_one_of = numeric_one_of_->GetUniqueValue(ctx)) {
      return numeric_one_of->GetApproxValue();
    }

    std::optional<Range::ExtremeValues<Real>> extremes =
        core_constraints_.Bounds().RealExtremes(
            [&](std::string_view var) -> int64_t {
              auto value = ctx.GetUniqueValue<MInteger>(var);
              if (!value) throw ValueNotFound(var);
              return *value;
            });
    if (!extremes || extremes->min != extremes->max) return std::nullopt;
    return extremes->min.GetApproxValue();
  } catch (const ValueNotFound& e) {
    // There might be a unique-value, but we can't evaluate it.
    return std::nullopt;
  }

  return std::nullopt;
}

MRealFormat& MReal::Format() { return format_; }

MRealFormat MReal::Format() const { return format_; }

MRealFormat& MRealFormat::Digits(int num_digits) {
  if (!(1 <= num_digits && num_digits <= 20)) {
    throw InvalidConstraint(
        "MRealFormat::Digits",
        std::format("num_digits must be between 1 and 20 (got: {}).",
                    num_digits));
  }
  digits_ = num_digits;
  return *this;
}

int MRealFormat::GetDigits() const { return digits_; }

}  // namespace moriarty
