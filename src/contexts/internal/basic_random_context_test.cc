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

#include <map>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/librarian/testing/gtest_helpers.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

using ::moriarty_testing::Context;
using ::testing::AllOf;
using ::testing::AnyOfArray;
using ::testing::Each;
using ::testing::ElementsAre;
using ::testing::Ge;
using ::testing::IsEmpty;
using ::testing::Le;
using ::testing::Lt;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAre;
using ::testing::WhenSorted;

// This is a *massively* simplified check to see if a distribution is
// approximately uniform.
MATCHER_P(IsApproximatelyUniformlyRandom, expected_number_of_buckets, "") {
  // This check is extremely naive. If the expected number of items in a bucket
  // is X, then this accepts as long as the number in each bucket is in the
  // range [X/3, 5*X/3]. (Those bounds were picked somewhat arbitrarily). This
  // moreso tests that each value "appears" than it checks for uniformity. This
  // can be improved to use Chi-Squared for a better test if needed.
  int expected_items_per_bucket = arg.size() / expected_number_of_buckets;
  int expected_lower = expected_items_per_bucket / 3;
  int expected_upper = 5 * expected_items_per_bucket / 3;

  if (expected_lower <= 5) {
    *result_listener
        << "This test is not effective if each bucket contains <= 5 items.";
    return false;
  }

  // arg_type                           == std::vector<Foo>
  // std::decay_t<arg_type>::value_type == Foo
  using sample_type = typename std::decay_t<arg_type>::value_type;
  std::map<sample_type, int> count;
  for (const auto& item : arg) count[item]++;

  if (count.size() != expected_number_of_buckets) {
    *result_listener << "actual number of buckets (" << count.size()
                     << ") is not expected (" << expected_number_of_buckets
                     << ")";
    return false;
  }

  for (const auto& [bucket, num_occurrence] : count) {
    if (num_occurrence < expected_lower || num_occurrence > expected_upper) {
      *result_listener << testing::PrintToString(bucket) << " occurs "
                       << num_occurrence
                       << " times, which is outside the acceptable ["
                       << expected_lower << ", " << expected_upper << "] range";
      return false;
    }
  }

  return true;
}

// -----------------------------------------------------------------------------
//  RandomInteger()

TEST(BasicRandomContextTest, RandomIntegerInTypicalCase) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  EXPECT_EQ(ctx.RandomInteger(314, 314), 314);
  EXPECT_EQ(ctx.RandomInteger(1), 0);
  EXPECT_THAT(ctx.RandomInteger(1, 5), AllOf(Ge(1), Le(5)));
  EXPECT_THAT(ctx.RandomInteger(-10, -5), AllOf(Ge(-10), Le(-5)));
  EXPECT_THAT(ctx.RandomInteger(4), AllOf(Ge(0), Lt(4)));
}

TEST(BasicRandomContextTest, RandomIntegerForInvalidRangesShouldFail) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  EXPECT_THROW({ (void)ctx.RandomInteger(5, 3); }, std::invalid_argument);
  EXPECT_THROW({ (void)ctx.RandomInteger(0); }, std::invalid_argument);
  EXPECT_THROW({ (void)ctx.RandomInteger(-5); }, std::invalid_argument);
}

TEST(BasicRandomContextTest,
     RandomIntegerGivesApproximatelyTheRightDistribution) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  {  // Double argument:
    std::vector<int> samples;
    for (int i = 0; i < 6000; i++) samples.push_back(ctx.RandomInteger(23, 34));
    EXPECT_THAT(samples,
                IsApproximatelyUniformlyRandom(12));  // 12=34-23+1
  }

  {  // Single argument:
    std::vector<int> samples;
    for (int i = 0; i < 6000; i++) samples.push_back(ctx.RandomInteger(13));
    EXPECT_THAT(samples, IsApproximatelyUniformlyRandom(13));
  }
}

// -----------------------------------------------------------------------------
//  RandomReal()

TEST(BasicRandomContextTest, RandomRealWithRealArgumentsInTypicalCase) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  EXPECT_THAT(ctx.RandomReal(Real(3, 2), Real(7, 2)), AllOf(Ge(1.5), Le(3.5)));
  EXPECT_EQ(ctx.RandomReal(Real(5), Real(5)), 5.0);
}

