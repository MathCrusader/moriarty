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

#ifndef MORIARTY_SRC_CONTEXTS_INTERNAL_BASIC_RANDOM_CONTEXT_H_
#define MORIARTY_SRC_CONTEXTS_INTERNAL_BASIC_RANDOM_CONTEXT_H_

#include <cstdint>
#include <format>
#include <iterator>
#include <span>
#include <stdexcept>
#include <unordered_set>

#include "src/internal/random_engine.h"
#include "src/librarian/util/ref.h"
#include "src/types/real.h"

namespace moriarty {
namespace moriarty_internal {

template <typename T>
using ElementType = std::iter_value_t<decltype(std::begin(T{}))>;

// BasicRandomContext
//
// A class to handle Moriarty-agnostic randomness (RandomInteger,
// DistinctIntegers, RandomPermutation, etc).
class BasicRandomContext {
 public:
  explicit BasicRandomContext(Ref<RandomEngine> engine);

  // RandomInteger()
  //
  // Returns a random integer in the closed interval [min, max].
  [[nodiscard]] int64_t RandomInteger(int64_t min, int64_t max);

  // RandomInteger()
  //
  // Returns a random integer in the semi-closed interval [0, n). Useful for
  // random indices.
  [[nodiscard]] int64_t RandomInteger(int64_t n);

  // RandomReal()
  //
  // Returns a random real number in the closed interval [min, max].
  [[nodiscard]] double RandomReal(Real min, Real max);

  // RandomReal()
  //
  // Returns a random real number in the semi-closed interval [0, n).
  [[nodiscard]] double RandomReal(Real n);

  // RandomReal()
  //
  // Returns a random real number in the closed interval [min, max].
  [[nodiscard]] double RandomReal(double min, double max);

  // RandomReal()
  //
  // Returns a random real number in the semi-closed interval [0, n).
  [[nodiscard]] double RandomReal(double n);

  // Shuffle()
  //
  // Shuffles the elements in `container`.
  template <typename Container>
  void Shuffle(Container& container);

  // RandomElement()
  //
  // Returns a random element of `container`.
  template <typename Container>
  [[nodiscard]] ElementType<Container> RandomElement(
      const Container& container);

  // RandomElementsWithReplacement()
  //
  // Returns k (randomly ordered) elements of `container`, possibly with
  // duplicates.
  template <typename Container>
  [[nodiscard]] std::vector<ElementType<Container>>
  RandomElementsWithReplacement(const Container& container, int k);

  // RandomElementsWithoutReplacement()
  //
  // Returns k (randomly ordered) elements of `container`, without duplicates.
  //
  // Each element may appear at most once. Note that if there are duplicates in
  // `container`, each of those could be returned once each.
  //
  // So RandomElementsWithoutReplacement({0, 1, 1, 1}, 2) could return {1, 1}.
  template <typename Container>
  [[nodiscard]] std::vector<ElementType<Container>>
  RandomElementsWithoutReplacement(const Container& container, int k);

  // RandomPermutation()
  //
  // Returns a random permutation of {0, 1, ... , n-1}.
  [[nodiscard]] std::vector<int> RandomPermutation(int n);

  // RandomPermutation()
  //
  // Returns a random permutation of {min, min + 1, ... , min + (n-1)}.
  //
  // Requires min + (n-1) to not overflow T.
  template <std::integral T>
  [[nodiscard]] std::vector<T> RandomPermutation(int n, T min);

  // DistinctIntegers()
  //
  // Returns k (randomly ordered) distinct integers from
  // {min, min + 1, ... , min + (n-1)}.
  //
  // Requires min + (n-1) to not overflow T.
  template <std::integral T>
  [[nodiscard]] std::vector<T> DistinctIntegers(T n, int k, T min = 0);

  // RandomComposition()
  //
  // Returns a random composition. A composition partitions n objects
  // into k "buckets". This function returns a vector of size k representing the
  // number of objects in each bucket.
  //
  // Example: (11, 22) means the first bucket has 11 elements and the second
  // bucket has 22 elements. (22, 11) is a different composition.
  //
  // A lower bound on bucket size can be set with `min_bucket_size`. It is most
  // common that this is 0 or 1.
  //
  // Requires n + (k - 1) to not overflow T.
  template <std::integral T>
  [[nodiscard]] std::vector<T> RandomComposition(T n, int k,
                                                 T min_bucket_size = 1);

 private:
  Ref<RandomEngine> engine_;

