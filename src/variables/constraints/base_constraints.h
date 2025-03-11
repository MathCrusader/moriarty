/*
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

#ifndef MORIARTY_SRC_VARIABLES_CONSTRAINTS_BASE_CONSTRAINTS_H_
#define MORIARTY_SRC_VARIABLES_CONSTRAINTS_BASE_CONSTRAINTS_H_

#include <concepts>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace moriarty {

// Base class for all constraints in Moriarty.
class MConstraint {};

// Constraint stating that the variable must be exactly this value.
template <typename T>
class Exactly : public MConstraint {
 public:
  // The variable must be exactly this value.
  // E.g., `Exactly(10)` or `Exactly(std::vector<int64_t>{1, 2, 3})`
  explicit Exactly(T value);

  // Returns the value that the variable must be.
  [[nodiscard]] T GetValue() const;

 private:
  T value_;
};

// Constraint stating that the variable must be exactly this string value.
// Specialization for string-like types.
template <>
class Exactly<std::string> : public MConstraint {
 public:
  // The string variable must be exactly this value.
  // E.g., Exactly("abc");
  explicit Exactly(std::string_view value);

  // Returns the value that the variable must be.
  [[nodiscard]] std::string GetValue() const;

 private:
  std::string value_;
};

// The variable must be one of these values.
// Examples:
//  * OneOf({1, 2, 4})
//  * OneOf({"a", "b", "c"})
//  * OneOf(1, 2, 4)
//  * OneOf("Possible", "Impossible")
template <typename T>
class OneOf : public MConstraint {
 public:
  template <typename Container>
  explicit OneOf(Container&& options);
  explicit OneOf(std::initializer_list<T> options);
  explicit OneOf(T options...);

  // Returns the values that the variable must be one of.
  [[nodiscard]] std::vector<T> GetOptions() const;

 private:
  std::vector<T> options_;
};

// Remap all integer types (via CTAD) to MInteger-compatible type.
// E.g., Exactly(10) is remapped to Exactly<int64_t>(10)
template <typename T>
  requires std::integral<T>
Exactly(T) -> Exactly<int64_t>;

// Remap all string types (via CTAD) to MString-compatible type.
// E.g., Exactly("abc") is remapped to Exactly<std::string>("abc")
template <typename T>
  requires std::convertible_to<T, std::string_view>
Exactly(T) -> Exactly<std::string>;

// -----------------------------------------------------------------------------
//  Template Implementation Below

template <typename T>
Exactly<T>::Exactly(T value) : value_(std::move(value)) {}

template <typename T>
T Exactly<T>::GetValue() const {
  return value_;
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_CONSTRAINTS_BASE_CONSTRAINTS_H_