TEST(BasicRandomContextTest, RandomRealWithSingleRealArgumentInTypicalCase) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  EXPECT_THAT(ctx.RandomReal(Real(5, 2)), AllOf(Ge(0.0), Lt(2.5)));
}

TEST(BasicRandomContextTest, RandomRealWithDoubleArgumentsInTypicalCase) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  EXPECT_THAT(ctx.RandomReal(1.5, 3.5), AllOf(Ge(1.5), Le(3.5)));
  EXPECT_EQ(ctx.RandomReal(2.7, 2.7), 2.7);
}

TEST(BasicRandomContextTest, RandomRealWithSingleDoubleArgumentInTypicalCase) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  EXPECT_THAT(ctx.RandomReal(2.5), AllOf(Ge(0.0), Lt(2.5)));
}

TEST(BasicRandomContextTest, RandomRealForInvalidRangesShouldFail) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  // Test Real arguments
  EXPECT_THROW(
      { (void)ctx.RandomReal(Real(5), Real(3)); }, std::invalid_argument);
  EXPECT_THROW({ (void)ctx.RandomReal(Real(-1)); }, std::invalid_argument);

  // Test double arguments
  EXPECT_THROW({ (void)ctx.RandomReal(5.0, 3.0); }, std::invalid_argument);
  EXPECT_THROW({ (void)ctx.RandomReal(-1.0); }, std::invalid_argument);
  EXPECT_THROW({ (void)ctx.RandomReal(0.0); }, std::invalid_argument);
}

TEST(BasicRandomContextTest, RandomRealGivesApproximatelyUniformDistribution) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  {  // Test Real arguments [0.5, 5.3]
    std::vector<int> buckets(4, 0);
    for (int i = 0; i < 6000; i++) {
      double val = ctx.RandomReal(Real(1, 2), Real(53, 10));
      int bucket = [](double x) {
        if (x < 1.7) return 0;  // [0.5, 1.7)
        if (x < 2.9) return 1;  // [1.7, 2.9)
        if (x < 4.1) return 2;  // [2.9, 4.1)
        return 3;               // [4.1, 5.3]
      }(val);
      buckets[bucket]++;
    }

    // Each bucket should have approximately 1500 samples (6000/4)
    for (int count : buckets) {
      EXPECT_THAT(count, AllOf(Ge(1200), Le(1800)));  // Rough uniformity check
    }
  }

  {                                  // Test double arguments [2.0, 6.0]
    std::vector<int> buckets(4, 0);  // 4 buckets: [2,3), [3,4), [4,5), [5,6]
    for (int i = 0; i < 6000; i++) {
      double val = ctx.RandomReal(2.0, 6.0);
      int bucket = static_cast<int>(val - 2.0);
      if (bucket >= 4) bucket = 3;  // Handle edge case for exactly 6.0
      buckets[bucket]++;
    }

    // Each bucket should have approximately 1500 samples (6000/4)
    for (int count : buckets) {
      EXPECT_THAT(count, AllOf(Ge(1200), Le(1800)));  // Rough uniformity check
    }
  }
}

TEST(BasicRandomContextTest, RandomRealSingleArgumentIsInCorrectRange) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  // Test many samples to ensure they're all in [0, n)
  for (int i = 0; i < 1000; i++) {
    EXPECT_THAT(ctx.RandomReal(Real(3, 2)),
                AllOf(Ge(0.0), Lt(1.5)));                       // [0, 1.5)
    EXPECT_THAT(ctx.RandomReal(4.7), AllOf(Ge(0.0), Lt(4.7)));  // [0, 4.7)
  }
}

// -----------------------------------------------------------------------------
//  Shuffle()

TEST(BasicRandomContextTest, ShufflingAnEmptyArrayIsSuccessful) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  std::vector<int> empty;
  ctx.Shuffle(empty);
  EXPECT_THAT(empty, IsEmpty());
}

