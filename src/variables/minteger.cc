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

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/substitute.h"
#include "src/contexts/librarian/analysis_context.h"
#include "src/contexts/librarian/printer_context.h"
#include "src/contexts/librarian/reader_context.h"
#include "src/contexts/librarian/resolver_context.h"
#include "src/errors.h"
#include "src/internal/expressions.h"
#include "src/internal/range.h"
#include "src/librarian/one_of_handler.h"
#include "src/librarian/size_property.h"
#include "src/util/status_macro/status_macros.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/constraints/numeric_constraints.h"
#include "src/variables/constraints/size_constraints.h"

namespace moriarty {

MInteger& MInteger::AddConstraint(Exactly<int64_t> constraint) {
  bounds_.Mutable().Intersect(
      Range(constraint.GetValue(), constraint.GetValue()));
  InternalAddConstraint(std::move(constraint));
  return *this;
}

MInteger& MInteger::AddConstraint(ExactlyIntegerExpression constraint) {
  bounds_.Mutable().Intersect(constraint.GetRange());
  InternalAddConstraint(RangeConstraint(
      std::make_unique<ExactlyIntegerExpression>(constraint),
      [constraint](MInteger& other) { other.AddConstraint(constraint); }));
  return *this;
}

MInteger& MInteger::AddConstraint(Exactly<std::string> constraint) {
  return AddConstraint(ExactlyIntegerExpression(constraint.GetValue()));
}

MInteger& MInteger::AddConstraint(OneOf<int64_t> constraint) {
  one_of_int_.Mutable().ConstrainOptions(constraint.GetOptions());
  InternalAddConstraint(std::move(constraint));
  return *this;
}

MInteger& MInteger::AddConstraint(OneOfIntegerExpression constraint) {
  one_of_expr_.Mutable().ConstrainOptions(constraint.GetOptions());
  InternalAddConstraint(RangeConstraint(
      std::make_unique<OneOfIntegerExpression>(std::move(constraint)),
      [constraint](MInteger& other) { other.AddConstraint(constraint); }));
  return *this;
}

MInteger& MInteger::AddConstraint(OneOf<std::string> constraint) {
  return AddConstraint(OneOfIntegerExpression(constraint.GetOptions()));
}

MInteger& MInteger::AddConstraint(Between constraint) {
  bounds_.Mutable().Intersect(constraint.GetRange());
  InternalAddConstraint(RangeConstraint(
      std::make_unique<Between>(constraint),
      [constraint](MInteger& other) { other.AddConstraint(constraint); }));
  return *this;
}

MInteger& MInteger::AddConstraint(AtMost constraint) {
  bounds_.Mutable().Intersect(constraint.GetRange());
  InternalAddConstraint(RangeConstraint(
      std::make_unique<AtMost>(constraint),
      [constraint](MInteger& other) { other.AddConstraint(constraint); }));
  return *this;
}

MInteger& MInteger::AddConstraint(AtLeast constraint) {
  bounds_.Mutable().Intersect(constraint.GetRange());
  InternalAddConstraint(RangeConstraint(
      std::make_unique<AtLeast>(constraint),
      [constraint](MInteger& other) { other.AddConstraint(constraint); }));
  return *this;
}

MInteger& MInteger::AddConstraint(SizeCategory constraint) {
  size_handler_.Mutable().ConstrainSize(constraint.GetCommonSize());
  InternalAddConstraint(std::move(constraint));
  return *this;
}

std::optional<int64_t> MInteger::GetUniqueValueImpl(
    librarian::AnalysisContext ctx) const {
  auto option1 = one_of_int_->GetUniqueValue();
  if (option1) return *option1;

  auto option2 = one_of_expr_->GetUniqueValue();
  if (option2) {
    auto expr = *ParseExpression(*option2);
    try {
      return EvaluateIntegerExpression(expr, [&](std::string_view var) {
        auto value = ctx.GetUniqueValue<MInteger>(var);
        if (!value) throw ValueNotFound(var);
        return *value;
      });
    } catch (const ValueNotFound& e) {
      // There is a unique-value, but we can't evaluate it.
      return std::nullopt;
    }
  }

  absl::StatusOr<Range::ExtremeValues> extremes = GetExtremeValues(ctx);
  if (!extremes.ok()) return std::nullopt;
  if (extremes->min != extremes->max) return std::nullopt;

  return extremes->min;
}

absl::StatusOr<Range::ExtremeValues> MInteger::GetExtremeValues(
    librarian::ResolverContext ctx) const {
  MORIARTY_ASSIGN_OR_RETURN(
      absl::flat_hash_set<std::string> needed_dependent_variables,
      bounds_->NeededVariables(), _ << "Error getting the needed variables");

  absl::flat_hash_map<std::string, int64_t> dependent_variables;

  for (std::string_view name : needed_dependent_variables) {
    dependent_variables[name] = ctx.GenerateVariable<MInteger>(name);
  }

  MORIARTY_ASSIGN_OR_RETURN(std::optional<Range::ExtremeValues> extremes,
                            bounds_->Extremes(dependent_variables));
  if (!extremes) return absl::InvalidArgumentError("Valid range is empty");
  return *extremes;
}

absl::StatusOr<Range::ExtremeValues> MInteger::GetExtremeValues(
    librarian::AnalysisContext ctx) const {
  MORIARTY_ASSIGN_OR_RETURN(
      absl::flat_hash_set<std::string> needed_dependent_variables,
      bounds_->NeededVariables(), _ << "Error getting the needed variables");

  absl::flat_hash_map<std::string, int64_t> dependent_variables;

  for (std::string_view name : needed_dependent_variables) {
    std::optional<int64_t> value = ctx.GetUniqueValue<MInteger>(name);
    if (!value) {
      return absl::InvalidArgumentError(
          absl::Substitute("Unknown dependent variable: $0", name));
    }
    dependent_variables[name] = std::move(*value);
  }

  MORIARTY_ASSIGN_OR_RETURN(std::optional<Range::ExtremeValues> extremes,
                            bounds_->Extremes(dependent_variables));
  if (!extremes) return absl::InvalidArgumentError("Valid range is empty");
  return *extremes;
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

  if (exprs.HasBeenConstrained()) {
    std::vector<int64_t> options;
    for (const std::string& option : exprs.GetOptions()) {
      Expression expr = *ParseExpression(option);
      options.push_back(
          EvaluateIntegerExpression(expr, [&](std::string_view var) {
            return ctx.GenerateVariable<MInteger>(var);
          }));
    }
  }

  return options;
}

}  // namespace

