// Copyright 2024 Google LLC
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

#include "src/internal/range.h"

#include <algorithm>
#include <cstdint>
#include <format>
#include <functional>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "absl/strings/str_join.h"
#include "src/internal/expressions.h"
#include "src/types/real.h"

namespace moriarty {

Range& Range::AtLeast(int64_t minimum) {
  min_int_ = std::max(min_int_, minimum);
  return *this;
}

Range& Range::AtLeast(Expression minimum) {
  min_exprs_.push_back(std::move(minimum));
  return *this;
}

Range& Range::AtLeast(const Real& minimum) {
  min_real_ = min_real_ ? std::max(*min_real_, minimum) : minimum;
  return *this;
}

Range& Range::AtMost(int64_t maximum) {
  max_int_ = std::min(max_int_, maximum);
  return *this;
}

Range& Range::AtMost(Expression maximum) {
  max_exprs_.push_back(std::move(maximum));
  return *this;
}

Range& Range::AtMost(const Real& maximum) {
  max_real_ = max_real_ ? std::min(*max_real_, maximum) : maximum;
  return *this;
}

namespace {

// Evaluate all expressions, then find the minimum/maximum, depending on the
// value of `compare`.
template <typename F>
int64_t FindExtreme(int64_t initial_value, std::span<const Expression> exprs,
                    std::function<int64_t(std::string_view)> get_value,
                    F compare) {
  for (const Expression& expr : exprs) {
    int64_t val = expr.Evaluate(get_value);
    if (compare(val, initial_value)) initial_value = val;
  }
  return initial_value;
}

}  // namespace

std::optional<Range::ExtremeValues> Range::Extremes(
    std::function<int64_t(std::string_view)> get_value) const {
  int64_t min = min_real_ ? std::max(min_int_, min_real_->Ceiling()) : min_int_;
  int64_t max = max_real_ ? std::min(max_int_, max_real_->Floor()) : max_int_;
  ExtremeValues extremes = {
      .min = FindExtreme(min, min_exprs_, get_value, std::greater<int64_t>()),
      .max = FindExtreme(max, max_exprs_, get_value, std::less<int64_t>())};

  if (extremes.min > extremes.max) return std::nullopt;

  return extremes;
}

void Range::Intersect(const Range& other) {
  AtLeast(other.min_int_);
  AtMost(other.max_int_);
  if (other.min_real_) AtLeast(*other.min_real_);
  if (other.max_real_) AtMost(*other.max_real_);

  min_exprs_.insert(min_exprs_.end(), other.min_exprs_.begin(),
                    other.min_exprs_.end());
  max_exprs_.insert(max_exprs_.end(), other.max_exprs_.begin(),
                    other.max_exprs_.end());
}

namespace {

// Return a nice string representation of these bounds. If there is no
// restriction, then nullopt is returned. If there is one restriction, it will
// just return that. Otherwise, will return a comma separated list of bounds.
std::optional<std::string> BoundsToString(
    bool is_minimum, int64_t int_limit, const std::optional<Real>& real_limit,
    std::span<const Expression> expression_limits) {
  bool has_int_limit =
      int_limit != (is_minimum ? std::numeric_limits<int64_t>::min()
                               : std::numeric_limits<int64_t>::max());
  bool has_real_limit = real_limit.has_value();

  // No restrictions.
  if (expression_limits.empty() && !has_int_limit && !has_real_limit)
    return std::nullopt;

  std::vector<std::string> bounds;
  bounds.reserve(expression_limits.size() + 1);
  if (has_int_limit) bounds.push_back(std::to_string(int_limit));
  if (has_real_limit) bounds.push_back(real_limit->ToString());
  for (const Expression& expr : expression_limits)
    bounds.push_back(expr.ToString());

  if (bounds.size() == 1) return bounds[0];

  // Note, we swap min/max here since we want max(a, b, c) <= x <= min(d, e, f).
  return std::format("{}({})", (is_minimum ? "max" : "min"),
                     absl::StrJoin(bounds, ", "));
}

}  // namespace

std::string Range::ToString() const {
  std::optional<std::string> min_bounds =
      BoundsToString(/* is_minimum = */ true, min_int_, min_real_, min_exprs_);
  std::optional<std::string> max_bounds =
      BoundsToString(/* is_minimum = */ false, max_int_, max_real_, max_exprs_);

  if (!min_bounds.has_value() && !max_bounds.has_value()) return "(-inf, inf)";
  if (!min_bounds.has_value()) return std::format("(-inf, {}]", *max_bounds);
  if (!max_bounds.has_value()) return std::format("[{}, inf)", *min_bounds);

  return std::format("[{}, {}]", *min_bounds, *max_bounds);
}

bool operator==(const Range& r1, const Range& r2) {
  if (std::tie(r1.min_int_, r1.max_int_, r1.min_real_, r1.max_real_) !=
      std::tie(r2.min_int_, r2.max_int_, r2.min_real_, r2.max_real_))
    return false;
  if (r1.min_exprs_.size() != r2.min_exprs_.size() ||
      r1.max_exprs_.size() != r2.max_exprs_.size())
    return false;
  for (int i = 0; i < r1.min_exprs_.size(); ++i) {
    if (r1.min_exprs_[i].ToString() != r2.min_exprs_[i].ToString())
      return false;
  }
  for (int i = 0; i < r1.max_exprs_.size(); ++i) {
    if (r1.max_exprs_[i].ToString() != r2.max_exprs_[i].ToString())
      return false;
  }
  return true;
}

Range EmptyRange() { return Range().AtLeast(1).AtMost(0); }

}  // namespace moriarty
