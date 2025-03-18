/*
 * Copyright 2025 Darcy Best
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MORIARTY_SRC_VARIABLES_CUSTOM_CONSTRAINT_H_
#define MORIARTY_SRC_VARIABLES_CUSTOM_CONSTRAINT_H_

#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "src/contexts/librarian/analysis_context.h"
#include "src/librarian/debug_print.h"
#include "src/variables/constraints/base_constraints.h"

namespace moriarty {

// CustomConstraint
//
// Holds a constraint specified by the user.
template <typename T>
class CustomConstraint : MConstraint {
 public:
  // The value must satisfy `checker`. `name` is used for debugging and error
  // messages.
  explicit CustomConstraint(std::string_view name,
                            std::function<bool(const T&)> checker);

  // The value must satisfy `checker`. `name` is used for debugging and error
  // messages. This constraint depends on the variables in `dependencies`. Their
  // values must be generated before this constraint is checked.
  explicit CustomConstraint(
      std::string_view name, std::vector<std::string> dependencies,
      std::function<bool(librarian::AnalysisContext, const T&)> checker);

  // Returns the name of the constraint.
  [[nodiscard]] std::string GetName() const;

  // Returns the names of the variables this constraint depends on.
  [[nodiscard]] std::vector<std::string> GetDependencies() const;

  // Determines if `value` satisfies the constraint.
  [[nodiscard]] bool IsSatisfiedWith(librarian::AnalysisContext ctx,
                                     const T& value) const;

  // Returns a string representation of the constraint.
  [[nodiscard]] std::string ToString() const;

  // Returns a string explaining why `value` does not satisfy the constraint.
  [[nodiscard]] std::string Explanation(const T& value) const;

 private:
  std::string name_;
  std::function<bool(librarian::AnalysisContext, const T&)> constraint_;
  std::vector<std::string> dependencies_;
};

// ----------------------------------------------------------------------------
// Template implementation below

template <typename T>
CustomConstraint<T>::CustomConstraint(std::string_view name,
                                      std::function<bool(const T&)> checker)
    : name_(std::string(name)),
      constraint_([checker](auto, const T& value) { return checker(value); }) {}

template <typename T>
CustomConstraint<T>::CustomConstraint(
    std::string_view name, std::vector<std::string> dependencies,
    std::function<bool(librarian::AnalysisContext, const T&)> checker)
    : name_(std::string(name)),
      constraint_(std::move(checker)),
      dependencies_(std::move(dependencies)) {}

template <typename T>
std::vector<std::string> CustomConstraint<T>::GetDependencies() const {
  return dependencies_;
}

template <typename T>
std::string CustomConstraint<T>::GetName() const {
  return name_;
}

template <typename T>
bool CustomConstraint<T>::IsSatisfiedWith(librarian::AnalysisContext ctx,
                                          const T& value) const {
  return constraint_(ctx, value);
}

template <typename T>
std::string CustomConstraint<T>::ToString() const {
  return std::format("[CustomConstraint] {}", name_);
}

template <typename T>
std::string CustomConstraint<T>::Explanation(const T& value) const {
  return std::format("{} does not satisfy the custom constraint `{}`",
                     librarian::DebugString(value), name_);
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_CUSTOM_CONSTRAINT_H_
