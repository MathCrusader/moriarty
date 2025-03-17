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

#include "src/librarian/debug_print.h"

#include <format>

namespace moriarty {
namespace librarian {

std::string DebugString(const std::string& x, int max_len,
                        bool include_backticks) {
  return ShortenDebugString(x, max_len, include_backticks);
}

std::string ShortenDebugString(const std::string& x, int max_len,
                               bool include_backticks) {
  auto do_it = [&]() {
    if (x.length() <= max_len) return std::format("{}", x);
    int left_half = (max_len - 3 + 1) / 2;
    int right_half = (max_len - 3) / 2;
    return std::format("{}...{}", x.substr(0, left_half),
                       x.substr(x.length() - right_half));
  };
  if (include_backticks) return std::format("`{}`", do_it());
  return do_it();
}

}  // namespace librarian
}  // namespace moriarty
