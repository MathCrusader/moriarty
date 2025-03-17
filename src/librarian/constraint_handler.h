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

#include "src/contexts/librarian/analysis_context.h"
#include "src/librarian/cow_ptr.h"

namespace moriarty {
namespace librarian {

template <typename ConstraintType, typename ValueType>
concept ConstraintHasSimplifiedIsSatisfiedWithFn =
    requires(const ConstraintType& c, const ValueType& v) {
      { c.IsSatisfiedWith(v) } -> std::same_as<bool>;
    };

template <typename ConstraintType, typename ValueType>
concept ConstraintHasSimplifiedExplanationFn =
    requires(const ConstraintType& c, const ValueType& v) {
      { c.Explanation(v) } -> std::same_as<std::string>;
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
  // to_string_prefix will be prepended to the string representation of the
  // constraints.
  explicit ConstraintHandler(std::string to_string_prefix)
      : to_string_prefix_(std::move(to_string_prefix)) {}

  // AddConstraint()
  //
  // Adds a constraint to the system.
  template <typename ConstraintType>
  void AddConstraint(ConstraintType constraint);

  // IsSatisfiedWith()
  //
  // Determines if all constraints are satisfied with the given value.
  auto IsSatisfiedWith(AnalysisContext ctx, const ValueType& value) const
      -> bool;

  // Explanation()
  //
  // Returns a string explaining why the value does not satisfy the constraints.
  // It is assumed that IsSatisfiedWith() returned false.
  auto Explanation(AnalysisContext ctx, const ValueType& value) const
      -> std::string;

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
    virtual auto IsSatisfiedWith(AnalysisContext ctx,
                                 const ValueType& value) const -> bool = 0;
    virtual auto Explanation(AnalysisContext ctx, const ValueType& value) const
        -> std::string = 0;
    virtual auto ToString() const -> std::string = 0;
    virtual auto ApplyTo(VariableType& other) const -> void = 0;
  };

  template <typename U>
  class ConstraintWrapper : public ConstraintHusk {
   public:
    ~ConstraintWrapper() override = default;
    explicit ConstraintWrapper(U constraint)
        : constraint_(std::move(constraint)) {}
    auto IsSatisfiedWith(AnalysisContext ctx, const ValueType& value) const
        -> bool override {
      if constexpr (ConstraintHasSimplifiedIsSatisfiedWithFn<U, ValueType>) {
        return constraint_.IsSatisfiedWith(value);
      } else {
        return constraint_.IsSatisfiedWith(ctx, value);
      }
    }
    auto Explanation(AnalysisContext ctx, const ValueType& value) const
        -> std::string override {
      if constexpr (ConstraintHasSimplifiedExplanationFn<U, ValueType>) {
        return constraint_.Explanation(value);
      } else {
        return constraint_.Explanation(ctx, value);
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
  std::string to_string_prefix_;
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
auto ConstraintHandler<VariableType, ValueType>::IsSatisfiedWith(
    AnalysisContext ctx, const ValueType& value) const -> bool {
  for (const auto& constraint : constraints_) {
    if (!constraint->IsSatisfiedWith(ctx, value)) {
      return false;
    }
  }
  return true;
}

template <typename VariableType, typename ValueType>
auto ConstraintHandler<VariableType, ValueType>::Explanation(
    AnalysisContext ctx, const ValueType& value) const -> std::string {
  std::string explanation;
  for (const auto& constraint : constraints_) {
    if (!constraint->IsSatisfiedWith(ctx, value)) {
      if (!explanation.empty()) explanation += "; ";
      explanation += constraint->Explanation(ctx, value);
    }
  }
  return explanation;
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
