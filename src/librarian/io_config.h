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

#ifndef MORIARTY_SRC_LIBRARIAN_IO_CONFIG_H_
#define MORIARTY_SRC_LIBRARIAN_IO_CONFIG_H_

#include <optional>
#include <string>

namespace moriarty {

// Various kinds of whitespace characters.
enum class Whitespace { kSpace, kTab, kNewline };

// Information about where the input cursor is in the input stream.
struct InputCursor {
  int line_num = 1;
  int col_num = 1;
  int token_num_file = 1;
  int token_num_line = 1;
  std::optional<std::string> last_read_item;  // May be a token or whitespace.
};

}  // namespace moriarty

#endif  // MORIARTY_SRC_LIBRARIAN_IO_CONFIG_H_
