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

TEST(BasicOStreamContextTest, WriteWhitespaceShouldWriteTheCorrectWhitespace) {
  std::stringstream ss;
  BasicOStreamContext c(ss);
  c.WriteWhitespace(Whitespace::kSpace);
  EXPECT_EQ(ss.str(), " ");
  c.WriteWhitespace(Whitespace::kNewline);
  EXPECT_EQ(ss.str(), " \n");
  c.WriteWhitespace(Whitespace::kTab);
  EXPECT_EQ(ss.str(), " \n\t");
}

TEST(BasicOStreamContextTest, WriteTokenShouldWriteProperly) {
  std::stringstream ss;
  BasicOStreamContext c(ss);
  c.WriteToken("Hello!");
  EXPECT_EQ(ss.str(), "Hello!");
  c.WriteToken("bye");
  EXPECT_EQ(ss.str(), "Hello!bye");
}

TEST(BasicOStreamContextTest, WriteingToABadStreamShouldThrow) {
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
