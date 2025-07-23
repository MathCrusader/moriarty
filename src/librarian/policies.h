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

#ifndef MORIARTY_SRC_LIBRARIAN_POLICIES_H_
#define MORIARTY_SRC_LIBRARIAN_POLICIES_H_

namespace moriarty {

// How to handle whitespace when reading from an input stream.
enum class WhitespaceStrictness {
  // The whitespace must be exactly as expected.
  kPrecise,
  // All whitespace characters are treated as the same, and back-to-back
  // whitespace is collapsed.
  kFlexible
};

// How strict the precision of a number should be when reading from an input
// stream.
enum class NumericStrictness {
  // Ensures the following:
  //  * No leading '+'.
  //  * No -0.
  //  * No inf/nan.
  //  * No unnecessary leading zeroes.
  //  * No exponential notation.
  //  * The number of digits after the decimal point is exactly as specified.
  kPrecise,
  // No restriction on the number. Calls std::from_chars(), but allows leading
  // '+'.
  kFlexible
};

// Determines if a generation should be retried or not.
enum class RetryPolicy { kRetry, kAbort };

}  // namespace moriarty

#endif  // MORIARTY_SRC_LIBRARIAN_POLICIES_H_