TEST(BasicRandomContextTest, ShufflePermutesElements) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  // Shuffle integers
  std::vector<int> int_vals = {1, 22, 333, 4444, 55555};
  ctx.Shuffle(int_vals);
  EXPECT_THAT(int_vals, UnorderedElementsAre(1, 22, 333, 4444, 55555));

  // Shuffle strings
  std::vector<std::string> str_vals = {"these", "are", "some", "words"};
  ctx.Shuffle(str_vals);
  EXPECT_THAT(str_vals, UnorderedElementsAre("these", "are", "some", "words"));
}

TEST(BasicRandomContextTest, ShuffleGivesApproximatelyTheRightDistribution) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  std::vector<std::vector<int>> samples;
  std::vector<int> v = {1, 2, 3, 4};
  for (int i = 0; i < 6000; i++) {
    ctx.Shuffle(v);
    samples.push_back(v);
  }
  EXPECT_THAT(samples, IsApproximatelyUniformlyRandom(24));  // 24 = 4!
}

// -----------------------------------------------------------------------------
//  RandomElement()

TEST(BasicRandomContextTest,
     RandomElementGivesApproximatelyTheRightDistribution) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  std::vector<int> samples;
  std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  for (int i = 0; i < 6000; i++) samples.push_back(ctx.RandomElement(v));

  EXPECT_THAT(samples, IsApproximatelyUniformlyRandom(10));
}

TEST(BasicRandomContextTest, RandomElementGivesSomeValueFromArray) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  std::vector<int> v = {123456, 456789, 789123};
  EXPECT_THAT(ctx.RandomElement(v), AnyOfArray(v));
}

TEST(BasicRandomContextTest, RandomElementFromAnEmptyContainerShouldFail) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  std::vector<int> empty;
  EXPECT_THROW({ (void)ctx.RandomElement(empty); }, std::invalid_argument);
}

// -----------------------------------------------------------------------------
//  RandomElements()

TEST(BasicRandomContextTest,
     RandomElementsWithReplacementGivesApproximatelyTheRightDistribution) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  std::vector<std::vector<int>> samples;
  std::vector<int> v = {1, 2, 3, 4, 5};
  for (int i = 0; i < 6000; i++)
    samples.push_back(ctx.RandomElementsWithReplacement(v, 3));

  EXPECT_THAT(samples, IsApproximatelyUniformlyRandom(125));  // 125 = 5^3
}

TEST(BasicRandomContextTest,
     RandomElementsWithoutReplacementGivesApproximatelyTheRightDistribution) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  std::vector<std::vector<int>> samples;
  std::vector<int> v = {1, 2, 3, 4, 5};
  for (int i = 0; i < 6000; i++)
    samples.push_back(ctx.RandomElementsWithoutReplacement(v, 3));

  // 60 = (5 choose 3) * 3!
  EXPECT_THAT(samples, IsApproximatelyUniformlyRandom(60));
}

TEST(BasicRandomContextTest, RandomElementsGivesSomeValuesFromArray) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  std::vector<std::string> v = {"123456", "456789", "789123"};
  EXPECT_THAT(ctx.RandomElementsWithReplacement(v, 5), Each(AnyOfArray(v)));
  EXPECT_THAT(ctx.RandomElementsWithoutReplacement(v, 2), Each(AnyOfArray(v)));
}

TEST(BasicRandomContextTest, RandomElementsObeysDistinctOnlyPolicy) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  std::vector<int> v = {11, 22, 33, 44};

  // Trial 20 times. Each trial has 4/64 chance of false positive.
  for (int i = 0; i < 20; i++) {
    EXPECT_THAT(ctx.RandomElementsWithoutReplacement(v, 3),
                WhenSorted(AnyOfArray<std::vector<int>>(
                    {{11, 22, 33}, {11, 33, 44}, {11, 22, 44}, {22, 33, 44}})));
  }
}

TEST(BasicRandomContextTest,
     RandomElementsWithReplacementCanContainDuplicates) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  std::vector<int> v = {11, 22, 33, 44};

  // Only 4 elements and I'm querying for 6. Duplicates must be there!
  // Not crashing is the test.
  (void)ctx.RandomElementsWithReplacement(v, 6);
}

