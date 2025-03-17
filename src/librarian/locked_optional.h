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

#ifndef MORIARTY_SRC_LIBRARIAN_LOCKED_OPTIONAL_H_
#define MORIARTY_SRC_LIBRARIAN_LOCKED_OPTIONAL_H_

#include <optional>

namespace moriarty {
namespace librarian {

// LockedOptional
//
// A class that holds a single value. If Set() is called multiple times,
// then each call must have the same value.
template <typename T>
class LockedOptional {
 public:
  explicit LockedOptional(T default_value)
      : default_value_(std::move(default_value)) {}

  // Sets the value to be the given value. If the value is already set and is
  // not equal to the current value, returns false and the underlying value does
  // not change. Otherwise, returns true.
  bool Set(T value) {
    if (!value_) {
      value_ = value;
      return true;
    }

    return *value_ == value;
  }

  // Returns the value.
  [[nodiscard]] T Get() const { return value_.value_or(default_value_); }

 private:
  std::optional<T> value_;
  T default_value_;
};

}  // namespace librarian
}  // namespace moriarty

#endif  // MORIARTY_SRC_LIBRARIAN_LOCKED_OPTIONAL_H_
