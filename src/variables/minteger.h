/*
 * Copyright 2025 Darcy Best
 * Copyright 2024 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MORIARTY_SRC_VARIABLES_MINTEGER_H_
#define MORIARTY_SRC_VARIABLES_MINTEGER_H_

#include <concepts>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "src/contexts/librarian/analysis_context.h"
#include "src/contexts/librarian/printer_context.h"
#include "src/contexts/librarian/reader_context.h"
#include "src/contexts/librarian/resolver_context.h"
#include "src/errors.h"
#include "src/internal/range.h"
#include "src/librarian/cow_ptr.h"
#include "src/librarian/mvariable.h"
#include "src/librarian/one_of_handler.h"
#include "src/librarian/size_property.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/constraints/numeric_constraints.h"
#include "src/variables/constraints/size_constraints.h"

namespace moriarty {

// MInteger
//
// Describes constraints placed on an integer. We mean a "mathematical" integer,
// not a "computer science" integer. As such, we intentionally do not have an
// MVariable for 32-bit integers.
class MInteger : public librarian::MVariable<MInteger, int64_t> {
 public:
  // Create an MInteger from a set of constraints. Logically equivalent to
  // calling AddConstraint() for each constraint.
  //
  // E.g., MInteger(Between(1, "3 * N + 1"), SizeCategory::Small())
  template <typename... Constraints>
    requires(std::derived_from<std::decay_t<Constraints>, MConstraint> && ...)
  explicit MInteger(Constraints&&... constraints);

  // ---------------------------------------------------------------------------
  //  Constrain the value to a specific set of values

  // The integer must be exactly this value.
  MInteger& AddConstraint(Exactly<int64_t> constraint);
  // The integer must be exactly this integer expression (e.g., "3 * N + 1").
  MInteger& AddConstraint(ExactlyIntegerExpression constraint);
  // The integer must be exactly this integer expression (e.g., "3 * N + 1").
  MInteger& AddConstraint(Exactly<std::string> constraint);

  // The integer must be one of these values.
  MInteger& AddConstraint(OneOf<int64_t> constraint);
  // The integer must be one of these integer expressions.
  MInteger& AddConstraint(OneOfIntegerExpression constraint);
  // The integer must be one of these integer expressions.
  MInteger& AddConstraint(OneOf<std::string> constraint);

  // ---------------------------------------------------------------------------
  //  Constraints about ranges

  // The integer must be in this inclusive range.
  MInteger& AddConstraint(Between constraint);
  // The integer must be this value or smaller.
  MInteger& AddConstraint(AtMost constraint);
  // The integer must be this value or larger.
  MInteger& AddConstraint(AtLeast constraint);

  // ---------------------------------------------------------------------------
  //  Pseudo-constraints on size

  // The integer should be approximately this size.
  MInteger& AddConstraint(SizeCategory constraint);

  [[nodiscard]] std::string Typename() const override { return "MInteger"; }

 private:
  librarian::CowPtr<Range> bounds_;
  librarian::CowPtr<librarian::OneOfHandler<int64_t>> one_of_int_;
  librarian::CowPtr<librarian::OneOfHandler<std::string>> one_of_expr_;
  librarian::CowPtr<librarian::SizeHandler> size_handler_;

  // Computes and returns the minimum and maximum of `bounds_`. Returns
  // `kInvalidArgumentError` if the range is empty. The `ResolverContext`
  // version may generate other dependent variables if needed along the way.
  absl::StatusOr<Range::ExtremeValues> GetExtremeValues(
      librarian::AnalysisContext ctx) const;
  absl::StatusOr<Range::ExtremeValues> GetExtremeValues(
      librarian::ResolverContext ctx) const;

  struct RangeConstraint {
   public:
    explicit RangeConstraint(
        std::unique_ptr<IntegerRangeMConstraint> constraint,
        std::function<void(MInteger&)> apply_to_fn)
        : constraint_(std::move(constraint)),
          apply_to_fn_(std::move(apply_to_fn)) {};

    bool IsSatisfiedWith(librarian::AnalysisContext ctx, int64_t value) const {
      return constraint_->IsSatisfiedWith(Wrap(ctx), value);
    }
    std::string Explanation(librarian::AnalysisContext ctx,
                            int64_t value) const {
      return constraint_->Explanation(Wrap(ctx), value);
    }
    std::string ToString() const { return constraint_->ToString(); }
    void ApplyTo(MInteger& other) const { apply_to_fn_(other); }

   private:
    std::unique_ptr<IntegerRangeMConstraint> constraint_;
    std::function<void(MInteger&)> apply_to_fn_;

    IntegerRangeMConstraint::LookupVariableFn Wrap(
        librarian::AnalysisContext ctx) const {
      return [=](std::string_view variable) -> int64_t {
        auto value = ctx.GetUniqueValue<MInteger>(variable);
        if (!value) throw ValueNotFound(variable);
        return *value;
      };
    }
  };

  // ---------------------------------------------------------------------------
  //  MVariable overrides
  int64_t GenerateImpl(librarian::ResolverContext ctx) const override;
  absl::Status IsSatisfiedWithImpl(librarian::AnalysisContext ctx,
                                   const int64_t& value) const override;
  absl::Status MergeFromImpl(const MInteger& other) override;
  int64_t ReadImpl(librarian::ReaderContext ctx) const override;
  void PrintImpl(librarian::PrinterContext ctx,
                 const int64_t& value) const override;
  std::vector<std::string> GetDependenciesImpl() const override;
  std::vector<MInteger> ListEdgeCasesImpl(
      librarian::AnalysisContext ctx) const override;
  std::optional<int64_t> GetUniqueValueImpl(
      librarian::AnalysisContext ctx) const override;
  // ---------------------------------------------------------------------------
};

// -----------------------------------------------------------------------------
//  Implementation details
// -----------------------------------------------------------------------------

template <typename... Constraints>
  requires(std::derived_from<std::decay_t<Constraints>, MConstraint> && ...)
MInteger::MInteger(Constraints&&... constraints) {
  (AddConstraint(std::forward<Constraints>(constraints)), ...);
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_MINTEGER_H_
