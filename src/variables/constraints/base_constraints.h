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

#ifndef MORIARTY_SRC_VARIABLES_CONSTRAINTS_BASE_CONSTRAINTS_H_
#define MORIARTY_SRC_VARIABLES_CONSTRAINTS_BASE_CONSTRAINTS_H_

#include <concepts>
#include <cstdint>
#include <format>
#include <functional>
#include <initializer_list>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace moriarty {

// Base class for all constraints in Moriarty.
class MConstraint {};

// Constraint stating that the variable must be exactly this value.
//
// Examples:
//  * Exactly(10) accepts only the integer 10.
//  * Exactly({1, 2, 3}) accepts only the vector {1, 2, 3}.
//
// NOTE:
// We have added several special constructors to handle different types.
// This should make working with basic types nicer. For non-basic types, you may
// need to specify the type explicitly:
//   E.g.,
//         Exactly<std::tuple<int, int>>({2, 4})) or
//         Exactly(std::tuple<int, int>({2, 4})) or
//         std::tuple<int, int> x = {2, 4}; Exactly(x);
template <typename T>
class Exactly : public MConstraint {
 public:
  // The variable must be exactly this value.
  // E.g., `Exactly(10)` or `Exactly(std::vector<int64_t>{1, 2, 3})`
  explicit Exactly(T value);

  // Special constructor for integral types -> int64_t
  template <std::integral IntLike>
    requires std::same_as<T, int64_t>
  explicit Exactly(IntLike num);

  // Special constructor for string-like types (const char*, std::string_view)
  template <std::convertible_to<std::string_view> StringLike>
    requires std::same_as<T, std::string>
  explicit Exactly(StringLike str);

  // Special constructor for std::span<E> -> std::vector<E>
  template <typename ElemType>
    requires std::same_as<T, std::vector<std::remove_cv_t<ElemType>>>
  explicit Exactly(std::span<ElemType> sp);

  // Special constructor for std::initializer_list<E> -> std::vector<E>
  template <typename ElemType>
    requires std::same_as<T, std::vector<std::remove_cv_t<ElemType>>>
  explicit Exactly(std::initializer_list<ElemType> sp);

  // Returns the value that the variable must be.
  [[nodiscard]] T GetValue() const;

  // Returns a human-readable representation of this constraint. Uses
  // `to_string` to print `value` in the message.
  [[nodiscard]] std::string ToString(
      std::function<std::string(const T&)> to_string) const;

  // Determines if all constraints are satisfied with the given value.
  [[nodiscard]] bool IsSatisfiedWith(const T& value) const;

  // Returns a string explaining why the value does not satisfy the constraints.
  // It is assumed that IsSatisfiedWith() returned false.
  // Uses `to_string` to print `value` in the message.
  [[nodiscard]] std::string Explanation(
      std::function<std::string(const T&)> to_string, const T& value) const;

 private:
  T value_;
};

// The variable must be one of these values.
// Examples:
//  * OneOf({1, 2, 4})
//  * OneOf({"3 * N + 1", "1", "X"})
//  * OneOf({"Possible", "Impossible"})
template <typename T>
class OneOf : public MConstraint {
 public:
  // Constructor for std::vector<T>
  explicit OneOf(std::vector<T> options);

  // Constructor for std::span<T> -> Convert to std::vector<T>
  explicit OneOf(std::span<typename std::remove_cv<T>::type> options);

  // Constructor for std::initializer_list<T> -> Convert to std::vector<T>
  explicit OneOf(std::initializer_list<T> options);

  // Convert integral types to int64_t
  template <std::integral IntLike>
    requires std::same_as<T, int64_t>
  explicit OneOf(std::vector<IntLike> options);

  template <std::integral IntLike>
    requires std::same_as<T, int64_t>
  explicit OneOf(std::span<IntLike> options);

  template <std::integral IntLike>
    requires std::same_as<T, int64_t>
  explicit OneOf(std::initializer_list<IntLike> options);

  // Convert string-like types to std::string
  template <std::convertible_to<std::string_view> StringLike>
    requires std::same_as<T, std::string>
  explicit OneOf(std::vector<StringLike> options);

  template <std::convertible_to<std::string_view> StringLike>
    requires std::same_as<T, std::string>
  explicit OneOf(std::span<StringLike> options);

