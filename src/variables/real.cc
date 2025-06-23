#include "src/variables/real.h"

#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace moriarty {

namespace {

void Reduce(int64_t& num, int64_t& den) {
  if (den == 0) throw std::invalid_argument("division by zero");

  // abs(INT_MIN) is not representable as a int64_t, so we need to
  // handle it specially to avoid undefined behavior.
  uint64_t safe_num =
      num == std::numeric_limits<int64_t>::min()
          ? static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1
          : std::abs(num);
  uint64_t safe_den =
      den == std::numeric_limits<int64_t>::min()
          ? static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1
          : std::abs(den);
  uint64_t gcd = std::gcd(safe_num, safe_den);
  safe_num /= gcd;
  safe_den /= gcd;

  if ((num < 0) == (den < 0)) {
    if (std::max(safe_num, safe_den) ==
        static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1) {
      throw std::invalid_argument("cannot represent fraction without overflow");
    }
    num = static_cast<int64_t>(safe_num);
    den = static_cast<int64_t>(safe_den);
  } else {
    if (safe_num ==
        static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1) {
      num = std::numeric_limits<int64_t>::min();
    } else {
      num = -static_cast<int64_t>(safe_num);
    }
    den = static_cast<int64_t>(safe_den);
  }
}

}  // namespace

Real::Real(int64_t numerator, int64_t denominator)
    : numerator_(numerator), denominator_(denominator) {
  Reduce(numerator_, denominator_);
}

namespace {

// ^[+-]?[0-9]+(\.[0-9]*)?([eE][+-]?[0-9]+)?$
struct ParsedReal {
  bool negate;
  std::string_view digits;
  std::string_view decimal_part;
  bool negate_exponent;
  std::string_view exponent_digits;
};

std::optional<ParsedReal> ParseRealSyntax(std::string_view str) {
  ParsedReal result = {.negate = false,
                       .digits = {},
                       .decimal_part = {},
                       .negate_exponent = false,
                       .exponent_digits = {}};

  size_t i = 0;

  if (i < str.size() && (str[i] == '+' || str[i] == '-')) {
    result.negate = (str[i] == '-');
    i++;
  }

  int start_idx = i;
  while (i < str.size() && std::isdigit(str[i])) i++;
  result.digits = str.substr(start_idx, i - start_idx);

  if (i < str.size() && str[i] == '.') {
    i++;
    start_idx = i;
    while (i < str.size() && std::isdigit(str[i])) i++;
    result.decimal_part = str.substr(start_idx, i - start_idx);
  }

  if (result.digits.empty() && result.decimal_part.empty()) return std::nullopt;

  if (i < str.size() && (str[i] == 'e' || str[i] == 'E')) {
    i++;
    if (i < str.size() && (str[i] == '+' || str[i] == '-')) {
      result.negate_exponent = (str[i] == '-');
      i++;
    }
    start_idx = i;
    while (i < str.size() && std::isdigit(str[i])) i++;
    result.exponent_digits = str.substr(start_idx, i - start_idx);
    if (i == start_idx) {
      // If we reached here, it means there was no exponent value, but there was
      // an 'e' or 'E'.
      return std::nullopt;
    }
  }

  // If we didn't consume the entire string, it's not a valid real number.
  if (i != str.size()) return std::nullopt;
  return result;
}

std::optional<std::pair<int64_t, int64_t>> ParseReal(std::string_view str) {
  std::optional<ParsedReal> parsed = ParseRealSyntax(str);
  if (!parsed) return std::nullopt;
  if (parsed->digits.empty() && parsed->decimal_part.empty()) {
    return std::nullopt;  // Invalid real number.
  }

  // Remove leading zeros from digits and trailing zeros from decimal part.
  while (!parsed->digits.empty() && parsed->digits.front() == '0') {
    parsed->digits.remove_prefix(1);
  }
  while (!parsed->decimal_part.empty() && parsed->decimal_part.back() == '0') {
    parsed->decimal_part.remove_suffix(1);
  }
  if (parsed->digits.empty() && parsed->decimal_part.empty()) {
    // There were 0's before, but not now. Valid.
    return std::make_pair<int64_t, int64_t>(0, 1);
  }

  if (parsed->digits.size() + parsed->decimal_part.size() > 19)
    return std::nullopt;  // Guaranteed overflow

  while (parsed->exponent_digits.size() > 1 &&
         parsed->exponent_digits.front() == '0') {
    parsed->exponent_digits.remove_prefix(1);
  }
  if (parsed->exponent_digits.size() > 2)
    return std::nullopt;  // Too large exponent. Anything over 20 is too much.

  int64_t exp_value = parsed->exponent_digits.empty()
                          ? 0
                          : std::stoll(std::string(parsed->exponent_digits));
  exp_value = parsed->negate_exponent ? -exp_value : exp_value;
  exp_value -= parsed->decimal_part.size();

  std::string base_digits =
      std::string(parsed->digits) + std::string(parsed->decimal_part);
  if (exp_value > 0) {
    base_digits += std::string(exp_value, '0');
    exp_value = 0;  // No exponent left, we just added zeros.
  }
  if (base_digits.size() > 19) return std::nullopt;  // Guaranteed overflow
  uint64_t base_value = std::stoull(base_digits);

  if (exp_value < -18) return std::nullopt;  // Guaranteed overflow
  int64_t denominator = 1;
  while (exp_value < 0) {
    denominator *= 10;
    exp_value++;
  }

  if (parsed->negate &&
      base_value ==
          static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1)
    return std::make_pair(std::numeric_limits<int64_t>::min(), denominator);

  if (base_value > std::numeric_limits<int64_t>::max()) {
    return std::nullopt;  // Overflow
  }
  int64_t numerator = parsed->negate ? -static_cast<int64_t>(base_value)
                                     : static_cast<int64_t>(base_value);

  return std::make_pair(numerator, denominator);
}

}  // namespace

Real::Real(std::string_view value) {
  auto parsed = ParseReal(value);
  if (!parsed) {
    throw std::invalid_argument("Real(): Invalid real number format: " +
                                std::string(value));
  }
  numerator_ = parsed->first;
  denominator_ = parsed->second;
  Reduce(numerator_, denominator_);
}

double Real::GetApproxValue() const {
  return static_cast<double>(numerator_) / denominator_;
}

}  // namespace moriarty
