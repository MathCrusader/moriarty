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
#include <vector>

#include "src/contexts/librarian/analysis_context.h"
#include "src/librarian/cow_ptr.h"

namespace moriarty {
namespace librarian {

// ConstraintHandler
//
// Holds a collection of type-erased constraints. Each constraint must have a
// particular API.
//
// T is the type we are adding constraints to (int, std::string, etc).
template <typename T>
class ConstraintHandler {
 public:
  // to_string_prefix will be prepended to the string representation of the
  // constraints.
  explicit ConstraintHandler(std::string to_string_prefix)
      : to_string_prefix_(std::move(to_string_prefix)) {}

  // AddConstraint()
  //
  // Adds a constraint to the system.
  template <typename U>
  void AddConstraint(U constraint);

  // IsSatisfiedWith()
  //
  // Determines if all constraints are satisfied with the given value.
  auto IsSatisfiedWith(AnalysisContext ctx, const T& value) const -> bool;

  // Explanation()
  //
  // Returns a string explaining why the value does not satisfy the constraints.
  // It is assumed that IsSatisfiedWith() returned false.
  auto Explanation(AnalysisContext ctx, const T& value) const -> std::string;

  // ToString()
  //
  // Returns a string representation of the constraints.
  auto ToString() const -> std::string;

 private:
  class ConstraintHusk {
   public:
    virtual ~ConstraintHusk() = default;
    virtual auto IsSatisfiedWith(AnalysisContext ctx, const T& value) const
        -> bool = 0;
    virtual auto Explanation(AnalysisContext ctx, const T& value) const
        -> std::string = 0;
    virtual auto ToString() const -> std::string = 0;
  };

  template <typename U>
  class ConstraintWrapper : public ConstraintHusk {
   public:
    explicit ConstraintWrapper(U constraint)
        : constraint_(std::move(constraint)) {}
    auto IsSatisfiedWith(AnalysisContext ctx, const T& value) const
        -> bool override {
      return constraint_.IsSatisfiedWith(ctx, value);
    }
    auto Explanation(AnalysisContext ctx, const T& value) const
        -> std::string override {
      return constraint_.Explanation(ctx, value);
    }
    auto ToString() const -> std::string override {
      return constraint_.ToString();
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
template <typename T>
template <typename U>
void ConstraintHandler<T>::AddConstraint(U constraint) {
  constraints_.push_back(
      CowPtr<ConstraintHusk>(new ConstraintWrapper<U>(std::move(constraint))));
}

template <typename T>
auto ConstraintHandler<T>::IsSatisfiedWith(AnalysisContext ctx,
                                           const T& value) const -> bool {
  for (const auto& constraint : constraints_) {
    if (!constraint->IsSatisfiedWith(ctx, value)) {
      return false;
    }
  }
  return true;
}

template <typename T>
auto ConstraintHandler<T>::Explanation(AnalysisContext ctx,
                                       const T& value) const -> std::string {
  std::string explanation;
  for (const auto& constraint : constraints_) {
    if (!constraint->IsSatisfiedWith(ctx, value)) {
      if (!explanation.empty()) explanation += "; ";
      explanation += constraint->Explanation(ctx, value);
    }
  }
  return explanation;
}

template <typename T>
auto ConstraintHandler<T>::ToString() const -> std::string {
  std::string str = to_string_prefix_ + "\n";
  for (const auto& constraint : constraints_)
    str += " * " + constraint->ToString() + "\n";
  return str;
}

}  // namespace librarian
}  // namespace moriarty

#endif  // MORIARTY_SRC_LIBRARIAN_CONSTRAINT_HANDLER_H_
