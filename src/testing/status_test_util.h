/*
 * Copyright 2025 Darcy Best
 * Copyright 2024 Google LLC
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

#ifndef MORIARTY_SRC_TESTING_STATUS_TEST_UTIL_H_
#define MORIARTY_SRC_TESTING_STATUS_TEST_UTIL_H_

// Moriarty Error Space googletest Matchers
//
// These test macros make testing Moriarty code much easier.
//
// For example:
//  EXPECT_THAT(Generate(MInteger().Between(5, 4)), IsGenerationFailure());

#include <functional>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/errors.h"

namespace moriarty_testing {
namespace moriarty_testing_internal {

// Convenience functions to extract status from either absl::Status or
// absl::StatusOr<>. Use this inside of MATCHER if you want to accept either
// `absl::Status` or `absl::StatusOr`.
inline absl::Status GetStatus(const absl::Status& status) { return status; }

template <typename T>
inline absl::Status GetStatus(const absl::StatusOr<T>& status_or) {
  return status_or.status();
}

// https://github.com/google/googletest/issues/4073#issuecomment-1925047305
template <class Function>
std::function<void()> single_call(Function function) {
  auto shared_exception_ptr = std::make_shared<std::exception_ptr>();
  auto was_called = std::make_shared<bool>(false);
  return [shared_exception_ptr, was_called, function]() {
    if (*shared_exception_ptr) {
      std::rethrow_exception(*shared_exception_ptr);
    }
    if (*was_called) {
      return;
    }
    *was_called = true;
    try {
      std::invoke(function);
    } catch (...) {
      *shared_exception_ptr = std::current_exception();
      std::rethrow_exception(*shared_exception_ptr);
    }
  };
}

}  // namespace moriarty_testing_internal

MATCHER_P(ThrowsVariableNotFound, expected_variable_name,
          std::format("{} a function that throws a VariableNotFound exception "
                      "with the variable name `{}`",
                      (negation ? "is not" : "is"), expected_variable_name)) {
  // This first line is to get a better compile error message when arg is not of
  // the expected type.
  std::function<void()> fn = arg;

  std::function<void()> function = moriarty_testing_internal::single_call(fn);
  try {
    function();
    *result_listener << "did not throw";
    return false;
  } catch (const moriarty::VariableNotFound& e) {
    if (e.VariableName() != expected_variable_name) {
      *result_listener
          << "threw the expected exception type, but the wrong variable name. `"
          << e.VariableName() << "`";
      return false;
    }
    *result_listener << "threw the expected exception";
    return true;
  } catch (...) {
    // Call the built-in explainer to get a better message.
    return ::testing::ExplainMatchResult(
        ::testing::Throws<moriarty::VariableNotFound>(), function,
        result_listener);
  }
}

MATCHER_P(ThrowsValueNotFound, expected_variable_name,
          std::format("{} a function that throws a ValueNotFound exception "
                      "with the variable name `{}`",
                      (negation ? "is not" : "is"), expected_variable_name)) {
  // This first line is to get a better compile error message when arg is not of
  // the expected type.
  std::function<void()> fn = arg;

  std::function<void()> function = moriarty_testing_internal::single_call(fn);
  try {
    function();
    *result_listener << "did not throw";
    return false;
  } catch (const moriarty::ValueNotFound& e) {
    if (e.VariableName() != expected_variable_name) {
      *result_listener
          << "threw the expected exception type, but the wrong variable name. `"
          << e.VariableName() << "`";
      return false;
    }
    *result_listener << "threw the expected exception";
    return true;
  } catch (...) {
    // Call the built-in explainer to get a better message.
    return ::testing::ExplainMatchResult(
        ::testing::Throws<moriarty::ValueNotFound>(), function,
        result_listener);
  }
}

MATCHER_P2(ThrowsMVariableTypeMismatch, from_type, to_type,
           std::format("{} a function that throws a MVariableTypeMismatch "
                       "exception when trying to convert {} to {}",
                       (negation ? "is not" : "is"), from_type, to_type)) {
  // This first line is to get a better compile error message when arg is not of
  // the expected type.
  std::function<void()> fn = arg;

  std::function<void()> function = moriarty_testing_internal::single_call(fn);
  try {
    function();
    *result_listener << "did not throw";
    return false;
  } catch (const moriarty::MVariableTypeMismatch& e) {
    if (e.ConvertingFrom() != from_type || e.ConvertingTo() != to_type) {
      *result_listener
          << "threw the expected exception type, but is converting "
          << e.ConvertingFrom() << " to " << e.ConvertingTo();
      return false;
    }
    *result_listener << "threw the expected exception";
    return true;
  } catch (...) {
    // Call the built-in explainer to get a better message.
    return ::testing::ExplainMatchResult(
        ::testing::Throws<moriarty::MVariableTypeMismatch>(), function,
        result_listener);
  }
}

MATCHER_P2(
    ThrowsValueTypeMismatch, name, type,
    std::format("{} a function that throws a ValueTypeMismatch "
                "exception when trying to convert the variable named {} to {}",
                (negation ? "is not" : "is"), name, type)) {
  // This first line is to get a better compile error message when arg is not of
  // the expected type.
  std::function<void()> fn = arg;

  std::function<void()> function = moriarty_testing_internal::single_call(fn);
  try {
    function();
    *result_listener << "did not throw";
    return false;
  } catch (const moriarty::ValueTypeMismatch& e) {
    if (e.Name() != name || e.Type() != type) {
      *result_listener
          << "threw the expected exception type, but is converting " << e.Name()
          << " to " << e.Type();
      return false;
    }
    *result_listener << "threw the expected exception";
    return true;
  } catch (...) {
    // Call the built-in explainer to get a better message.
    return ::testing::ExplainMatchResult(
        ::testing::Throws<moriarty::MVariableTypeMismatch>(), function,
        result_listener);
  }
}

}  // namespace moriarty_testing

#endif  // MORIARTY_SRC_TESTING_STATUS_TEST_UTIL_H_
