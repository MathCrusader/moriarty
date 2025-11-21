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

#include "src/variables/minteger.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "src/constraints/base_constraints.h"
#include "src/constraints/constraint_violation.h"
#include "src/constraints/numeric_constraints.h"
#include "src/constraints/size_constraints.h"
#include "src/contexts/librarian_context.h"
#include "src/internal/range.h"
#include "src/librarian/errors.h"
#include "src/librarian/policies.h"
#include "src/librarian/size_property.h"

namespace moriarty {

MInteger& MInteger::AddConstraint(Exactly<int64_t> constraint) {
  if (!numeric_one_of_.Mutable().ConstrainOptions(constraint.GetValue())) {
    throw ImpossibleToSatisfy(ToString(), constraint.ToString());
  }
  core_constraints_.data_.Mutable()
      .bounds.AtLeast(constraint.GetValue())
      .AtMost(constraint.GetValue());
  return InternalAddExactlyConstraint(std::move(constraint));
}

MInteger& MInteger::AddConstraint(Exactly<std::string> constraint) {
  if (!numeric_one_of_.Mutable().ConstrainOptions(constraint.GetValue())) {
    throw ImpossibleToSatisfy(ToString(), constraint.ToString());
  }
  auto actual_constraint =
      std::make_unique<librarian::ExactlyNumeric>(constraint.GetValue());
  core_constraints_.data_.Mutable().bounds.Intersect(
      actual_constraint->GetRange());
  return InternalAddConstraint(RangeConstraint(
      std::move(actual_constraint),
      [constraint](MInteger& other) { other.AddConstraint(constraint); }));
}

MInteger& MInteger::AddConstraint(OneOf<int64_t> constraint) {
  librarian::OneOfNumeric actual_constraint(constraint.GetOptions());
  if (!numeric_one_of_.Mutable().ConstrainOptions(
          std::move(actual_constraint))) {
    throw ImpossibleToSatisfy(ToString(), constraint.ToString());
  }
  return InternalAddOneOfConstraint(std::move(constraint));
}

MInteger& MInteger::AddConstraint(OneOf<std::string> constraint) {
  auto actual_constraint =
      std::make_unique<librarian::OneOfNumeric>(constraint.GetOptions());
  if (!numeric_one_of_.Mutable().ConstrainOptions(*actual_constraint)) {
    throw ImpossibleToSatisfy(ToString(), constraint.ToString());
  }
  return InternalAddConstraint(RangeConstraint(
      std::move(actual_constraint),
      [constraint](MInteger& other) { other.AddConstraint(constraint); }));
}

MInteger& MInteger::AddConstraint(Between constraint) {
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kBounds;
  constraints.bounds.Intersect(constraint.GetRange());
  return InternalAddConstraint(RangeConstraint(
      std::make_unique<Between>(constraint),
      [constraint](MInteger& other) { other.AddConstraint(constraint); }));
}

MInteger& MInteger::AddConstraint(AtMost constraint) {
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kBounds;
  constraints.bounds.Intersect(constraint.GetRange());
  return InternalAddConstraint(RangeConstraint(
      std::make_unique<AtMost>(constraint),
      [constraint](MInteger& other) { other.AddConstraint(constraint); }));
}

MInteger& MInteger::AddConstraint(AtLeast constraint) {
  auto& constraints = core_constraints_.data_.Mutable();
  constraints.touched |= CoreConstraints::Flags::kBounds;
  constraints.bounds.Intersect(constraint.GetRange());
  return InternalAddConstraint(RangeConstraint(
      std::make_unique<AtLeast>(constraint),
      [constraint](MInteger& other) { other.AddConstraint(constraint); }));
}

MInteger& MInteger::AddConstraint(SizeCategory constraint) {
  size_handler_.Mutable().ConstrainSize(constraint.GetCommonSize());
  return InternalAddConstraint(std::move(constraint));
}

std::optional<int64_t> MInteger::GetUniqueValueImpl(
    librarian::AnalysisContext ctx) const {
  try {
    if (auto one_of =
            numeric_one_of_->GetUniqueValue([&](std::string_view var) {
              auto value = ctx.GetUniqueValue<MInteger>(var);
              if (!value) throw ValueNotFound(var);
              return *value;
            })) {
      auto value = one_of->GetValue();
      if (value.denominator != 1) return std::nullopt;
      return value.numerator;
    }
  } catch (const ValueNotFound& e) {
    // There might be a unique-value, but we can't evaluate it.
    return std::nullopt;
  }

  std::optional<Range::ExtremeValues<int64_t>> extremes = GetExtremeValues(ctx);
  if (!extremes || extremes->min != extremes->max) return std::nullopt;

  return extremes->min;
}

Range::ExtremeValues<int64_t> MInteger::GetExtremeValues(
    librarian::ResolverContext ctx) const {
  std::optional<Range::ExtremeValues<int64_t>> extremes =
      core_constraints_.Bounds().IntegerExtremes([&](std::string_view var) {
        return ctx.GenerateVariable<MInteger>(var);
      });
  if (!extremes) {
    throw GenerationError(ctx.GetVariableName(),
                          std::format("No integer satisfies: {}", ToString()),
                          RetryPolicy::kAbort);
  }
  return *extremes;
}

std::optional<Range::ExtremeValues<int64_t>> MInteger::GetExtremeValues(
    librarian::AnalysisContext ctx) const {
  try {
    std::optional<Range::ExtremeValues<int64_t>> extremes =
        core_constraints_.Bounds().IntegerExtremes([&](std::string_view var) {
          auto value = ctx.GetUniqueValue<MInteger>(var);
          if (!value) throw ValueNotFound(var);
          return *value;
        });
    return extremes;
  } catch (const ValueNotFound& e) {
    return std::nullopt;
  }
}

int64_t MInteger::GenerateImpl(librarian::ResolverContext ctx) const {
  Range::ExtremeValues<int64_t> extremes = GetExtremeValues(ctx);

  if (numeric_one_of_->HasBeenConstrained()) {
    std::vector<Real> real_options =
        numeric_one_of_->GetOptions([&](std::string_view var) {
          return ctx.GenerateVariable<MInteger>(var);
        });
    std::vector<int64_t> options;
    for (const Real& value : real_options) {
      if (value.GetValue().denominator != 1) continue;
      int64_t int_value = value.GetValue().numerator;
      if (extremes.min <= int_value && int_value <= extremes.max) {
        options.push_back(int_value);
      }
    }
    if (options.empty()) {
      throw GenerationError(ctx.GetVariableName(), ToString(),
                            RetryPolicy::kAbort);
    }
    return ctx.RandomElement(options);
  }

  if (size_handler_->GetConstrainedSize() == CommonSize::kAny)
    return ctx.RandomInteger(extremes.min, extremes.max);

  // TODO(darcybest): Make this work for larger ranges.
  if ((extremes.min <= std::numeric_limits<int64_t>::min() / 2) &&
      (extremes.max >= std::numeric_limits<int64_t>::max() / 2)) {
    return ctx.RandomInteger(extremes.min, extremes.max);
  }

  // Note: `max - min + 1` does not overflow because of the check above.
  Range range = librarian::GetRange(size_handler_->GetConstrainedSize(),
                                    extremes.max - extremes.min + 1);

  std::optional<Range::ExtremeValues<int64_t>> rng_extremes =
      range.IntegerExtremes([&](std::string_view var) -> int64_t {
        return ctx.GenerateVariable<MInteger>(var);
      });

  // If a special size has been requested, attempt to generate that. If that
  // fails, generate the full range.
  if (rng_extremes) {
    // Offset the values appropriately. These ranges were supposed to be for
    // [1, N].
    rng_extremes->min += extremes.min - 1;
    rng_extremes->max += extremes.min - 1;
    return ctx.RandomInteger(rng_extremes->min, rng_extremes->max);
  }

  return ctx.RandomInteger(extremes.min, extremes.max);
}

int64_t MInteger::ReadImpl(librarian::ReaderContext ctx) const {
  return ctx.ReadInteger();
}

void MInteger::PrintImpl(librarian::PrinterContext ctx,
                         const int64_t& value) const {
  ctx.PrintToken(std::to_string(value));
}

std::vector<MInteger> MInteger::ListEdgeCasesImpl(
    librarian::AnalysisContext ctx) const {
  std::optional<Range::ExtremeValues<int64_t>> extremes = GetExtremeValues(ctx);

  int64_t min = extremes ? extremes->min : std::numeric_limits<int64_t>::min();
  int64_t max = extremes ? extremes->max : std::numeric_limits<int64_t>::max();

  // TODO(hivini): Create more interesting cases instead of a list of ints.
  std::vector<int64_t> values = {min};
  if (min != max) values.push_back(max);

  // Takes all elements in `insert_list` and inserts the ones between `min`
  // and `max` into `values`.

  auto insert_into_values = [&](const std::vector<int64_t>& insert_list) {
    for (int64_t v : insert_list) {
      // min and max are already in, only include values strictly in (min,
      // max).
      if (min < v && v < max && std::ranges::find(values, v) == values.end())
        values.push_back(v);
    }
  };

  // Small values
  insert_into_values({0, 1, 2, -1, -2});

  // Near powers of 2 (2^63 will be handled with min/max if applicable)
  for (int exp : {7, 8, 15, 16, 31, 32, 62}) {
    int64_t pow_two = 1LL << exp;
    insert_into_values({pow_two, pow_two + 1, pow_two - 1});
    insert_into_values({-pow_two, -pow_two + 1, -pow_two - 1});
  }

  // Relative to min/max
  insert_into_values({min / 2, max / 2, min + 1, max - 1});
  if (max >= 0) {
    int64_t square_root = std::sqrt(max);
    insert_into_values({square_root, square_root + 1, square_root - 1});
  }

  std::vector<MInteger> instances;
  instances.reserve(values.size());
  for (const auto& v : values) {
    instances.push_back(MInteger(Exactly(v)));
  }

  return instances;
}

namespace {

NumericRangeMConstraint::LookupVariableFn Wrap(librarian::AnalysisContext ctx) {
  return [=](std::string_view variable) -> int64_t {
    auto value = ctx.GetUniqueValue<MInteger>(variable);
    if (!value) throw ValueNotFound(variable);
    return *value;
  };
}

}  // namespace

MInteger::RangeConstraint::RangeConstraint(
    std::unique_ptr<NumericRangeMConstraint> constraint,
    std::function<void(MInteger&)> apply_to_fn)
    : constraint_(std::move(constraint)),
      apply_to_fn_(std::move(apply_to_fn)) {};

ConstraintViolation MInteger::RangeConstraint::CheckValue(
    librarian::AnalysisContext ctx, int64_t value) const {
  return constraint_->CheckIntegerValue(Wrap(ctx), value);
}

std::string MInteger::RangeConstraint::ToString() const {
  return constraint_->ToString();
}

std::vector<std::string> MInteger::RangeConstraint::GetDependencies() const {
  return constraint_->GetDependencies();
}

void MInteger::RangeConstraint::ApplyTo(MInteger& other) const {
  apply_to_fn_(other);
}

const Range& MInteger::CoreConstraints::Bounds() const { return data_->bounds; }

bool MInteger::CoreConstraints::BoundsConstrained() const {
  return IsSet(Flags::kBounds);
}

bool MInteger::CoreConstraints::IsSet(Flags flag) const {
  return (data_->touched & static_cast<std::underlying_type_t<Flags>>(flag)) !=
         0;
}

}  // namespace moriarty
