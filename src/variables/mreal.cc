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
#include <stdexcept>

#include "src/contexts/librarian_context.h"
#include "src/librarian/errors.h"
#include "src/types/real.h"
#include "src/variables/minteger.h"

namespace moriarty {

MReal& MReal::AddConstraint(Exactly<double> constraint) {
  double value = constraint.GetValue();
  core_constraints_.data_.Mutable().bounds.AtLeast(value).AtMost(value);
  return InternalAddExactlyConstraint(std::move(constraint));
}

MReal& MReal::AddConstraint(Exactly<int64_t> constraint) {
  if (!numeric_one_of_.Mutable().ConstrainOptions(constraint.GetValue())) {
    throw ImpossibleToSatisfy(ToString(), constraint.ToString());
  }
  auto actual_constraint =
      std::make_unique<librarian::ExactlyNumeric>(constraint.GetValue());
  core_constraints_.data_.Mutable().bounds.Intersect(
      actual_constraint->GetRange());
  return InternalAddConstraint(RangeConstraint(
      std::move(actual_constraint),
      [constraint](MReal& other) { other.AddConstraint(constraint); }));
}

MReal& MReal::AddConstraint(Exactly<Real> constraint) {
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kBounds;
  if (!numeric_one_of_.Mutable().ConstrainOptions(constraint.GetValue())) {
    throw ImpossibleToSatisfy(ToString(), constraint.ToString());
  }
  auto actual_constraint =
      std::make_unique<librarian::ExactlyNumeric>(constraint.GetValue());
  core_constraints_.data_.Mutable().bounds.Intersect(
      actual_constraint->GetRange());
  return InternalAddConstraint(RangeConstraint(
      std::move(actual_constraint),
      [constraint](MReal& other) { other.AddConstraint(constraint); }));
}

MReal& MReal::AddConstraint(Exactly<std::string> constraint) {
  if (!numeric_one_of_.Mutable().ConstrainOptions(constraint.GetValue())) {
    throw ImpossibleToSatisfy(ToString(), constraint.ToString());
  }
  auto actual_constraint =
      std::make_unique<librarian::ExactlyNumeric>(constraint.GetValue());
  core_constraints_.data_.Mutable().bounds.Intersect(
      actual_constraint->GetRange());
  return InternalAddConstraint(RangeConstraint(
      std::move(actual_constraint),
      [constraint](MReal& other) { other.AddConstraint(constraint); }));
}

MReal& MReal::AddConstraint(OneOf<int64_t> constraint) {
  auto actual_constraint =
      std::make_unique<librarian::OneOfNumeric>(constraint.GetOptions());
  if (!numeric_one_of_.Mutable().ConstrainOptions(*actual_constraint)) {
    throw ImpossibleToSatisfy(ToString(), constraint.ToString());
  }
  return InternalAddConstraint(RangeConstraint(
      std::move(actual_constraint),
      [constraint](MReal& other) { other.AddConstraint(constraint); }));
}

MReal& MReal::AddConstraint(OneOf<Real> constraint) {
  auto actual_constraint =
      std::make_unique<librarian::OneOfNumeric>(constraint.GetOptions());
  if (!numeric_one_of_.Mutable().ConstrainOptions(*actual_constraint)) {
    throw ImpossibleToSatisfy(ToString(), constraint.ToString());
  }
  return InternalAddConstraint(RangeConstraint(
      std::move(actual_constraint),
      [constraint](MReal& other) { other.AddConstraint(constraint); }));
}

MReal& MReal::AddConstraint(OneOf<std::string> constraint) {
  auto actual_constraint =
      std::make_unique<librarian::OneOfNumeric>(constraint.GetOptions());
  if (!numeric_one_of_.Mutable().ConstrainOptions(*actual_constraint)) {
    throw ImpossibleToSatisfy(ToString(), constraint.ToString());
  }
  return InternalAddConstraint(RangeConstraint(
      std::move(actual_constraint),
      [constraint](MReal& other) { other.AddConstraint(constraint); }));
}

MReal& MReal::AddConstraint(Between constraint) {
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kBounds;
  constraints.bounds.Intersect(constraint.GetRange());
  return InternalAddConstraint(RangeConstraint(
      std::make_unique<Between>(constraint),
      [constraint](MReal& other) { other.AddConstraint(constraint); }));
}

MReal& MReal::AddConstraint(AtMost constraint) {
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kBounds;
  constraints.bounds.Intersect(constraint.GetRange());
  return InternalAddConstraint(RangeConstraint(
      std::make_unique<AtMost>(constraint),
      [constraint](MReal& other) { other.AddConstraint(constraint); }));
}

MReal& MReal::AddConstraint(AtLeast constraint) {
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kBounds;
  constraints.bounds.Intersect(constraint.GetRange());
  return InternalAddConstraint(RangeConstraint(
      std::make_unique<AtLeast>(constraint),
      [constraint](MReal& other) { other.AddConstraint(constraint); }));
}

MReal& MReal::AddConstraint(MRealFormat constraint) {
  if (!(1 <= constraint.GetDigits() && constraint.GetDigits() <= 20)) {
    throw std::invalid_argument("num_digits must be between 1 and 20.");
  }
  format_ = constraint;
  return *this;
}

const Range& MReal::CoreConstraints::Bounds() const { return data_->bounds; }

double MReal::GenerateImpl(librarian::ResolverContext ctx) const {
  if (GetOneOf().HasBeenConstrained()) {
    return GetOneOf().SelectOneOf([&](int n) { return ctx.RandomInteger(n); });
  }
  if (numeric_one_of_->HasBeenConstrained()) {
    return ctx
        .RandomElement(
            numeric_one_of_->GetOptions([&](std::string_view var) -> int64_t {
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

double MReal::ReadImpl(librarian::ReaderContext ctx) const {
  return ctx.ReadReal(Format().GetDigits());
}

void MReal::PrintImpl(librarian::PrinterContext ctx,
                      const double& value) const {
  ctx.PrintToken(std::format("{:.{}f}", value, Format().GetDigits()));
}

std::optional<double> MReal::GetUniqueValueImpl(
    librarian::AnalysisContext ctx) const {
  if (auto dbl_one_of = GetOneOf().GetUniqueValue()) return *dbl_one_of;

  try {
    if (auto numeric_one_of =
            numeric_one_of_->GetUniqueValue([&](std::string_view var) {
              auto value = ctx.GetUniqueValue<MInteger>(var);
              if (!value) throw ValueNotFound(var);
              return *value;
            })) {
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
    throw InvalidConstraint("MRealFormat::Digits must be between 1 and 20.");
  }
  digits_ = num_digits;
  return *this;
}

int MRealFormat::GetDigits() const { return digits_; }

namespace {

NumericRangeMConstraint::LookupVariableFn Wrap(librarian::AnalysisContext ctx) {
  return [=](std::string_view variable) -> int64_t {
    auto value = ctx.GetUniqueValue<MInteger>(variable);
    if (!value) throw ValueNotFound(variable);
    return *value;
  };
}

}  // namespace

MReal::RangeConstraint::RangeConstraint(
    std::unique_ptr<NumericRangeMConstraint> constraint,
    std::function<void(MReal&)> apply_to_fn)
    : constraint_(std::move(constraint)),
      apply_to_fn_(std::move(apply_to_fn)) {};

ConstraintViolation MReal::RangeConstraint::CheckValue(
    librarian::AnalysisContext ctx, double value) const {
  return constraint_->CheckRealValue(Wrap(ctx), value);
}

std::string MReal::RangeConstraint::ToString() const {
  return constraint_->ToString();
}

std::vector<std::string> MReal::RangeConstraint::GetDependencies() const {
  return constraint_->GetDependencies();
}

void MReal::RangeConstraint::ApplyTo(MReal& other) const {
  apply_to_fn_(other);
}

}  // namespace moriarty