  template <std::convertible_to<std::string_view> StringLike>
    requires std::same_as<T, std::string>
  explicit OneOf(std::initializer_list<StringLike> options);

  // Returns the values that the variable must be one of.
  [[nodiscard]] std::vector<T> GetOptions() const;

  // Returns a human-readable representation of this constraint. Uses
  // `to_string` to print `options` in the message.
  [[nodiscard]] std::string ToString(
      std::function<std::string(const T&)> to_string) const;

  // Determines if all constraints are satisfied with the given value.
  [[nodiscard]] bool IsSatisfiedWith(const T& value) const;

  // Returns a string explaining why the value does not satisfy the constraints.
  // It is assumed that IsSatisfiedWith() returned false.
  // Uses `to_string` to print `options` and `value` in the message.
  [[nodiscard]] std::string Explanation(
      std::function<std::string(const T&)> to_string, const T& value) const;

 private:
  std::vector<T> options_;
};

// -----------------------------------------------------------------------------
//  CTAD for Exactly<>

// General deduction guide
template <typename T>
Exactly(T) -> Exactly<T>;

// Special deduction guide for integral types
template <std::integral IntLike>
Exactly(IntLike) -> Exactly<int64_t>;

// Special deduction guide for string-like types
template <std::convertible_to<std::string_view> StringLike>
Exactly(StringLike) -> Exactly<std::string>;

// Special deduction guide for std::span<T> -> std::vector<T>
template <typename ElemType>
Exactly(std::span<ElemType>)
    -> Exactly<std::vector<std::remove_cv_t<ElemType>>>;

// Special deduction guide for std::initializer_list<T> -> std::vector<T>
template <typename ElemType>
Exactly(std::initializer_list<ElemType>)
    -> Exactly<std::vector<std::remove_cv_t<ElemType>>>;

// -----------------------------------------------------------------------------
//  CTAD for OneOf<>
// * We need all combinations of
//      (vector, span, initializer_list) and (general, integer, string).

// General deduction guide
template <typename T>
OneOf(std::vector<T>) -> OneOf<T>;

template <typename T>
OneOf(std::span<T>) -> OneOf<T>;

template <typename T>
OneOf(std::initializer_list<T>) -> OneOf<T>;

// Special deduction guide for integral types
template <std::integral IntLike>
OneOf(std::vector<IntLike>) -> OneOf<int64_t>;

template <std::integral IntLike>
OneOf(std::span<IntLike>) -> OneOf<int64_t>;

template <std::integral IntLike>
OneOf(std::initializer_list<IntLike>) -> OneOf<int64_t>;

// Special deduction guide for string-like types
template <std::convertible_to<std::string_view> StringLike>
OneOf(std::vector<StringLike>) -> OneOf<std::string>;

template <std::convertible_to<std::string_view> StringLike>
OneOf(std::span<StringLike>) -> OneOf<std::string>;

template <std::convertible_to<std::string_view> StringLike>
OneOf(std::initializer_list<StringLike>) -> OneOf<std::string>;

// -----------------------------------------------------------------------------
//  Template Implementation Below

template <typename T>
Exactly<T>::Exactly(T value) : value_(std::move(value)) {}

template <typename T>
template <std::integral IntLike>
  requires std::same_as<T, int64_t>
Exactly<T>::Exactly(IntLike num) : value_(static_cast<int64_t>(num)) {
  if (!std::in_range<int64_t>(num)) {
    throw std::invalid_argument(
        std::format("{} does not fit into int64_t in Exactly", num));
  }
}

template <typename T>
template <std::convertible_to<std::string_view> StringLike>
  requires std::same_as<T, std::string>
Exactly<T>::Exactly(StringLike str) : value_(std::string(std::move(str))) {}

template <typename T>
template <typename ElemType>
  requires std::same_as<T, std::vector<std::remove_cv_t<ElemType>>>
Exactly<T>::Exactly(std::span<ElemType> sp)
    : value_(std::vector<ElemType>(sp.begin(), sp.end())) {}

template <typename T>
template <typename ElemType>
  requires std::same_as<T, std::vector<std::remove_cv_t<ElemType>>>
Exactly<T>::Exactly(std::initializer_list<ElemType> sp)
    : value_(sp.begin(), sp.end()) {}

