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

#ifndef MORIARTY_SRC_VARIABLES_REAL_H_
#define MORIARTY_SRC_VARIABLES_REAL_H_

#include <compare>
#include <concepts>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>

namespace moriarty {

// Real
//
// Logically isomorphic to a simplified `double`. However, we take every effort
// to make the value "exact". Upon receiving a value, we convert it to a
// fraction and treat that value as exact. This class is meant to be used to
// describe constraints on real numbers, not to perform arithmetic on them. As
// such, no arithmetic operations are provided. You are responsible for doing
// the arithmetic yourself and ensuring the precision you need.
class Real {
 public:
  Real(double) = delete;  // You may *not* create a Real from a double.
  Real(long double) = delete;
  Real(float) = delete;

  // Creates a Real variable with an integer value.
  explicit Real(std::integral auto value);

  // Creates a Real variable represented by numerator / denominator.
  explicit Real(int64_t numerator, int64_t denominator);

  // Creates a Real variable from a string representation of a real number or
  // expression.
  //
  // The string must be:
  //  * a simple real number (e.g., "3.14", "10", "3.0e-10"). This value must be
  //    expressible as a fraction using int64_t as both numerator and
  //    denominator. For example, "0.1" is allowed (as 1/10), but "1e-100" will
  //    be rejected since the denominator is too large. A rather naive
  //    implementation is used to parse the string (essentially using 10^k as
  //    the denominator, then reducing), so long strings may unexpectedly
  //    fail. If you know the fraction you're trying to make, use the
  //    constructor that takes numerator and denominator directly.
  //
  // TODO: Support more complex expressions, such as:
  //  * an integer expression (e.g., "3 * N + 1", "10^x"). All arithmetic is
  //    done over the integers, not real numbers. So "3 * N / 4" is not
  //    mathematically equivalent to "0.75 * N". This may change in the future,
  //    so please do not depend on this behaviour.
  explicit Real(std::string_view value);

  // Returns the value of the Real variable.
  double GetApproxValue() const;

  struct Fraction {
    int64_t numerator;
    int64_t denominator;
  };
  // Returns the value of the Real variable as a fraction.
  Fraction GetValue() const { return {numerator_, denominator_}; }

  std::string ToString() const {
    if (denominator_ == 1) {
      return std::to_string(numerator_);
    } else {
      return std::to_string(numerator_) + '/' + std::to_string(denominator_);
    }
  }

  friend std::ostream& operator<<(std::ostream& os, const Real::Fraction& f) {
    if (f.denominator == 1) {
      return os << f.numerator;
    } else {
      return os << f.numerator << '/' << f.denominator;
    }
  }

  friend std::partial_ordering operator<=>(const Real& r, double d);
  // Returns a partial order so that both have the same return types.
  friend std::partial_ordering operator<=>(const Real& r, int64_t d);

 private:
  int64_t numerator_;
  int64_t denominator_ = 1;
};

// ----------------------------------------------------------------------------
//  Template implementation below

Real::Real(std::integral auto value)
    : numerator_(static_cast<int64_t>(value)), denominator_(1) {}

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_REAL_H_
