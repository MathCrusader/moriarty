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

#include "src/internal/random_engine.h"

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

using ::testing::AllOf;
using ::testing::Ge;
using ::testing::Le;
using ::testing::Throws;

TEST(RandomEngineTest, RandIntWithEmptySeedShouldReturnValue) {
  RandomEngine random({}, "v0.1");

  EXPECT_THAT(random.RandInt(10), AllOf(Ge(0), Le(9)));
  EXPECT_THAT(random.RandInt(5, 20), AllOf(Ge(5), Le(20)));
  EXPECT_THAT(random.RandInt(-120, -50), AllOf(Ge(-120), Le(-50)));
}

TEST(RandomEngineTest, RandIntWithIntMaxReturnsAValue) {
  RandomEngine random({}, "v0.1");

  EXPECT_THAT(random.RandInt(std::numeric_limits<int64_t>::max()), Ge(0));

  // Not crash = pass.
  random.RandInt(std::numeric_limits<int64_t>::min(),
                 std::numeric_limits<int64_t>::max());
}

TEST(RandomEngineTest, RandomShouldProduceReproducibleResults) {
  RandomEngine random({1, 117, 1337}, "v0.1");

  // We ensure consistency by doing a simple hash on the first 10,000
  // invocations of RandInt with this specific seed with these specific upper
  // bounds that pass over at least one power of two.
  int64_t hash = 0;
  for (int i = 0; i < 5000; i++) {
    int64_t val = random.RandInt((1LL << 60) - 1 + i);
    hash = (hash << 5) ^ val;
  }
  for (int i = 0; i < 5000; i++) {
    int64_t val = random.RandInt(-(1LL << 60) + 1, (1LL << 60) + i);
    hash = (hash << 5) ^ val;
  }

  EXPECT_EQ(hash, 6899628773667041596LL);
}

TEST(RandomEngineTest, RandIntWithOneArgumentWithNonpositiveShouldThrow) {
  RandomEngine random({}, "v0.1");

  EXPECT_THAT([&] { random.RandInt(0); }, Throws<std::invalid_argument>());
  EXPECT_THAT([&] { random.RandInt(-3); }, Throws<std::invalid_argument>());
}

TEST(RandomEngineTest, RandIntWithTwoInvalidArgumentsShouldThrow) {
  RandomEngine random({}, "v0.1");

  EXPECT_THAT([&] { random.RandInt(0, -1); }, Throws<std::invalid_argument>());
  EXPECT_THAT([&] { random.RandInt(-3, -50); },
              Throws<std::invalid_argument>());
  EXPECT_THAT(
      [&] {
        random.RandInt(std::numeric_limits<int64_t>::max(),
                       std::numeric_limits<int64_t>::min());
      },
      Throws<std::invalid_argument>());
}

std::vector<int64_t> GetNRandomNumbersUnderK(RandomEngine rand, int N,
                                             int64_t K) {
  std::vector<int64_t> values(N);
  for (int64_t& val : values) val = rand.RandInt(K);
  return values;
}

TEST(RandomEngineTest, DifferentSeedsShouldProduceDifferentResults) {
  // This has a (1/123456)^10 chance of being equal.
  std::vector<int64_t> values1 =
      GetNRandomNumbersUnderK(RandomEngine({1, 2, 3}, "v0.1"), 10, 123456);
  std::vector<int64_t> values2 =
      GetNRandomNumbersUnderK(RandomEngine({2, 3, 5}, "v0.1"), 10, 123456);

  EXPECT_NE(values1, values2);
}

TEST(RandomEngineTest, SameSeedsProduceTheSameResults) {
  // This has a (1/123456)^10 chance of passing when it shouldn't.
  std::vector<int64_t> values1 = GetNRandomNumbersUnderK(
      RandomEngine({123, 456, 789}, "v0.1"), 10, 123456);
  std::vector<int64_t> values2 = GetNRandomNumbersUnderK(
      RandomEngine({123, 456, 789}, "v0.1"), 10, 123456);

  EXPECT_EQ(values1, values2);
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