TEST(BasicRandomContextTest, RandomElementsWithNegativeInputShouldFail) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  std::vector<int> v = {11, 22, 33, 44};

  EXPECT_THROW(
      { (void)ctx.RandomElementsWithReplacement(v, -1); },
      std::invalid_argument);
  EXPECT_THROW(
      { (void)ctx.RandomElementsWithoutReplacement(v, -1); },
      std::invalid_argument);
}

TEST(BasicRandomContextTest, RandomElementsFromAnEmptyContainerShouldFail) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  std::vector<std::string> empty;
  EXPECT_THROW(
      { (void)ctx.RandomElementsWithReplacement(empty, 1); },
      std::invalid_argument);
  EXPECT_THROW(
      { (void)ctx.RandomElementsWithoutReplacement(empty, 1); },
      std::invalid_argument);
}

TEST(BasicRandomContextTest,
     RandomElementsFromAnEmptyContainerShouldReturnNothing) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  std::vector<std::string> empty;
  EXPECT_THAT(ctx.RandomElementsWithReplacement(empty, 0), IsEmpty());
  EXPECT_THAT(ctx.RandomElementsWithoutReplacement(empty, 0), IsEmpty());
}

TEST(BasicRandomContextTest, RandomElementsAskingForTooManyDistinctShouldFail) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  std::vector<std::string> v = {"123456", "456789", "789123"};
  EXPECT_THROW(
      { (void)ctx.RandomElementsWithoutReplacement(v, 4); },
      std::invalid_argument);
}

// -----------------------------------------------------------------------------
//  RandomPermutation()

TEST(BasicRandomContextTest, RandomPermutationOfNegativeSizeShouldFail) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  EXPECT_THROW({ (void)ctx.RandomPermutation(-1); }, std::invalid_argument);
}

TEST(BasicRandomContextTest, RandomPermutationOfSizeZeroShouldBeEmpty) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  EXPECT_THAT(ctx.RandomPermutation(0), IsEmpty());
}

TEST(BasicRandomContextTest, RandomPermutationGeneratesAPermutation) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  EXPECT_THAT(ctx.RandomPermutation(5), UnorderedElementsAre(0, 1, 2, 3, 4));
  EXPECT_THAT(ctx.RandomPermutation(5, 10),
              UnorderedElementsAre(10, 11, 12, 13, 14));
}

TEST(BasicRandomContextTest, RandomPermutationGivesPermutation) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  EXPECT_THAT(ctx.RandomPermutation(0), IsEmpty());
  EXPECT_THAT(ctx.RandomPermutation(1), UnorderedElementsAre(0));
  EXPECT_THAT(ctx.RandomPermutation(4), UnorderedElementsAre(0, 1, 2, 3));
  EXPECT_THAT(ctx.RandomPermutation(6), UnorderedElementsAre(0, 1, 2, 3, 4, 5));
}

TEST(BasicRandomContextTest,
     RandomPermutationGivesApproximatelyCorrectDistribution) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  std::vector<std::vector<int>> samples;
  for (int i = 0; i < 6000; i++) samples.push_back(ctx.RandomPermutation(3));

  EXPECT_THAT(samples, IsApproximatelyUniformlyRandom(6));  // 6 = 3!
}

// -----------------------------------------------------------------------------
//  DistinctIntegers()

TEST(BasicRandomContextTest,
     DistinctIntegersGivesApproximatelyTheRightDistribution) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  std::vector<std::vector<int>> samples;
  for (int i = 0; i < 6000; i++) samples.push_back(ctx.DistinctIntegers(8, 2));

  EXPECT_THAT(samples, IsApproximatelyUniformlyRandom(56));  // 56 = 8 * 7
}

TEST(BasicRandomContextTest, DistinctIntegersWithNegativeNOrKShouldFail) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  EXPECT_THROW({ (void)ctx.DistinctIntegers(-1, 5); }, std::invalid_argument);
  EXPECT_THROW({ (void)ctx.DistinctIntegers(10, -5); }, std::invalid_argument);
}

