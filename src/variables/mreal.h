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

#ifndef MORIARTY_VARIABLES_MREAL_H_
#define MORIARTY_VARIABLES_MREAL_H_

#include <sys/types.h>

#include <cstdint>

#include "src/constraints/numeric_constraints.h"
#include "src/internal/range.h"
#include "src/librarian/mvariable.h"
#include "src/librarian/size_property.h"
#include "src/librarian/util/cow_ptr.h"
#include "src/types/real.h"

namespace moriarty {

class MReal;
namespace librarian {
template <>
struct MVariableValueTypeTrait<MReal> {
  using type = double;
};
}  // namespace librarian

// MRealFormat
//
// How to format an MReal when reading/writing.
class MRealFormat {
 public:
  // Sets the number of digits after the decimal place.
  //
  // When writing: writes this many digits.
  //
  // When reading: if PrecisionStrictness is kPrecise, then Read ensures
  // there are exactly `num_digits` digits after the decimal point. Otherwise,
  // this value is ignored.
  //
  // Default: 6
  MRealFormat& Digits(int num_digits);
  // Returns the number of digits to use when writing this value.
  int GetDigits() const;

  // Take any non-defaults in `other` and apply them to this format.
  void Merge(const MRealFormat& other);

 private:
  int digits_ = 6;
};

// MReal
//
// Describes constraints placed on a real number.
//
// Note: We intentially do not support `long double`, since the precision of
// `long double` has different precision on various systems.
class MReal : public librarian::MVariable<MReal> {
 public:
  // Create an MReal from a set of constraints. Logically equivalent to
  // calling AddConstraint() for each constraint.
  //
  // E.g., MReal(Between(0, Real("5.28")), SizeCategory::Small())
  template <typename... Constraints>
    requires(ConstraintFor<MReal, Constraints> && ...)
  explicit MReal(Constraints&&... constraints);

  ~MReal() override = default;

  using MVariable<MReal>::AddConstraint;  // Custom constraints

  // ---------------------------------------------------------------------------
  //  Constrain the value to a specific set of values

  // The number must be exactly this value. Prefer to use the `Exactly<Real>()`
  // constraint instead of `Exactly<double>()`.
  MReal& AddConstraint(Exactly<double> constraint);
  // The number must be exactly this value.
  MReal& AddConstraint(Exactly<Real> constraint);
  // The number must be exactly this value.
  MReal& AddConstraint(Exactly<int64_t> constraint);
  // The number must be exactly this integer expression (e.g., "3 * N + 1").
  MReal& AddConstraint(Exactly<std::string> constraint);
  // The number must be exactly this value.
  MReal& AddConstraint(librarian::ExactlyNumeric constraint);

  // The number must be one of these values.
  MReal& AddConstraint(OneOf<Real> constraint);
  // The number must be one of these values.
  MReal& AddConstraint(OneOf<int64_t> constraint);
  // The number must be one of these integer expressions (e.g., "3 * N + 1").
  MReal& AddConstraint(OneOf<std::string> constraint);
  // The number must be one of these values.
  MReal& AddConstraint(librarian::OneOfNumeric constraint);

  // ---------------------------------------------------------------------------
  //  Constrain the interval of values that the number can be in

  // The number must be in this inclusive range.
  MReal& AddConstraint(Between constraint);
  // The number must be this value or smaller.
  MReal& AddConstraint(AtMost constraint);
  // The number must be this value or larger.
  MReal& AddConstraint(AtLeast constraint);

  // ---------------------------------------------------------------------------
  //  Pseudo-constraints on size

  // TODO: Add AddConstraint(SizeCategory constraint)

  // ---------------------------------------------------------------------------
  //  Input/Output style

  // Change the I/O format of the number.
  // Note: I/O constraints behave as overrides instead of merges.
  MReal& AddConstraint(MRealFormat constraint);
  // Returns the I/O format for this number.
  [[nodiscard]] MRealFormat& Format();
  // Returns the I/O format for this number.
  [[nodiscard]] MRealFormat Format() const;

  [[nodiscard]] std::string Typename() const override { return "MReal"; }

  // MReal::CoreConstraints
  //
  // A base set of constraints for `MReal` that are used during generation.
  // Note: Returned references are invalidated after any non-const call to this
  // class or the corresponding `MReal`.
  class CoreConstraints {
   public:
    bool BoundsConstrained() const;
    const Range& Bounds() const;

   private:
    friend class MReal;
    enum Flags : uint32_t {
      kBounds = 1 << 0,
    };
    struct Data {
      std::underlying_type_t<Flags> touched = 0;
      Range bounds;
    };
    librarian::CowPtr<Data> data_;
    bool IsSet(Flags flag) const;
  };
  [[nodiscard]] CoreConstraints GetCoreConstraints() const {
    return core_constraints_;
  }

 private:
  CoreConstraints core_constraints_;
  librarian::CowPtr<librarian::OneOfNumeric> numeric_one_of_;
  librarian::CowPtr<librarian::SizeHandler> size_handler_;
  MRealFormat format_;

  // ---------------------------------------------------------------------------
  //  MVariable overrides
  double GenerateImpl(librarian::GenerateVariableContext ctx) const override;
  double ReadImpl(librarian::ReadVariableContext ctx) const override;
  void WriteImpl(librarian::WriteVariableContext ctx,
                 const double& value) const override;
  std::optional<double> GetUniqueValueImpl(
      librarian::AnalyzeVariableContext ctx) const override;
  // ---------------------------------------------------------------------------
};

// -----------------------------------------------------------------------------
//  Implementation details
// -----------------------------------------------------------------------------

template <typename... Constraints>
  requires(ConstraintFor<MReal, Constraints> && ...)
MReal::MReal(Constraints&&... constraints) {
  (AddConstraint(std::forward<Constraints>(constraints)), ...);
}

}  // namespace moriarty

#endif  // MORIARTY_VARIABLES_MREAL_H_
