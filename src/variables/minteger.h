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
#include "src/librarian/size_property.h"
#include "src/property.h"
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

  // The integer must be exactly this value.
  MInteger& AddConstraint(const Exactly<int64_t>& constraint);
  // The integer must be exactly this integer expression (e.g., "3 * N + 1").
  MInteger& AddConstraint(const ExactlyIntegerExpression& constraint);
  // The integer must be exactly this integer expression (e.g., "3 * N + 1").
  MInteger& AddConstraint(const Exactly<std::string>& constraint);
  // The integer must be in this inclusive range.
  MInteger& AddConstraint(const Between& constraint);
  // The integer must be this value or smaller.
  MInteger& AddConstraint(const AtMost& constraint);
  // The integer must be this value or larger.
  MInteger& AddConstraint(const AtLeast& constraint);
  // The integer should be approximately this size.
  MInteger& AddConstraint(const SizeCategory& constraint);

  [[nodiscard]] std::string Typename() const override { return "MInteger"; }

  // OfSizeProperty()
  //
  // Tells this int to have a specific size. `property.category` must be
  // "size". The exact values here are not guaranteed and may change over
  // time. If exact values are required, specify them manually.
  absl::Status OfSizeProperty(Property property);

 private:
  librarian::CowPtr<Range> bounds_;

  // What approximate size should the int64_t be when it is generated.
  CommonSize approx_size_ = CommonSize::kAny;

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
        std::unique_ptr<IntegerRangeMConstraint> constraint)
        : constraint_(std::move(constraint)) {};

    bool IsSatisfiedWith(librarian::AnalysisContext ctx, int64_t value) const {
      return constraint_->IsSatisfiedWith(Wrap(ctx), value);
    }
    std::string Explanation(librarian::AnalysisContext ctx,
                            int64_t value) const {
      return constraint_->Explanation(Wrap(ctx), value);
    }
    std::string ToString() const { return constraint_->ToString(); }

   private:
    std::unique_ptr<IntegerRangeMConstraint> constraint_;

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
  absl::StatusOr<std::vector<MInteger>> GetDifficultInstancesImpl(
      librarian::AnalysisContext ctx) const override;
  std::optional<int64_t> GetUniqueValueImpl(
      librarian::AnalysisContext ctx) const override;
  std::string ToStringImpl() const override;
  absl::StatusOr<std::string> ValueToStringImpl(
      const int64_t& value) const override;
  // ---------------------------------------------------------------------------
};

// -----------------------------------------------------------------------------
//  Implementation details
// -----------------------------------------------------------------------------

template <typename... Constraints>
  requires(std::derived_from<std::decay_t<Constraints>, MConstraint> && ...)
MInteger::MInteger(Constraints&&... constraints) {
  RegisterKnownProperty("size", &MInteger::OfSizeProperty);
  (AddConstraint(std::forward<Constraints>(constraints)), ...);
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_MINTEGER_H_
