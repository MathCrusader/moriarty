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

#ifndef MORIARTY_SRC_LIBRARIAN_DEBUG_STRING_H_
#define MORIARTY_SRC_LIBRARIAN_DEBUG_STRING_H_

#include <concepts>
#include <string>
#include <tuple>
#include <vector>

namespace moriarty {
namespace librarian {

// Values <10 may be weird, I wasn't careful for small values.
static constexpr int kMaxDebugStringLength = 50;

template <std::integral IntLike>
std::string DebugString(IntLike x, int max_len = kMaxDebugStringLength,
                        bool include_backticks = true);

template <typename T>
std::string DebugString(const std::vector<T>& x,
                        int max_len = kMaxDebugStringLength,
                        bool include_backticks = true);

template <typename... Ts>
std::string DebugString(const std::tuple<Ts...>& x,
                        int max_len = kMaxDebugStringLength,
                        bool include_backticks = true);

std::string DebugString(const std::string& x,
                        int max_len = kMaxDebugStringLength,
                        bool include_backticks = true);

std::string ShortenDebugString(const std::string& x, int max_len,
                               bool include_backticks);

// ----------------------------------------------------------------------------

template <std::integral IntLike>
std::string DebugString(IntLike x, int max_len, bool include_backticks) {
  return ShortenDebugString(std::to_string(x), max_len, include_backticks);
}

template <typename T>
std::string DebugString(const std::vector<T>& x, int max_len,
                        bool include_backticks) {
  std::string res = "[";
  for (bool first = true; const auto& elem : x) {
    if (!first) res += ",";
    first = false;
    res += DebugString(elem, max_len, false);
  }
  return ShortenDebugString(res + "]", max_len, include_backticks);
}

template <typename... Ts>
std::string DebugString(const std::tuple<Ts...>& x, int max_len,
                        bool include_backticks) {
  std::string res = "<";
  std::apply(
      [&](const auto&... elems) {
        bool first = true;
        ((res += (first ? "" : ",") + DebugString(elems, max_len, false),
          first = false),
         ...);
      },
      x);
  return ShortenDebugString(res + ">", max_len, include_backticks);
}

}  // namespace librarian
}  // namespace moriarty

#endif  // MORIARTY_SRC_LIBRARIAN_DEBUG_STRING_H_
