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

#ifndef MORIARTY_SRC_TYPES_NO_TYPE_H_
#define MORIARTY_SRC_TYPES_NO_TYPE_H_

#include <compare>
#include <functional>

namespace moriarty {

// NoType
//
// An empty type.
struct NoType {
  friend std::strong_ordering operator<=>(const NoType&,
                                          const NoType&) = default;
  friend bool operator==(const NoType&, const NoType&) = default;
};

}  // namespace moriarty

namespace std {
template <>
struct hash<moriarty::NoType> {
  size_t operator()(const moriarty::NoType& nt) const noexcept { return 0; }
};
}  // namespace std

#endif  // MORIARTY_SRC_TYPES_NO_TYPE_H_