  // Shuffles so that the first k elements are the prefix of a random shuffle.
  // The last n-k elements are not shuffled, and likely in pseudo-increasing
  // order.
  template <typename Container>
  void PartialShuffle(Container& container, int64_t k);
};

// -----------------------------------------------------------------------------
//  Template implementation below

template <typename Container>
void BasicRandomContext::PartialShuffle(Container& container, int64_t k) {
  std::span c(container);
  for (int i = 0; i < k; i++) {
    int64_t s = RandomInteger(i, c.size() - 1);  // size()-1 is safe
    if (i != s) std::swap(c[i], c[s]);
  }
}

template <typename Container>
void BasicRandomContext::Shuffle(Container& container) {
  PartialShuffle(container, container.size());
}

template <typename Container>
ElementType<Container> BasicRandomContext::RandomElement(
    const Container& container) {
  std::span c(container);
  if (c.empty())
    throw std::invalid_argument("RandomElement() called with empty c.");

  int64_t idx = RandomInteger(c.size());
  return c[idx];
}

template <typename Container>
std::vector<ElementType<Container>>
BasicRandomContext::RandomElementsWithReplacement(const Container& container,
                                                  int k) {
  std::span c(container);
  using T = ElementType<Container>;

  if (k < 0) {
    throw std::invalid_argument(
        std::format("RandomElementsWithReplacement(<container>, {}) is invalid "
                    "(need k >= 0)",
                    k));
  }
  if (c.empty() && k > 0) {
    throw std::invalid_argument(
        std::format("RandomElementsWithReplacement(<empty_container>, {}) is "
                    "invalid (need nonempty container)",
                    k));
  }

  std::vector<T> result;
  result.reserve(k);
  for (int i = 0; i < k; i++) result.push_back(c[RandomInteger(c.size())]);

  return result;
}

template <typename Container>
std::vector<ElementType<Container>>
BasicRandomContext::RandomElementsWithoutReplacement(const Container& container,
                                                     int k) {
  std::span c(container);
  using T = ElementType<Container>;

  if (k < 0) {
    throw std::invalid_argument(
        std::format("RandomElementsWithoutReplacement(<container>, {}) is "
                    "invalid (need k >= 0)",
                    k));
  }
  if (k > c.size()) {
    throw std::invalid_argument(
        std::format("RandomElementsWithReplacement(<container>, {}) is invalid "
                    "since <container>.size() == {} (need k <= size)",
                    k, c.size()));
  }

  std::vector<T> result;
  result.reserve(k);

  auto indices = DistinctIntegers(c.size(), k);
  for (size_t index : indices) result.push_back(c[index]);
  return result;
}

template <std::integral T>
std::vector<T> BasicRandomContext::RandomPermutation(int n, T min) {
  if (n < 0) {
    throw std::invalid_argument(std::format(
        "RandomPermutation({}, {}) is invalid (need n >= 0)", n, min));
  }
  return DistinctIntegers(n, n, min);
}

template <std::integral T>
std::vector<T> BasicRandomContext::DistinctIntegers(T n, int k, T min) {
  if (!(0 <= k && k <= n)) {
    throw std::invalid_argument(std::format(
        "DistinctIntegers({}, {}, {}) is invalid (need 0 <= k <= n)", n, k,
        min));
  }

  // If we are asking for a large percentage of the range, we just generate all
  // values and shuffle that list so we don't have to deal with the (minor)
  // overhead of hash sets, plus the potential pathological cases of the
  // sampling failing many times. 4 is mostly arbitrary.
  if (4 * k >= n) {
    std::vector<T> all;
    all.reserve(n);
    for (int i = 0; i < n; i++) all.push_back(i + min);
    PartialShuffle(all, k);
    all.resize(k);
    return all;
  }

  std::unordered_set<T> sample;
  sample.reserve(k);

  std::vector<T> result;
  result.reserve(k);

  while (result.size() < k) {
    T value = min + RandomInteger(n);
    auto [it, inserted] = sample.insert(value);
    if (inserted) result.push_back(value);
  }

  return result;
}

template <std::integral T>
std::vector<T> BasicRandomContext::RandomComposition(T n, int k,
                                                     T min_bucket_size) {
  if (n == 0 && k == 0 && min_bucket_size == 0) return std::vector<T>();

  if (n < 0 || k <= 0 || min_bucket_size < 0) {
    throw std::invalid_argument(std::format(
        "RandomComposition({}, {}, {}) is invalid (need n >= 0, k > 0, "
        "min_bucket_size >= 0)",
        n, k, min_bucket_size));
  }

  if (min_bucket_size > 0 && n / min_bucket_size < k) {
    throw std::invalid_argument(
        std::format("RandomComposition({}, {}, {}) is impossible: "
                    "min_bucket_size uses more than n elements already",
                    n, k, min_bucket_size));
  }

  if (min_bucket_size > 0) {
    auto result = RandomComposition(n - min_bucket_size * k, k, 0);
    for (T& val : result) val += min_bucket_size;
    return result;
  }
  if (n == 0) return std::vector<T>(k, 0);

  auto barriers = DistinctIntegers(n + (k - 1), k - 1);
  std::sort(barriers.begin(), barriers.end());

  std::vector<T> result;
  result.reserve(k);

  T prev = -1;
  for (T b : barriers) {
    result.push_back(b - prev - 1);
    prev = b;
  }
  result.push_back((n + (k - 1)) - prev - 1);

  return result;
}

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_INTERNAL_BASIC_RANDOM_CONTEXT_H_
