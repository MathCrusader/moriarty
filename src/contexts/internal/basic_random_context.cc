// Copyright 2025 Darcy Best
// Copyright 2024 Google LLC
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

#include "src/contexts/internal/basic_random_context.h"

#include <cstdint>
#include <format>
#include <limits>
#include <stdexcept>

namespace moriarty {
namespace moriarty_internal {

BasicRandomContext::BasicRandomContext(
    std::reference_wrapper<RandomEngine> engine)
    : engine_(engine) {}

int64_t BasicRandomContext::RandomInteger(int64_t min, int64_t max) {
  if (min > max) {
    throw std::invalid_argument(std::format(
        "RandomInteger({}, {}) invalid (need min <= max)", min, max));
  }
  return engine_.get().RandInt(min, max);
}

int64_t BasicRandomContext::RandomInteger(int64_t n) {
  if (n <= 0) {
    throw std::invalid_argument(
        std::format("RandomInteger({}) invalid (need n > 0)", n));
  }
  return RandomInteger(0, n - 1);
}

double BasicRandomContext::RandomReal(Real min, Real max) {
  if (min > max) {
    throw std::invalid_argument(
        std::format("RandomReal({}, {}) invalid (need min <= max)",
                    min.ToString(), max.ToString()));
  }
  // TODO: Handle this better, we have more context than normal doubles.
  return RandomReal(min.GetApproxValue(), max.GetApproxValue());
}

double BasicRandomContext::RandomReal(Real n) {
  auto [num, den] = n.GetValue();
  if (num <= 0) {
    throw std::invalid_argument(
        std::format("RandomReal({}) invalid (need n > 0)", n.ToString()));
  }
  static constexpr int digits = std::numeric_limits<double>::digits;
  int64_t k = RandomInteger(int64_t{1} << digits);
  double value = static_cast<double>(num) / den * k;
  return std::ldexp(value, -digits);
}

double BasicRandomContext::RandomReal(double min, double max) {
  if (min > max) {
    throw std::invalid_argument(
        std::format("RandomReal({}, {}) invalid (need min <= max)", min, max));
  }
  if (min == max) return min;
  return RandomReal(max - min) + min;
}

double BasicRandomContext::RandomReal(double n) {
  if (n <= 0) {
    throw std::invalid_argument(
        std::format("RandomReal({}) invalid (need n > 0)", n));
  }
  static constexpr int digits = std::numeric_limits<double>::digits;
  int64_t k = RandomInteger(int64_t{1} << digits);
  return std::ldexp(static_cast<double>(k), -digits) * n;
}

std::vector<int> BasicRandomContext::RandomPermutation(int n) {
  if (n < 0) {
    throw std::invalid_argument(
        std::format("RandomPermutation({}) invalid (need n >= 0)", n));
  }
  return RandomPermutation<int>(n, 0);
}

}  // namespace moriarty_internal
}  // namespace moriarty