int64_t MInteger::GenerateImpl(librarian::ResolverContext ctx) const {
  auto extremes = GetExtremeValues(ctx);
  if (!extremes.ok()) {
    throw std::runtime_error(std::string(extremes.status().message()));
  }

  if (one_of_int_->HasBeenConstrained() || one_of_expr_->HasBeenConstrained()) {
    std::vector<int64_t> options =
        ExtractOneOfOptions(ctx, *one_of_int_, *one_of_expr_);
    std::erase_if(options, [&](int64_t value) {
      return !(extremes->min <= value && value <= extremes->max);
    });
    return ctx.RandomElement(options);
  }

  if (size_handler_->GetConstrainedSize() == CommonSize::kAny)
    return ctx.RandomInteger(extremes->min, extremes->max);

  // TODO(darcybest): Make this work for larger ranges.
  if ((extremes->min <= std::numeric_limits<int64_t>::min() / 2) &&
      (extremes->max >= std::numeric_limits<int64_t>::max() / 2)) {
    return ctx.RandomInteger(extremes->min, extremes->max);
  }

  // Note: `max - min + 1` does not overflow because of the check above.
  Range range = librarian::GetRange(size_handler_->GetConstrainedSize(),
                                    extremes->max - extremes->min + 1);

  absl::StatusOr<std::optional<Range::ExtremeValues>> rng_extremes =
      range.Extremes();
  if (!rng_extremes.ok()) {
    throw std::runtime_error(std::string(rng_extremes.status().message()));
  }

  // If a special size has been requested, attempt to generate that. If that
  // fails, generate the full range.
  if (*rng_extremes) {
    // Offset the values appropriately. These ranges were supposed to be for
    // [1, N].
    (*rng_extremes)->min += extremes->min - 1;
    (*rng_extremes)->max += extremes->min - 1;

    try {
      return ctx.RandomInteger((*rng_extremes)->min, (*rng_extremes)->max);
    } catch (const std::exception& e) {
      // If the special size fails, simply pass through.
    }
  }

  return ctx.RandomInteger(extremes->min, extremes->max);
}

int64_t MInteger::ReadImpl(librarian::ReaderContext ctx) const {
  std::string token = ctx.ReadToken();
  std::stringstream is(token);

  int64_t value;
  if (!(is >> value)) throw std::runtime_error("Unable to read an integer");
  std::string garbage;
  if (is >> garbage)
    throw std::runtime_error("Token contains non-integer items");
  return value;
}

void MInteger::PrintImpl(librarian::PrinterContext ctx,
                         const int64_t& value) const {
  ctx.PrintToken(std::to_string(value));
}

std::vector<MInteger> MInteger::ListEdgeCasesImpl(
    librarian::AnalysisContext ctx) const {
  absl::StatusOr<Range::ExtremeValues> extremes = GetExtremeValues(ctx);

  int64_t min =
      extremes.ok() ? extremes->min : std::numeric_limits<int64_t>::min();
  int64_t max =
      extremes.ok() ? extremes->max : std::numeric_limits<int64_t>::max();

  // TODO(hivini): Create more interesting cases instead of a list of ints.
  std::vector<int64_t> values = {min};
  if (min != max) values.push_back(max);

  // Takes all elements in `insert_list` and inserts the ones between `min`
  // and `max` into `values`.

  auto insert_into_values = [&](const std::vector<int64_t>& insert_list) {
    for (int64_t v : insert_list) {
      // min and max are already in, only include values strictly in (min,
      // max).
      if (min < v && v < max && absl::c_find(values, v) == values.end())
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
    instances.push_back(MInteger().Is(v));
  }

  return instances;
}

std::vector<std::string> MInteger::GetDependenciesImpl() const {
  absl::StatusOr<absl::flat_hash_set<std::string>> needed =
      bounds_->NeededVariables();
  if (!needed.ok()) return {};
  return std::vector<std::string>(needed->begin(), needed->end());
}

}  // namespace moriarty
