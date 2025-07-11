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

#ifndef MORIARTY_SRC_INTERNAL_RANGE_H_
#define MORIARTY_SRC_INTERNAL_RANGE_H_

#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "src/internal/expressions.h"
#include "src/types/real.h"

namespace moriarty {

// Range
//
// All integers between min and max, inclusive.
//
// * An empty range can be created by setting min > max.
// * Additional calls to `AtMost` and `AtLeast` add extra constraints, and does
//   not overwrite the old ones.
class Range {
 public:
  // AtLeast()
  //
  // This range is at least `minimum`. For example,
  //   `AtLeast(5)`
  //
  // Multiple calls to `AtLeast` are ANDed  together. For example,
  //   `AtLeast(5); AtLeast("X + Y"); AtLeast("W");`
  // means that this is at least max({5, Evaluate("X + Y"), Evaluate("W")}).
  Range& AtLeast(int64_t minimum);
  Range& AtLeast(Expression minimum);
  Range& AtLeast(const Real& minimum);

  // AtMost()
  //
  // This range is at least `maximum`. For example,
  //   `AtMost(5)`
  //
  // Multiple calls to `AtMost` are ANDed  together. For example,
  //   `AtMost(5); AtMost("X + Y"); AtMost("W");`
  // means that this is at least min({5, Evaluate("X + Y"), Evaluate("W")}).
  Range& AtMost(int64_t maximum);
  Range& AtMost(Expression maximum);
  Range& AtMost(const Real& maximum);

  template <typename T>
  struct ExtremeValues {
    T min;
    T max;

    friend bool operator==(const ExtremeValues& e1,
                           const ExtremeValues& e2) = default;
  };

  // IntegerExtremes()
  //
  // Returns the two (integer-valued) extremes of the range (min and max).
  // Returns `std::nullopt` if the range is empty.
  //
  // Uses get_value(var_name) to get the current value of any needed variables.
  std::optional<ExtremeValues<int64_t>> IntegerExtremes(
      std::function<int64_t(std::string_view)> get_value) const;

  // RealExtremes()
  //
  // Returns the two (real-valued) extremes of the range (min and max). Returns
  // `std::nullopt` if the range is empty.
  //
  // Uses get_value(var_name) to get the current value of any needed variables.
  std::optional<ExtremeValues<Real>> RealExtremes(
      std::function<int64_t(std::string_view)> get_value) const;

  // Intersect()
  //
  // Intersects `other` with this Range (updating this range with the
  // intersection).
  void Intersect(const Range& other);

  // ToString()
  //
  // Returns a string representation of this range.
  [[nodiscard]] std::string ToString() const;

  // Determine if two Ranges are equal.
  //
  // The exact implementation is not guaranteed to be stable over time.
  // For now, Range.AtMost(5) and Range.AtMost("5") are considered different and
  // insertion order of expressions matters, but may not in the future.
  friend bool operator==(const Range& r1, const Range& r2);

 private:
  int64_t min_int_ = std::numeric_limits<int64_t>::min();
  int64_t max_int_ = std::numeric_limits<int64_t>::max();
  std::optional<Real> min_real_;
  std::optional<Real> max_real_;

  // `min_exprs_` and `max_exprs_` are lists of Expressions that represent the
  // lower/upper bounds. They must be evaluated when `Extremes()` is called in
  // order to determine which is largest/smallest.
  std::vector<Expression> min_exprs_;
  std::vector<Expression> max_exprs_;
};

// Creates a range with no elements in it.
Range EmptyRange();

}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_RANGE_H_
