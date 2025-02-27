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

#include "src/contexts/internal/basic_ostream_context.h"

#include <ios>
#include <sstream>

#include "gtest/gtest.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

TEST(BasicOStreamContextTest, PrintWhitespaceShouldPrintTheCorrectWhitespace) {
  std::stringstream ss;
  BasicOStreamContext c(ss);
  c.PrintWhitespace(Whitespace::kSpace);
  EXPECT_EQ(ss.str(), " ");
  c.PrintWhitespace(Whitespace::kNewline);
  EXPECT_EQ(ss.str(), " \n");
  c.PrintWhitespace(Whitespace::kTab);
  EXPECT_EQ(ss.str(), " \n\t");
}

TEST(BasicOStreamContextTest, PrintTokenShouldPrintProperly) {
  std::stringstream ss;
  BasicOStreamContext c(ss);
  c.PrintToken("Hello!");
  EXPECT_EQ(ss.str(), "Hello!");
  c.PrintToken("bye");
  EXPECT_EQ(ss.str(), "Hello!bye");
}

TEST(BasicOStreamContextTest, PrintingToABadStreamShouldThrow) {
  // Error in stream before construction
  std::stringstream ss1;
  ss1.setstate(std::ios_base::failbit);
  EXPECT_THROW({ BasicOStreamContext c(ss1); }, std::ios_base::failure);

  // Error in stream after construction
  std::stringstream ss2;
  BasicOStreamContext c(ss2);
  EXPECT_THROW(
      { ss2.setstate(std::ios_base::failbit); }, std::ios_base::failure);
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