template <typename T>
T Exactly<T>::GetValue() const {
  return value_;
}

template <typename T>
std::string Exactly<T>::ToString(
    std::function<std::string(const T&)> to_string) const {
  return std::format("is exactly {}", to_string(value_));
}

template <typename T>
bool Exactly<T>::IsSatisfiedWith(const T& value) const {
  return value == value_;
}

template <typename T>
std::string Exactly<T>::Explanation(
    std::function<std::string(const T&)> to_string, const T& value) const {
  return std::format("`{}` is not exactly `{}`", to_string(value),
                     to_string(value_));
}

template <typename T>
OneOf<T>::OneOf(std::vector<T> options) : options_(std::move(options)) {}

template <typename T>
OneOf<T>::OneOf(std::span<typename std::remove_cv<T>::type> options)
    : options_(options.begin(), options.end()) {}

template <typename T>
OneOf<T>::OneOf(std::initializer_list<T> options) : options_(options) {}

template <typename T>
template <std::integral IntLike>
  requires std::same_as<T, int64_t>
OneOf<T>::OneOf(std::vector<IntLike> options) {
  options_.reserve(options.size());
  for (IntLike v : options) {
    if (!std::in_range<int64_t>(v)) {
      throw std::invalid_argument(
          std::format("{} does not fit into int64_t in OneOf", v));
    }
    options_.push_back(static_cast<int64_t>(v));
  }
}

template <typename T>
template <std::integral IntLike>
  requires std::same_as<T, int64_t>
OneOf<T>::OneOf(std::span<IntLike> options) {
  options_.reserve(options.size());
  for (IntLike v : options) {
    if (!std::in_range<int64_t>(v)) {
      throw std::invalid_argument(
          std::format("{} does not fit into int64_t in OneOf", v));
    }
    options_.push_back(static_cast<int64_t>(v));
  }
}

template <typename T>
template <std::integral IntLike>
  requires std::same_as<T, int64_t>
OneOf<T>::OneOf(std::initializer_list<IntLike> options) {
  options_.reserve(options.size());
  for (IntLike v : options) {
    if (!std::in_range<int64_t>(v)) {
      throw std::invalid_argument(
          std::format("{} does not fit into int64_t in OneOf", v));
    }
    options_.push_back(static_cast<int64_t>(v));
  }
}

template <typename T>
template <std::convertible_to<std::string_view> StringLike>
  requires std::same_as<T, std::string>
OneOf<T>::OneOf(std::vector<StringLike> options) {
  options_.reserve(options.size());
  for (const auto& v : options) {
    options_.push_back(static_cast<std::string>(v));
  }
}

template <typename T>
template <std::convertible_to<std::string_view> StringLike>
  requires std::same_as<T, std::string>
OneOf<T>::OneOf(std::span<StringLike> options) {
  options_.reserve(options.size());
  for (const auto& v : options) {
    options_.push_back(static_cast<std::string>(v));
  }
}

template <typename T>
template <std::convertible_to<std::string_view> StringLike>
  requires std::same_as<T, std::string>
OneOf<T>::OneOf(std::initializer_list<StringLike> options) {
  options_.reserve(options.size());
  for (const auto& v : options) {
    options_.push_back(static_cast<std::string>(v));
  }
}

template <typename T>
std::vector<T> OneOf<T>::GetOptions() const {
  return options_;
}

template <typename T>
std::string OneOf<T>::ToString(
    std::function<std::string(const T&)> to_string) const {
  std::string options = "{";
  for (const auto& option : options_) {
    if (options.size() > 1) options += ", ";
    options += std::format("`{}`", to_string(option));
  }
  options += "}";

  return std::format("is one of {}", options);
}

template <typename T>
bool OneOf<T>::IsSatisfiedWith(const T& value) const {
  return std::find(options_.begin(), options_.end(), value) != options_.end();
}

template <typename T>
std::string OneOf<T>::Explanation(
    std::function<std::string(const T&)> to_string, const T& value) const {
  std::string options = "{";
  for (const auto& option : options_) {
    if (options.size() > 1) options += ", ";
    options += std::format("`{}`", to_string(option));
  }
  options += "}";

  return std::format("`{}` is not one of {}", to_string(value), options);
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_CONSTRAINTS_BASE_CONSTRAINTS_H_
