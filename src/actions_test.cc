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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/actions.h"
#include "src/constraints/numeric_constraints.h"
#include "src/librarian/errors.h"
#include "src/librarian/policies.h"
#include "src/librarian/testing/gtest_helpers.h"
#include "src/problem.h"
#include "src/simple_io.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace {

using ::moriarty_testing::SingleCall;
using ::testing::HasSubstr;

// -----------------------------------------------------------------------------

TEST(ValidateInputTest, WorksForValidInput) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))),
            InputFormat(SimpleIO().AddLine("N")));

  std::istringstream input("5\n");
  EXPECT_NO_THROW(ValidateInput(p).ReadInputUsing({.is = input}).Run());
}

TEST(ValidateInputTest, InvalidVariableShouldFail) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))),
            InputFormat(SimpleIO().AddLine("N")));

  std::istringstream input("123\n");
  EXPECT_THAT(
      SingleCall([&] { ValidateInput(p).ReadInputUsing({.is = input}).Run(); }),
      ThrowsMessage<IOError>(HasSubstr("between 1 and 10")));
}

TEST(ValidateInputTest, InvalidWhitespaceShouldFail) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10)))),
            InputFormat(SimpleIO().AddLine("N")));

  {
    std::istringstream input(" 5\n");
    EXPECT_THAT(
        SingleCall(
            [&] { ValidateInput(p).ReadInputUsing({.is = input}).Run(); }),
        ThrowsMessage<IOError>(HasSubstr("Expected a token, but got ' '")));
  }
  {
    std::istringstream input("5");
    EXPECT_THAT(
        SingleCall(
            [&] { ValidateInput(p).ReadInputUsing({.is = input}).Run(); }),
        ThrowsMessage<IOError>(HasSubstr("Expected '\\n', but got EOF")));
  }
  {
    std::istringstream input("");
    EXPECT_THAT(
        SingleCall(
            [&] { ValidateInput(p).ReadInputUsing({.is = input}).Run(); }),
        ThrowsMessage<IOError>(HasSubstr("Expected a token, but got EOF")));
  }
}

TEST(ValidateInputTest, ValidMultipleTestCasesShouldSucceed) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10))),
                      Var("X", MInteger(Between(20, 25)))),
            InputFormat(
                SimpleIO().WithNumberOfTestCasesInHeader().AddLine("N", "X")));

  std::istringstream input(R"(3
7 21
1 22
5 25
)");
  EXPECT_NO_THROW(ValidateInput(p).ReadInputUsing({.is = input}).Run());
}

TEST(ValidateInputTest, InvalidMultipleTestCasesShouldFail) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10))),
                      Var("X", MInteger(Between(20, 25)))),
            InputFormat(
                SimpleIO().WithNumberOfTestCasesInHeader().AddLine("N", "X")));
  {
    std::istringstream input(R"(4
7 21
1 22
5 25
)");
    EXPECT_THAT(
        SingleCall(
            [&] { ValidateInput(p).ReadInputUsing({.is = input}).Run(); }),
        ThrowsMessage<IOError>(HasSubstr("Expected a token, but got EOF")));
  }
  {
    std::istringstream input(R"(2
7 21
1 22
5 25
)");
    EXPECT_THAT(
        SingleCall(
            [&] { ValidateInput(p).ReadInputUsing({.is = input}).Run(); }),
        ThrowsMessage<IOError>(HasSubstr("Expected EOF, but got more input")));
  }
}

TEST(ValidateInputTest, WhitespaceStrictnessShouldBeRespected) {
  Problem p(Variables(Var("N", MInteger(Between(1, 10))),
                      Var("X", MInteger(Between(20, 25)))),
            InputFormat(SimpleIO().AddLine("N", "X")));
  std::istringstream input(" 7 \t  \n \n21");
  EXPECT_NO_THROW(ValidateInput(p)
                      .ReadInputUsing({.is = input,
                                       .whitespace_strictness =
                                           WhitespaceStrictness::kFlexible})
                      .Run());
}

}  // namespace
}  // namespace moriarty
