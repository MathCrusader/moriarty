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

#include "src/contexts/librarian_context.h"
#include "src/internal/expressions.h"
#include "src/internal/range.h"
#include "src/librarian/errors.h"
#include "src/librarian/one_of_handler.h"
#include "src/librarian/policies.h"
#include "src/librarian/size_property.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/constraints/numeric_constraints.h"
#include "src/variables/constraints/size_constraints.h"

namespace moriarty {

MInteger& MInteger::AddConstraint(Exactly<int64_t> constraint) {
  bounds_.Mutable().Intersect(
      Range(constraint.GetValue(), constraint.GetValue()));
  return InternalAddExactlyConstraint(std::move(constraint));
}

MInteger& MInteger::AddConstraint(ExactlyIntegerExpression constraint) {
  bounds_.Mutable().Intersect(constraint.GetRange());
  return InternalAddConstraint(RangeConstraint(
      std::make_unique<ExactlyIntegerExpression>(constraint),
      [constraint](MInteger& other) { other.AddConstraint(constraint); }));
}

MInteger& MInteger::AddConstraint(Exactly<std::string> constraint) {
  return AddConstraint(ExactlyIntegerExpression(constraint.GetValue()));
}

MInteger& MInteger::AddConstraint(OneOf<int64_t> constraint) {
  return InternalAddOneOfConstraint(std::move(constraint));
}

MInteger& MInteger::AddConstraint(OneOfIntegerExpression constraint) {
  if (!one_of_expr_.Mutable().ConstrainOptions(constraint.GetOptions()))
    throw ImpossibleToSatisfy(ToString(), constraint.ToString());
  return InternalAddConstraint(RangeConstraint(
      std::make_unique<OneOfIntegerExpression>(std::move(constraint)),
      [constraint](MInteger& other) { other.AddConstraint(constraint); }));
}

MInteger& MInteger::AddConstraint(OneOf<std::string> constraint) {
  return AddConstraint(OneOfIntegerExpression(constraint.GetOptions()));
}

MInteger& MInteger::AddConstraint(Between constraint) {
  bounds_.Mutable().Intersect(constraint.GetRange());
  return InternalAddConstraint(RangeConstraint(
      std::make_unique<Between>(constraint),
      [constraint](MInteger& other) { other.AddConstraint(constraint); }));
}

MInteger& MInteger::AddConstraint(AtMost constraint) {
  bounds_.Mutable().Intersect(constraint.GetRange());
  return InternalAddConstraint(RangeConstraint(
      std::make_unique<AtMost>(constraint),
      [constraint](MInteger& other) { other.AddConstraint(constraint); }));
}

MInteger& MInteger::AddConstraint(AtLeast constraint) {
  bounds_.Mutable().Intersect(constraint.GetRange());
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
  auto option1 = GetOneOf().GetUniqueValue();
  if (option1) return *option1;

  // FIXME: This is silly to recompile the expression every time.
  auto option2 = one_of_expr_->GetUniqueValue();
  if (option2) {
    Expression expr(*option2);
    try {
      return expr.Evaluate([&](std::string_view var) {
        auto value = ctx.GetUniqueValue<MInteger>(var);
        if (!value) throw ValueNotFound(var);
        return *value;
      });
    } catch (const ValueNotFound& e) {
      // There is a unique-value, but we can't evaluate it.
      return std::nullopt;
    }
  }

  std::optional<Range::ExtremeValues> extremes = GetExtremeValues(ctx);
  if (!extremes || extremes->min != extremes->max) return std::nullopt;

  return extremes->min;
}

Range::ExtremeValues MInteger::GetExtremeValues(
    librarian::ResolverContext ctx) const {
  std::optional<Range::ExtremeValues> extremes =
      bounds_->Extremes([&](std::string_view var) {
        return ctx.GenerateVariable<MInteger>(var);
      });
  if (!extremes) {
    throw GenerationError(ctx.GetVariableName(),
                          std::format("No integer satisfies: {}", ToString()),
                          RetryPolicy::kAbort);
  }
  return *extremes;
}

std::optional<Range::ExtremeValues> MInteger::GetExtremeValues(
    librarian::AnalysisContext ctx) const {
  try {
    std::optional<Range::ExtremeValues> extremes =
        bounds_->Extremes([&](std::string_view var) {
          auto value = ctx.GetUniqueValue<MInteger>(var);
          if (!value) throw ValueNotFound(var);
          return *value;
        });
    return extremes;
  } catch (const ValueNotFound& e) {
    return std::nullopt;
  }
}

namespace {

[[nodiscard]] std::vector<int64_t> ExtractOneOfOptions(
    librarian::ResolverContext ctx,
    const librarian::OneOfHandler<int64_t>& ints,
    const librarian::OneOfHandler<std::string>& exprs) {
  std::vector<int64_t> options;

  if (ints.HasBeenConstrained()) {
    for (int64_t option : ints.GetOptions()) {
      options.push_back(option);
    }
  }

  // FIXME: This is stilly to reparse the expression every time.
  if (exprs.HasBeenConstrained()) {
    std::vector<int64_t> options;
    for (const std::string& option : exprs.GetOptions()) {
      Expression expr(option);
      options.push_back(expr.Evaluate([&](std::string_view var) {
        return ctx.GenerateVariable<MInteger>(var);
      }));
    }
  }

  return options;
}

}  // namespace

int64_t MInteger::GenerateImpl(librarian::ResolverContext ctx) const {
  Range::ExtremeValues extremes = GetExtremeValues(ctx);

  if (GetOneOf().HasBeenConstrained() || one_of_expr_->HasBeenConstrained()) {
    std::vector<int64_t> options =
        ExtractOneOfOptions(ctx, GetOneOf(), *one_of_expr_);
    std::erase_if(options, [&](int64_t value) {
      return !(extremes.min <= value && value <= extremes.max);
    });
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

  std::optional<Range::ExtremeValues> rng_extremes =
      range.Extremes([&](std::string_view var) -> int64_t {
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
  std::optional<Range::ExtremeValues> extremes = GetExtremeValues(ctx);

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

IntegerRangeMConstraint::LookupVariableFn Wrap(librarian::AnalysisContext ctx) {
  return [=](std::string_view variable) -> int64_t {
    auto value = ctx.GetUniqueValue<MInteger>(variable);
    if (!value) throw ValueNotFound(variable);
    return *value;
  };
}

}  // namespace

MInteger::RangeConstraint::RangeConstraint(
    std::unique_ptr<IntegerRangeMConstraint> constraint,
    std::function<void(MInteger&)> apply_to_fn)
    : constraint_(std::move(constraint)),
      apply_to_fn_(std::move(apply_to_fn)) {};

bool MInteger::RangeConstraint::IsSatisfiedWith(librarian::AnalysisContext ctx,
                                                int64_t value) const {
  return constraint_->IsSatisfiedWith(Wrap(ctx), value);
}

std::string MInteger::RangeConstraint::UnsatisfiedReason(
    librarian::AnalysisContext ctx, int64_t value) const {
  return constraint_->UnsatisfiedReason(Wrap(ctx), value);
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

}  // namespace moriarty
