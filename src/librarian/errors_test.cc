// Copyright 2025 Darcy Best
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

#include "src/librarian/errors.h"

#include <stdexcept>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/librarian/policies.h"
#include "src/librarian/testing/gtest_helpers.h"

namespace moriarty {
namespace {

using ::moriarty::ValueTypeMismatch;
using ::moriarty_testing::ThrowsGenerationError;
using ::moriarty_testing::ThrowsImpossibleToSatisfy;
using ::moriarty_testing::ThrowsMVariableTypeMismatch;
using ::moriarty_testing::ThrowsValueNotFound;
using ::moriarty_testing::ThrowsValueTypeMismatch;
using ::moriarty_testing::ThrowsVariableNotFound;
using ::testing::Not;

TEST(ErrorsTest, ThrowsVariableNotFoundMatcherShouldWorkCorrectly) {
  {  // Happy path
    EXPECT_THAT([] { throw VariableNotFound("X"); },
                ThrowsVariableNotFound("X"));
  }
  {  // Wrong name
    EXPECT_THAT([] { throw VariableNotFound("X"); },
                Not(ThrowsVariableNotFound("other")));
    EXPECT_THAT([] { throw VariableNotFound("X"); },
                Not(ThrowsVariableNotFound("")));
  }
  {  // Wrong exception
    EXPECT_THAT([] { throw std::runtime_error("words"); },
                Not(ThrowsVariableNotFound("X")));
    EXPECT_THAT([] { throw std::runtime_error("X"); },
                Not(ThrowsVariableNotFound("X")));
  }
  {  // No exception
    EXPECT_THAT([] {}, Not(ThrowsVariableNotFound("X")));
  }
  {  // Multiple throws (::testing::ThrowsMessage gets these wrong)
    EXPECT_THAT(
        [] {
          throw std::runtime_error("words");
          throw VariableNotFound("X");
        },
        Not(ThrowsVariableNotFound("X")));
    EXPECT_THAT(
        [] {
          throw VariableNotFound("X");
          throw std::runtime_error("words");
        },
        ThrowsVariableNotFound("X"));
  }
}

TEST(ErrorsTest, ThrowsValueNotFoundMatcherShouldWorkCorrectly) {
  {  // Happy path
    EXPECT_THAT([] { throw ValueNotFound("X"); }, ThrowsValueNotFound("X"));
  }
  {  // Wrong name
    EXPECT_THAT([] { throw ValueNotFound("X"); },
                Not(ThrowsValueNotFound("other")));
    EXPECT_THAT([] { throw ValueNotFound("X"); }, Not(ThrowsValueNotFound("")));
  }
  {  // Wrong exception
    EXPECT_THAT([] { throw std::runtime_error("words"); },
                Not(ThrowsValueNotFound("X")));
    EXPECT_THAT([] { throw std::runtime_error("X"); },
                Not(ThrowsValueNotFound("X")));
  }
  {  // No exception
    EXPECT_THAT([] {}, Not(ThrowsValueNotFound("X")));
  }
  {  // Multiple throws (::testing::ThrowsMessage gets these wrong)
    EXPECT_THAT(
        [] {
          throw std::runtime_error("words");
          throw ValueNotFound("X");
        },
        Not(ThrowsValueNotFound("X")));
    EXPECT_THAT(
        [] {
          throw ValueNotFound("X");
          throw std::runtime_error("words");
        },
        ThrowsValueNotFound("X"));
  }
}

TEST(ErrorsTest, ThrowsMVariableTypeMismatchMatcherShouldWorkCorrectly) {
  {  // Happy path
    EXPECT_THAT([] { throw MVariableTypeMismatch("x", "y"); },
                ThrowsMVariableTypeMismatch("x", "y"));
  }
  {  // Wrong name
    EXPECT_THAT([] { throw MVariableTypeMismatch("x", "y"); },
                Not(ThrowsMVariableTypeMismatch("x", "zy")));
    EXPECT_THAT([] { throw MVariableTypeMismatch("x", "y"); },
                Not(ThrowsMVariableTypeMismatch("zx", "y")));
  }
  {  // Wrong exception
    EXPECT_THAT([] { throw std::runtime_error("words"); },
                Not(ThrowsMVariableTypeMismatch("words", "words")));
  }
  {  // No exception
    EXPECT_THAT([] {}, Not(ThrowsMVariableTypeMismatch("X", "Y")));
  }
  {  // Multiple throws (::testing::ThrowsMessage gets these wrong)
    EXPECT_THAT(
        [] {
          throw std::runtime_error("words");
          throw MVariableTypeMismatch("X", "Y");
        },
        Not(ThrowsMVariableTypeMismatch("X", "Y")));
    EXPECT_THAT(
        [] {
          throw MVariableTypeMismatch("X", "Y");
          throw std::runtime_error("words");
        },
        ThrowsMVariableTypeMismatch("X", "Y"));
  }
}

TEST(ErrorsTest, ThrowsValueTypeMismatchMatcherShouldWorkCorrectly) {
  {  // Happy path
    EXPECT_THAT([] { throw ValueTypeMismatch("x", "y"); },
                ThrowsValueTypeMismatch("x", "y"));
  }
  {  // Wrong name
    EXPECT_THAT([] { throw ValueTypeMismatch("x", "y"); },
                Not(ThrowsValueTypeMismatch("x", "zy")));
    EXPECT_THAT([] { throw ValueTypeMismatch("x", "y"); },
                Not(ThrowsValueTypeMismatch("zx", "y")));
  }
  {  // Wrong exception
    EXPECT_THAT([] { throw std::runtime_error("words"); },
                Not(ThrowsValueTypeMismatch("words", "words")));
  }
  {  // No exception
    EXPECT_THAT([] {}, Not(ThrowsValueTypeMismatch("X", "Y")));
  }
  {  // Multiple throws (::testing::ThrowsMessage gets these wrong)
    EXPECT_THAT(
        [] {
          throw std::runtime_error("words");
          throw ValueTypeMismatch("X", "Y");
        },
        Not(ThrowsValueTypeMismatch("X", "Y")));
    EXPECT_THAT(
        [] {
          throw ValueTypeMismatch("X", "Y");
          throw std::runtime_error("words");
        },
        ThrowsValueTypeMismatch("X", "Y"));
  }
}

TEST(ErrorsTest, ThrowsImpossibleToSatisfyMatcherShouldWorkCorrectly) {
  {  // Happy path
    EXPECT_THAT([] { throw ImpossibleToSatisfy("x", "y"); },
                ThrowsImpossibleToSatisfy("x"));
    EXPECT_THAT([] { throw ImpossibleToSatisfy("x", "y"); },
                ThrowsImpossibleToSatisfy("y"));
  }
  {  // Wrong name
    EXPECT_THAT([] { throw ImpossibleToSatisfy("x", "y"); },
                Not(ThrowsImpossibleToSatisfy("zz")));
  }
  {  // Wrong exception
    EXPECT_THAT([] { throw std::runtime_error("words"); },
                Not(ThrowsImpossibleToSatisfy("words")));
  }
  {  // No exception
    EXPECT_THAT([] {}, Not(ThrowsImpossibleToSatisfy("")));
  }
  {  // Multiple throws (::testing::ThrowsMessage gets these wrong)
    EXPECT_THAT(
        [] {
          throw std::runtime_error("words");
          throw ImpossibleToSatisfy("X", "Y");
        },
        Not(ThrowsImpossibleToSatisfy("X")));
    EXPECT_THAT(
        [] {
          throw ImpossibleToSatisfy("X", "Y");
          throw std::runtime_error("words");
        },
        ThrowsImpossibleToSatisfy("X"));
  }
}

TEST(ErrorsTest, ThrowsGenerationErrorMatcherShouldWorkCorrectly) {
  using RetryPolicy::kAbort;
  using RetryPolicy::kRetry;
  {  // Happy path
    EXPECT_THAT([] { throw GenerationError("x", "y", kAbort); },
                ThrowsGenerationError("x", "y"));
    EXPECT_THAT([] { throw GenerationError("x", "y", kRetry); },
                ThrowsGenerationError("x", "y"));
  }
  {  // Wrong name / message
    EXPECT_THAT([] { throw GenerationError("x", "y", kRetry); },
                Not(ThrowsGenerationError("zz", "y")));
    EXPECT_THAT([] { throw GenerationError("x", "y", kRetry); },
                Not(ThrowsGenerationError("x", "zz")));
  }
  {  // Wrong exception
    EXPECT_THAT([] { throw std::runtime_error("words"); },
                Not(ThrowsGenerationError("words", "words")));
  }
  {  // No exception
    EXPECT_THAT([] {}, Not(ThrowsGenerationError("", "")));
  }
  {  // Multiple throws (::testing::ThrowsMessage gets these wrong)
    EXPECT_THAT(
        [] {
          throw std::runtime_error("words");
          throw GenerationError("X", "Y", kRetry);
        },
        Not(ThrowsGenerationError("X", "Y")));
    EXPECT_THAT(
        [] {
          throw GenerationError("X", "Y", kRetry);
          throw std::runtime_error("words");
        },
        ThrowsGenerationError("X", "Y"));
  }
}

}  // namespace
}  // namespace moriarty
