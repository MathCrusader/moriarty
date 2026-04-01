// Copyright 2026 Darcy Best
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

#ifndef MORIARTY_CONTEXTS_INTERNAL_ARG_CONTEXT_H_
#define MORIARTY_CONTEXTS_INTERNAL_ARG_CONTEXT_H_

#include <charconv>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <utility>

#include "moriarty/librarian/errors.h"
#include "moriarty/librarian/policies.h"
#include "moriarty/librarian/util/ref.h"

namespace moriarty {
namespace moriarty_internal {

// ArgContext
//
// A class to handle Moriarty type-specific arguments.
class ArgContext {
 public:
  explicit ArgContext(
      Ref<const std::unordered_map<std::string, std::string>> args);

  template <typename T>
  [[nodiscard]] T Arg(std::string_view name) const;

 private:
  Ref<const std::unordered_map<std::string, std::string>> args_;
};

// --------------------------------------------------------------------------
//  Template implementation below

template <typename T>
T ArgContext::Arg(std::string_view name) const {
  static_assert(std::integral<T> || std::floating_point<T> ||
                    std::convertible_to<T, std::string>,
                "Arg<T>(var) only supports numeric types and strings.");

  const auto& args = args_.get();
  auto it = args.find(std::string(name));
  if (it == args.end()) {
    throw GenerationError(std::format("Generator argument not found: {}", name),
                          RetryPolicy::kAbort);
  }

  auto parse_or_throw = [&name]<typename U>(std::string_view sv) -> U {
    U value{};
    auto result = std::from_chars(sv.data(), sv.data() + sv.size(), value);
    if (result.ec != std::errc() || result.ptr != sv.data() + sv.size()) {
      throw GenerationError(std::format("Generator argument '{}' does not "
                                        "parse to the requested type: {}",
                                        name, sv),
                            RetryPolicy::kAbort);
    }

    return value;
  };

  if constexpr (std::convertible_to<T, std::string>) {
    return std::string(it->second);
  }

  if constexpr (std::floating_point<T>) {
    return parse_or_throw.template operator()<double>(it->second);
  }

  if constexpr (std::signed_integral<T>) {
    int64_t value = parse_or_throw.template operator()<int64_t>(it->second);
    if (!std::in_range<T>(value)) {
      throw GenerationError(std::format("Generator argument '{}' does not fit "
                                        "in requested integer type: {}",
                                        name, it->second),
                            RetryPolicy::kAbort);
    }
    return static_cast<T>(value);
  }

  if constexpr (std::unsigned_integral<T>) {
    uint64_t value = parse_or_throw.template operator()<uint64_t>(it->second);
    if (!std::in_range<T>(value)) {
      throw GenerationError(std::format("Generator argument '{}' does not fit "
                                        "in requested integer type: {}",
                                        name, it->second),
                            RetryPolicy::kAbort);
    }
    return static_cast<T>(value);
  }

  throw GenerationError(
      std::format("Unsupported type for generator argument '{}': {}", name,
                  typeid(T).name()),
      RetryPolicy::kAbort);
}

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_CONTEXTS_INTERNAL_ARG_CONTEXT_H_
