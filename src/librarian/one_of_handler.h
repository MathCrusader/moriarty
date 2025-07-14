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

#ifndef MORIARTY_SRC_LIBRARIAN_ONEOF_HANDLER_H_
#define MORIARTY_SRC_LIBRARIAN_ONEOF_HANDLER_H_

#include <algorithm>
#include <optional>
#include <vector>

namespace moriarty {
namespace librarian {

// OneOfHandler
//
// Maintains a list of possible values for a variable. Initially, all values are
// considered possible. As new calls come in, values are removed from the list
// of possibilities.
//
// Note: See OneOfNumeric for a numeric specialization of this class.
template <typename T>
class OneOfHandler {
 public:
  // HasBeenConstrained()
  //
  // Returns true if any calls to ConstrainOptions() have been made.
  [[nodiscard]] bool HasBeenConstrained() const;

  // ConstrainOptions()
  //
  // Adds additional constraints that this object must be one of `one_of`. This
  // will intersect these values with any existing options.
  //
  // Returns a boolean indicating if there are any valid options that remain
  // after constraining.
  template <typename Container>
  [[nodiscard]] bool ConstrainOptions(Container&& one_of);

  // HasOption()
  //
  // Determines if the given value is one of the valid values.
  [[nodiscard]] bool HasOption(const T& value) const;

  // SelectOneOf()
  //
  // Returns a random value from the list of possible values. rand_fn should
  // take exactly one argument, an integer, and return an integer in the range
  // [0, n). The underlying items may or may not be well-ordered. Do not depend
  // on any particular ordering.
  template <typename RandFn>
  [[nodiscard]] T SelectOneOf(RandFn&& rand_fn) const;

  // GetUniqueValue()
  //
  // Returns the unique value if there is only one possible value.
  std::optional<T> GetUniqueValue() const;

  // GetOptions()
  //
  // Returns a list of all valid values.
  //
  // Precondition: HasBeenConstrained() == true
  [[nodiscard]] const std::vector<T>& GetOptions() const;

 private:
  std::optional<std::vector<T>> valid_options_;
};

// -----------------------------------------------------------------------------
//  Template implementation below

template <typename T>
bool OneOfHandler<T>::HasBeenConstrained() const {
  return valid_options_.has_value();
}

template <typename T>
bool OneOfHandler<T>::HasOption(const T& value) const {
  return !valid_options_ ||
         std::find(valid_options_->begin(), valid_options_->end(), value) !=
             valid_options_->end();
}

template <typename T>
template <typename RandFn>
T OneOfHandler<T>::SelectOneOf(RandFn&& rand_fn) const {
  return *std::next(valid_options_->begin(), rand_fn(valid_options_->size()));
}

template <typename T>
std::optional<T> OneOfHandler<T>::GetUniqueValue() const {
  if (!valid_options_ || valid_options_->size() != 1) return std::nullopt;
  return valid_options_->front();
}

template <typename T>
const std::vector<T>& OneOfHandler<T>::GetOptions() const {
  return *valid_options_;
}

template <typename T>
template <typename Container>
bool OneOfHandler<T>::ConstrainOptions(Container&& one_of) {
  if (!valid_options_.has_value()) {
    valid_options_ = std::vector<T>();
    std::ranges::copy(one_of, std::back_inserter(*valid_options_));
  } else {
    std::erase_if(*valid_options_, [&one_of](const T& value) {
      return std::ranges::find(one_of, value) == one_of.end();
    });
  }

  return !valid_options_->empty();
}

}  // namespace librarian
}  // namespace moriarty

#endif  // MORIARTY_SRC_LIBRARIAN_ONEOF_HANDLER_H_
