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
//
// * Precise means that the whitespace must be exactly as expected.
// * Flexible treats all whitespace characters as the same, and collapses all
//   back-to-back whitespace together.
enum class WhitespaceStrictness { kPrecise, kFlexible };

// Determines if a generation should be retried or not.
enum class RetryPolicy { kRetry, kAbort };

}  // namespace moriarty

#endif  // MORIARTY_SRC_LIBRARIAN_POLICIES_H_
