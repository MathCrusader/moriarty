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

#include <cmath>
#include <compare>
#include <concepts>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

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
  // You may *not* create a Real from double/float.
  Real(std::floating_point auto) = delete;

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

  // Returns the smallest integer that is greater than or equal to me
  int64_t Ceiling() const;

  // Returns the largest integer that is less than or equal to me
  int64_t Floor() const;

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

  template <std::floating_point Float>
  friend std::partial_ordering operator<=>(const Real& r, Float d);
  friend std::strong_ordering operator<=>(const Real& r,
                                          std::signed_integral auto d);
  friend std::strong_ordering operator<=>(const Real& r,
                                          std::unsigned_integral auto d);

  friend bool operator==(const Real& r, std::floating_point auto d);
  friend bool operator==(const Real& r, std::signed_integral auto d);
  friend bool operator==(const Real& r, std::unsigned_integral auto d);

 private:
  int64_t numerator_;
  int64_t denominator_ = 1;
};

// ----------------------------------------------------------------------------
//  Template implementation below

Real::Real(std::integral auto value)
    : numerator_(static_cast<int64_t>(value)), denominator_(1) {
  if (!std::in_range<int64_t>(value)) {
    throw std::out_of_range("Value is out of range for int64_t");
  }
}

namespace moriarty_internal {

inline int HighestBit(__int128 value) {
  if (value == 0) return 0;  // Special case for zero
  uint64_t hi = static_cast<uint64_t>(value >> 64);
  if (hi != 0) return 64 + (63 - std::countl_zero(hi));
  uint64_t lo = static_cast<uint64_t>(value);
  return 63 - std::countl_zero(lo);
}

}  // namespace moriarty_internal

template <std::floating_point Float>
std::partial_ordering operator<=>(const Real& r, Float d) {
  if (std::isnan(d)) return std::partial_ordering::unordered;
  if (std::isinf(d)) {
    return d > 0 ? std::partial_ordering::less : std::partial_ordering::greater;
  }
  if (r.numerator_ == 0) {
    // 0 == +0.0 and 0 == -0.0
    if (d == 0.0) return std::partial_ordering::equivalent;
    return d > 0 ? std::partial_ordering::less : std::partial_ordering::greater;
  }
  if (d == 0.0) {
    return r.numerator_ > 0 ? std::partial_ordering::greater
                            : std::partial_ordering::less;
  }
  if (r.numerator_ < 0 && d > 0) return std::partial_ordering::less;
  if (r.numerator_ > 0 && d < 0) return std::partial_ordering::greater;
  bool flip = r.numerator_ < 0;

  // d = mantissa * 2^exp, 0.5 ≤ abs(mantissa) < 1.0
  int exp;
  Float mantissa = std::frexp(d, &exp);

  // Scale mantissa to get an integer numerator
  constexpr int kMantissaBits = std::numeric_limits<Float>::digits;
  uint64_t d_numer =
      static_cast<uint64_t>(std::ldexp(std::abs(mantissa), kMantissaBits));
  int d_exponent = exp - kMantissaBits;

  // * r = a / b
  // * d = m * 2^e
  //
  // Thus,  r < d => a < b * m * 2^e
  // But we will handle the 2^e separately since `e` might be positive or
  // negative.
  __int128 lhs = static_cast<__int128>(r.numerator_);              // a
  __int128 rhs = static_cast<__int128>(r.denominator_) * d_numer;  // b * m
  if (flip) lhs = -lhs;  // rhs is already > 0

  auto ans = [&]() {
    if (d_exponent < 0) {  // a * 2^(-e) vs b * m
      if (-d_exponent + moriarty_internal::HighestBit(lhs) >= 126) {
        // a * 2^(-e) >= 2^126 > b * m
        return std::partial_ordering::greater;
      }
      lhs <<= -d_exponent;  // a * 2^(-e) < 2^126, so safe.
    } else {                // a vs b * m * 2^e
      // Check if lhs is already smaller, since rhs is only getting larger
      if (lhs < rhs) return std::partial_ordering::less;
      if (d_exponent + moriarty_internal::HighestBit(rhs) >= 126) {
        // b * m * 2^e >= 2^126 > a
        return std::partial_ordering::less;
      }
      rhs <<= d_exponent;  // b * m * 2^e < 2^126, so safe.
    }
    if (lhs == rhs) return std::partial_ordering::equivalent;
    return (lhs > rhs ? std::partial_ordering::greater
                      : std::partial_ordering::less);
  }();

  if (flip) {
    if (ans == std::partial_ordering::less)
      ans = std::partial_ordering::greater;
    else if (ans == std::partial_ordering::greater)
      ans = std::partial_ordering::less;
  }
  return ans;
}

std::strong_ordering operator<=>(const Real& r, std::signed_integral auto d) {
  __int128 lhs = static_cast<__int128>(r.numerator_);
  __int128 rhs = static_cast<__int128>(r.denominator_) * d;
  if (lhs == rhs) return std::strong_ordering::equivalent;
  return (lhs < rhs ? std::strong_ordering::less
                    : std::strong_ordering::greater);
}

std::strong_ordering operator<=>(const Real& r, std::unsigned_integral auto d) {
  if (r.numerator_ < 0) {
    // If r is negative, it is always less than any positive d.
    return std::strong_ordering::less;
  }
  __int128 lhs = static_cast<__int128>(r.numerator_);
  __int128 rhs = static_cast<__int128>(r.denominator_) * d;
  if (lhs == rhs) return std::strong_ordering::equivalent;
  return (lhs < rhs ? std::strong_ordering::less
                    : std::strong_ordering::greater);
}

bool operator==(const Real& r, std::floating_point auto d) {
  return (r <=> d) == 0;
}
bool operator==(const Real& r, std::signed_integral auto d) {
  return (r <=> d) == 0;
}
bool operator==(const Real& r, std::unsigned_integral auto d) {
  return (r <=> d) == 0;
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_REAL_H_
