// Copyright 2025 Darcy Best
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

#ifndef MORIARTY_SRC_LIBRARIAN_IO_CONFIG_H_
#define MORIARTY_SRC_LIBRARIAN_IO_CONFIG_H_

#include <deque>
#include <istream>
#include <string>

#include "src/librarian/policies.h"
#include "src/librarian/util/ref.h"

namespace moriarty {

// Various kinds of whitespace characters.
enum class Whitespace { kSpace, kTab, kNewline };

// Information about where the input cursor is in the input stream.
struct InputCursor {
  static constexpr int kRecentlyReadSize = 3;

  // Creates an InputCursor with the strictest settings.
  static InputCursor CreatePreciseStrictness(Ref<std::istream> is) {
    return InputCursor(is, WhitespaceStrictness::kPrecise,
                       NumericStrictness::kPrecise);
  }

  // Creates an InputCursor with the most flexible settings.
  static InputCursor CreateFlexibleStrictness(Ref<std::istream> is) {
    return InputCursor(is, WhitespaceStrictness::kFlexible,
                       NumericStrictness::kFlexible);
  }

  Ref<std::istream> is;
  WhitespaceStrictness whitespace_strictness = WhitespaceStrictness::kPrecise;
  NumericStrictness numeric_strictness = NumericStrictness::kPrecise;
  int line_num = 1;                       // 1-based
  int col_num = 0;                        // 1-based
  int token_num_file = 0;                 // 1-based
  int token_num_line = 0;                 // 1-based
  std::deque<std::string> recently_read;  // May be tokens or whitespace.

  void AddReadItem(std::string item) {
    recently_read.push_back(std::move(item));
    if (recently_read.size() > kRecentlyReadSize) recently_read.pop_front();
  }
};

}  // namespace moriarty

#endif  // MORIARTY_SRC_LIBRARIAN_IO_CONFIG_H_
