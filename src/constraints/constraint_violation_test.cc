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

#include "src/constraints/constraint_violation.h"

#include <memory>
#include <optional>
#include <string>

#include "gtest/gtest.h"

namespace moriarty {
namespace {

TEST(ConstraintViolationTest, PrettyReasonFormatsTreeWithBranches) {
  ValidationResult result = ValidationResult::Violation(
      "x", std::string("Hello"),
      ValidationResult::Violation("length", 5, librarian::Expected("0 (mod N)"),
                                  librarian::Evaluated("0 (mod 2)"),
                                  librarian::Details("5 ≡ 1 (mod 2)")));

  EXPECT_EQ(result.PrettyReason(),
            "x: \"Hello\"\n"
            "╰─ length: 5\n"
            "   ╰─ expected: 0 (mod N)\n"
            "      which is: 0 (mod 2)\n"
            "       details: 5 ≡ 1 (mod 2)\n");
}

TEST(ConstraintViolationTest,
     PrettyPrintValidationResultFormatsArrayIndexLengthViolation) {
  using ::moriarty::moriarty_internal::PrettyPrintValidationResult;
  using ::moriarty::moriarty_internal::ValidationResultNode;

  auto leaf = std::make_unique<ValidationResultNode>(ValidationResultNode{
      "length", "14",
      ValidationResultNode::Details{
          .expected = librarian::Expected("1 ≤ length ≤ 3 * N + 1"),
          .evaluated = librarian::Evaluated("1 ≤ length ≤ 13"),
          .details = librarian::Details("too large")}});

  auto index_5 = std::make_unique<ValidationResultNode>(
      ValidationResultNode{"index 5", "\"abcdefghijklmn\"", std::move(leaf)});

  ValidationResultNode root{
      "strings",
      "[\"a\", \"bb\", \"ccc\", \"dddd\", \"eeeee\", \"abcdefghijklmn\", ...]",
      std::move(index_5)};

  EXPECT_EQ(
      PrettyPrintValidationResult(root),
      R"(strings: ["a", "bb", "ccc", "dddd", "eeeee", "abcdefghijklmn", ...]
╰─ index 5: "abcdefghijklmn"
   ╰─ length: 14
      ╰─ expected: 1 ≤ length ≤ 3 * N + 1
         which is: 1 ≤ length ≤ 13
          details: too large
)");
}

TEST(ConstraintViolationTest,
     PrettyPrintValidationResultHandlesAllOptionalLeafConfigurations) {
  using ::moriarty::moriarty_internal::PrettyPrintValidationResult;
  using ::moriarty::moriarty_internal::ValidationResultNode;

  {
    ValidationResultNode node{
        "length", "14",
        ValidationResultNode::Details{
            .expected = librarian::Expected("1 ≤ length ≤ 10"),
            .evaluated = std::nullopt,
            .details = std::nullopt}};

    EXPECT_EQ(PrettyPrintValidationResult(node),
              "length: 14\n"
              "╰─ expected: 1 ≤ length ≤ 10\n");
  }

  {
    ValidationResultNode node{
        "length", "14",
        ValidationResultNode::Details{
            .expected = librarian::Expected("1 ≤ length ≤ 3 * N + 1"),
            .evaluated = librarian::Evaluated("1 ≤ length ≤ 13"),
            .details = std::nullopt}};

    EXPECT_EQ(PrettyPrintValidationResult(node),
              "length: 14\n"
              "╰─ expected: 1 ≤ length ≤ 3 * N + 1\n"
              "   which is: 1 ≤ length ≤ 13\n");
  }

  {
    ValidationResultNode node{
        "length", "14",
        ValidationResultNode::Details{
            .expected = librarian::Expected("1 ≤ length ≤ 10"),
            .evaluated = std::nullopt,
            .details = librarian::Details("too large")}};

    EXPECT_EQ(PrettyPrintValidationResult(node),
              "length: 14\n"
              "╰─ expected: 1 ≤ length ≤ 10\n"
              "    details: too large\n");
  }

  {
    ValidationResultNode node{
        "length", "14",
        ValidationResultNode::Details{
            .expected = librarian::Expected("1 ≤ length ≤ 3 * N + 1"),
            .evaluated = librarian::Evaluated("1 ≤ length ≤ 13"),
            .details = librarian::Details("too large")}};

    EXPECT_EQ(PrettyPrintValidationResult(node),
              "length: 14\n"
              "╰─ expected: 1 ≤ length ≤ 3 * N + 1\n"
              "   which is: 1 ≤ length ≤ 13\n"
              "    details: too large\n");
  }
}

}  // namespace
}  // namespace moriarty
