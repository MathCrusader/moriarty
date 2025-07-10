/*
 * Copyright 2025 Darcy Best
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

#ifndef MORIARTY_SRC_LIBRARIAN_CONSTRAINT_HANDLER_H_
#define MORIARTY_SRC_LIBRARIAN_CONSTRAINT_HANDLER_H_

#include <string>
#include <utility>
#include <vector>

#include "src/constraints/constraint_violation.h"
#include "src/contexts/librarian_context.h"
#include "src/librarian/util/cow_ptr.h"

namespace moriarty {
namespace librarian {

template <typename ConstraintType, typename ValueType>
concept ConstraintHasSimplifiedCheckValueFn =
    requires(const ConstraintType& c, const ValueType& v) {
      { c.CheckValue(v) } -> std::same_as<ConstraintViolation>;
    };

template <typename ConstraintType, typename VariableType>
concept ConstraintHasCustomApplyToFn =
    requires(const ConstraintType& c, VariableType& v) {
      { c.ApplyTo(v) } -> std::same_as<void>;
    };

// ConstraintHandler
//
// Holds a collection of type-erased constraints. Each constraint must have a
// particular API.
template <typename VariableType, typename ValueType>
class ConstraintHandler {
 public:
  explicit ConstraintHandler() = default;

  // AddConstraint()
  //
  // Adds a constraint to the system.
  template <typename ConstraintType>
  void AddConstraint(ConstraintType constraint);

  // CheckValue()
  //
  // Determines if all constraints are satisfied with the given value.
  ConstraintViolation CheckValue(AnalysisContext ctx,
                                 const ValueType& value) const;

  // ToString()
  //
  // Returns a string representation of the constraints.
  auto ToString() const -> std::string;

  // ApplyAllTo()
  //
  // Apply all constraints stored in this handler into `other`.
  void ApplyAllTo(VariableType& other) const;

 private:
  class ConstraintHusk {
   public:
    virtual ~ConstraintHusk() = default;
    virtual auto CheckValue(AnalysisContext ctx, const ValueType& value) const
        -> ConstraintViolation = 0;
    virtual auto ToString() const -> std::string = 0;
    virtual auto ApplyTo(VariableType& other) const -> void = 0;
  };

  template <typename U>
  class ConstraintWrapper : public ConstraintHusk {
   public:
    ~ConstraintWrapper() override = default;
    explicit ConstraintWrapper(U constraint)
        : constraint_(std::move(constraint)) {}
    auto CheckValue(AnalysisContext ctx, const ValueType& value) const
        -> ConstraintViolation override {
      if constexpr (ConstraintHasSimplifiedCheckValueFn<U, ValueType>) {
        return constraint_.CheckValue(value);
      } else {
        return constraint_.CheckValue(ctx, value);
      }
    }
    auto ToString() const -> std::string override {
      return constraint_.ToString();
    }
    auto ApplyTo(VariableType& other) const -> void override {
      if constexpr (ConstraintHasCustomApplyToFn<U, VariableType>) {
        constraint_.ApplyTo(other);
      } else {
        other.AddConstraint(constraint_);
      }
    }

   private:
    U constraint_;
  };

  std::vector<CowPtr<ConstraintHusk>> constraints_;
};

// -----------------------------------------------------------------------------
//  Template implementation below

// AddConstraint()
//
// Adds a constraint to the system.
template <typename VariableType, typename ValueType>
template <typename U>
void ConstraintHandler<VariableType, ValueType>::AddConstraint(U constraint) {
  constraints_.push_back(
      CowPtr<ConstraintHusk>(new ConstraintWrapper<U>(std::move(constraint))));
}

template <typename VariableType, typename ValueType>
ConstraintViolation ConstraintHandler<VariableType, ValueType>::CheckValue(
    AnalysisContext ctx, const ValueType& value) const {
  for (const auto& constraint : constraints_) {
    if (auto check = constraint->CheckValue(ctx, value)) return check;
  }
  return ConstraintViolation::None();
}

template <typename VariableType, typename ValueType>
auto ConstraintHandler<VariableType, ValueType>::ToString() const
    -> std::string {
  if (constraints_.empty()) return "has no constraints";
  std::string s;
  for (const auto& constraint : constraints_) {
    s += (s.empty() ? "" : ", ") + constraint->ToString();
  }
  return s;
}

template <typename VariableType, typename ValueType>
void ConstraintHandler<VariableType, ValueType>::ApplyAllTo(
    VariableType& other) const {
  for (const auto& constraint : constraints_) constraint->ApplyTo(other);
}

}  // namespace librarian
}  // namespace moriarty

#endif  // MORIARTY_SRC_LIBRARIAN_CONSTRAINT_HANDLER_H_
