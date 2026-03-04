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

#ifndef MORIARTY_INTERNAL_VALUE_PRINTER_H_
#define MORIARTY_INTERNAL_VALUE_PRINTER_H_

#include <concepts>
#include <format>
#include <functional>
#include <optional>
#include <ostream>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace moriarty {
namespace moriarty_internal {

// Note that in all functions below, the max length is a soft limit.
// We aim to keep under it, but it's not a big deal to slightly exceed it.

template <typename T>
std::string InternalValuePrinter(const T& value, int max_len);

template <typename T>
std::string ValuePrinter(const T& value, int max_len = 100) {
  return InternalValuePrinter(value, max_len);
}

template <typename GetFn>
std::string InternalValuePrinterGenericContainer(int n, int max_len,
                                                 int min_count, GetFn get_fn) {
  if (n == 0) return "";
  min_count = std::min(min_count, n);
  int overhead = 2;  // for ", " separators

  auto get_prefix = [&](int budget_per_item,
                        int limit_count) -> std::vector<std::string> {
    std::vector<std::string> parts;
    int current_len = 0;
    for (int i = 0; i < limit_count; i++) {
      std::string s = get_fn(i, budget_per_item);
      int added_len = s.size() + (i > 0 ? overhead : 0);

      if (current_len + added_len > max_len) break;
      parts.push_back(s);
      current_len += added_len;
    }
    return parts;
  };

  std::vector<std::string> parts = get_prefix(max_len, n);

  if (parts.size() < min_count) {
    // Force regeneration with distributed budget
    parts.clear();
    int budget = std::max(1, max_len / min_count);
    for (int i = 0; i < min_count; i++) parts.push_back(get_fn(i, budget));
  }

  std::string result;
  for (int i = 0; i < parts.size(); i++) {
    if (i > 0) result += ", ";
    result += parts[i];
  }
  if (parts.size() < n) result += ", ...";
  return result;
}

template <std::integral IntLike>
std::string InternalValuePrinterInt(const IntLike& value) {
  using T = std::remove_cvref_t<IntLike>;
  constexpr bool is_char = std::is_same_v<T, char> ||
                           std::is_same_v<T, unsigned char> ||
                           std::is_same_v<T, signed char>;
  if constexpr (is_char) {
    if (std::isprint(static_cast<unsigned char>(value))) {
      return std::format("'{}'", static_cast<char>(value));
    } else {
      return std::format("ASCII {}", static_cast<int>(value));
    }
  }
  return std::format("{}", value);
}

template <std::floating_point FloatLike>
std::string InternalValuePrinterFloat(const FloatLike& value) {
  return std::format("{}", value);
}

std::string InternalValuePrinterString(std::string_view value, int max_len) {
  max_len -= 2;                              // for the quotes
  if (value.size() > max_len) max_len -= 3;  // for the "..."
  max_len = std::max(max_len, 2);  // Always show at least 2 characters
  return std::format("\"{}{}\"", value.substr(0, max_len),
                     value.size() > max_len ? "..." : "");
}

template <typename T>
std::string InternalValuePrinterArray(const T& array, int max_len) {
  if (array.empty()) return "[]";
  max_len -= 2;  // for []

  std::string result = InternalValuePrinterGenericContainer(
      array.size(), max_len, 3, [&](int i, int budget) {
        return InternalValuePrinter(array[i], budget);
      });
  return "[" + result + "]";
}

template <typename T>
std::string InternalValuePrinterOptional(const std::optional<T>& opt,
                                         int max_len) {
  if (!opt) return "(empty optional)";
  return InternalValuePrinter(*opt, max_len);
}

template <typename... Args>
std::string InternalValuePrinterVariant(const std::variant<Args...>& v,
                                        int max_len) {
  return std::visit(
      [max_len](const auto& arg) { return InternalValuePrinter(arg, max_len); },
      v);
}

template <typename... Args>
std::string InternalValuePrinterTuple(const std::tuple<Args...>& t,
                                      int max_len) {
  max_len -= 2;  // for ()
  std::vector<std::function<std::string(int)>> printers;
  std::apply(
      [&](const auto&... args) {
        (printers.push_back(
             [&args](int b) { return InternalValuePrinter(args, b); }),
         ...);
      },
      t);

  std::string result = InternalValuePrinterGenericContainer(
      printers.size(), max_len, 3,
      [&](int i, int budget) { return printers[i](budget); });

  return "(" + result + ")";
}

template <typename A, typename B>
std::string InternalValuePrinterTuple(const std::pair<A, B>& p, int max_len) {
  max_len -= 2;  // for ()
  std::vector<std::function<std::string(int)>> printers;
  printers.push_back([&p](int b) { return InternalValuePrinter(p.first, b); });
  printers.push_back([&p](int b) { return InternalValuePrinter(p.second, b); });

  std::string result = InternalValuePrinterGenericContainer(
      printers.size(), max_len, 3,
      [&](int i, int budget) { return printers[i](budget); });

  return "(" + result + ")";
}

template <typename T, template <typename...> class Template>
struct is_specialization_of : std::false_type {};

template <template <typename...> class Template, typename... Args>
struct is_specialization_of<Template<Args...>, Template> : std::true_type {};

template <typename T>
concept OptionalType = is_specialization_of<T, std::optional>::value;

template <typename T>
concept VariantType = is_specialization_of<T, std::variant>::value;

template <typename T>
concept TupleType = is_specialization_of<T, std::tuple>::value;

template <typename T>
concept PairType = is_specialization_of<T, std::pair>::value;

template <typename T>
concept AdlPrettyPrintable = requires(const T& value, int max_len) {
  { PrettyPrintValue(value, max_len) } -> std::convertible_to<std::string>;
};

template <typename>
inline constexpr bool kDependentFalse = false;

template <typename T>
std::string InternalValuePrinter(const T& value, int max_len) {
  using U = std::remove_cvref_t<T>;
  if constexpr (std::integral<U>) {
    return InternalValuePrinterInt(value);
  } else if constexpr (std::floating_point<U>) {
    return InternalValuePrinterFloat(value);
  } else if constexpr (std::convertible_to<U, std::string_view>) {
    return InternalValuePrinterString(value, max_len);
  } else if constexpr (OptionalType<U>) {
    return InternalValuePrinterOptional(value, max_len);
  } else if constexpr (VariantType<U>) {
    return InternalValuePrinterVariant(value, max_len);
  } else if constexpr (TupleType<U>) {
    return InternalValuePrinterTuple(value, max_len);
  } else if constexpr (PairType<U>) {
    return InternalValuePrinterTuple(value, max_len);
  } else if constexpr (AdlPrettyPrintable<U>) {
    return PrettyPrintValue(value, max_len);
  } else if constexpr (std::ranges::range<U> && requires(const U& container) {
                         { container.empty() } -> std::convertible_to<bool>;
                         container.size();
                         container[0];
                       }) {
    return InternalValuePrinterArray(value, max_len);
  } else {
    static_assert(kDependentFalse<U>,
                  "ValuePrinter does not know how to pretty-print "
                  "this type");
  }
}

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_INTERNAL_VALUE_PRINTER_H_