TEST(BasicRandomContextTest, DistinctIntegersWithKGreaterThanNShouldFail) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  EXPECT_THROW({ (void)ctx.DistinctIntegers(5, 10); }, std::invalid_argument);
}

TEST(BasicRandomContextTest, DistinctIntegersWithZeroIsFine) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  EXPECT_THAT(ctx.DistinctIntegers(0, 0), IsEmpty());
  EXPECT_THAT(ctx.DistinctIntegers(5, 0), IsEmpty());
}

TEST(BasicRandomContextTest, DistinctIntegersDoesNotGiveDuplicates) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  EXPECT_THAT(ctx.DistinctIntegers(10, 10),
              UnorderedElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8, 9));

  std::vector<int> vals = ctx.DistinctIntegers(50, 30);
  EXPECT_THAT(std::set<int>(vals.begin(), vals.end()), SizeIs(30));
}

TEST(BasicRandomContextTest, DistinctIntegersOffsetWorksAppropriately) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  EXPECT_THAT(ctx.DistinctIntegers(10000, 100, 1234),
              Each(AllOf(Ge(1234), Le(10000 + 1234))));
}

// -----------------------------------------------------------------------------
//  RandomComposition()

TEST(BasicRandomContextTest,
     RandomCompositionGivesApproximatelyCorrectDistribution) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  std::vector<std::vector<int>> samples;
  for (int i = 0; i < 6000; i++) samples.push_back(ctx.RandomComposition(5, 3));

  // (1, 1, 3), (1, 2, 2), (1, 3, 1), (2, 1, 2), (2, 2, 1), (3, 1, 1)
  EXPECT_THAT(samples, IsApproximatelyUniformlyRandom(6));
}

TEST(BasicRandomContextTest, RandomCompositionWithNegativeArgumentShouldFail) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  EXPECT_THROW({ (void)ctx.RandomComposition(-1, 5); }, std::invalid_argument);
  EXPECT_THROW({ (void)ctx.RandomComposition(10, -5); }, std::invalid_argument);
  EXPECT_THROW(
      { (void)ctx.RandomComposition(10, 5, -1); }, std::invalid_argument);
}

TEST(BasicRandomContextTest, RandomCompositionWithZeroBucketsAndZeroSumIsFine) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  EXPECT_THAT(ctx.RandomComposition(0, 0, 0), IsEmpty());
}

TEST(BasicRandomContextTest,
     RandomCompositionWithZeroBucketsAndNonZeroSumFails) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  EXPECT_THROW({ (void)ctx.RandomComposition(5, 0); }, std::invalid_argument);
}

TEST(BasicRandomContextTest, RandomCompositionWithTooLargeMinBucketShouldFail) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  EXPECT_THROW(
      { (void)ctx.RandomComposition(4, 2, 3); }, std::invalid_argument);
}

TEST(BasicRandomContextTest, RandomCompositionWithExactMinBucketIsFine) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  EXPECT_THAT(ctx.RandomComposition(10, 5, 2), ElementsAre(2, 2, 2, 2, 2));
}

TEST(BasicRandomContextTest, RandomCompositionRespectsMinBucketSize) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  EXPECT_THAT(ctx.RandomComposition(40, 6, 2), Each(Ge(2)));
}

TEST(BasicRandomContextTest, RandomCompositionReturnsTheRightNumberOfBuckets) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  EXPECT_THAT(ctx.RandomComposition(40, 6, 2), SizeIs(6));
  EXPECT_THAT(ctx.RandomComposition(400, 3, 0), SizeIs(3));
}

TEST(BasicRandomContextTest, RandomCompositionHasCorrectTotalSum) {
  Context test_ctx;
  BasicRandomContext ctx(test_ctx.RandomEngine());

  auto sum = [](const std::vector<int>& v) {
    return std::accumulate(v.begin(), v.end(), 0);
  };

  EXPECT_EQ(sum(ctx.RandomComposition(40, 6, 2)), 40);
  EXPECT_EQ(sum(ctx.RandomComposition(400, 3, 0)), 400);
  EXPECT_EQ(sum(ctx.RandomComposition(555, 10)), 555);
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
