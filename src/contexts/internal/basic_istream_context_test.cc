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

#include "src/contexts/internal/basic_istream_context.h"

#include <sstream>

#include "gtest/gtest.h"
#include "src/librarian/errors.h"
#include "src/librarian/io_config.h"
#include "src/librarian/policies.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

TEST(BasicIStreamTest, ReadTokenShouldAdhereToWhitespaceScrictness) {
  {
    std::stringstream ss("  \n\tHello");
    BasicIStreamContext c(ss, WhitespaceStrictness::kFlexible);
    EXPECT_EQ(c.ReadToken(), "Hello");
  }
  {
    std::stringstream ss("  \n\tHello");
    BasicIStreamContext c(ss, WhitespaceStrictness::kPrecise);
    EXPECT_THROW({ (void)c.ReadToken(); }, IOError);
  }
}

TEST(BasicIStreamTest, ReadTokenShouldWorkInSimpleCases) {
  {
    std::stringstream ss("Hello");
    BasicIStreamContext c(ss, WhitespaceStrictness::kFlexible);
    EXPECT_EQ(c.ReadToken(), "Hello");
  }
  {
    std::stringstream ss("Hello");
    BasicIStreamContext c(ss, WhitespaceStrictness::kPrecise);
    EXPECT_EQ(c.ReadToken(), "Hello");
  }
  {
    std::stringstream ss("Hello World");
    BasicIStreamContext c(ss, WhitespaceStrictness::kFlexible);
    EXPECT_EQ(c.ReadToken(), "Hello");
    EXPECT_EQ(c.ReadToken(), "World");
  }
  {
    std::stringstream ss("Hello World");
    BasicIStreamContext c(ss, WhitespaceStrictness::kPrecise);
    EXPECT_EQ(c.ReadToken(), "Hello");
    EXPECT_THROW({ (void)c.ReadToken(); }, IOError);
  }
}

TEST(BasicIStreamTest, ReadEofShouldWork) {
  {
    std::stringstream ss("");
    BasicIStreamContext c(ss, WhitespaceStrictness::kFlexible);
    EXPECT_NO_THROW({ c.ReadEof(); });
  }
  {
    std::stringstream ss("     \t\n\t");
    BasicIStreamContext c(ss, WhitespaceStrictness::kFlexible);
    EXPECT_NO_THROW({ c.ReadEof(); });
  }
  {
    std::stringstream ss("");
    BasicIStreamContext c(ss, WhitespaceStrictness::kPrecise);
    EXPECT_NO_THROW({ c.ReadEof(); });
  }
  {
    std::stringstream ss("     \t\n\t");
    BasicIStreamContext c(ss, WhitespaceStrictness::kPrecise);
    EXPECT_THROW({ c.ReadEof(); }, IOError);
  }
  {
    std::stringstream ss("Hello");
    BasicIStreamContext c(ss, WhitespaceStrictness::kFlexible);
    EXPECT_THROW({ c.ReadEof(); }, IOError);
  }
  {
    std::stringstream ss("Hello");
    BasicIStreamContext c(ss, WhitespaceStrictness::kPrecise);
    EXPECT_THROW({ c.ReadEof(); }, IOError);
  }
  {
    std::stringstream ss("Hello");
    BasicIStreamContext c(ss, WhitespaceStrictness::kFlexible);
    EXPECT_EQ(c.ReadToken(), "Hello");
    EXPECT_NO_THROW({ c.ReadEof(); });
  }
  {
    std::stringstream ss("Hello");
    BasicIStreamContext c(ss, WhitespaceStrictness::kPrecise);
    EXPECT_EQ(c.ReadToken(), "Hello");
    EXPECT_NO_THROW({ c.ReadEof(); });
  }
}

TEST(BasicIStreamTest, ReadWhitespaceShouldWorkInHappyPath) {
  {
    std::stringstream ss(" \n\tHello");
    BasicIStreamContext c(ss, WhitespaceStrictness::kFlexible);
    c.ReadWhitespace(Whitespace::kSpace);
    c.ReadWhitespace(Whitespace::kNewline);
    c.ReadWhitespace(Whitespace::kTab);
    EXPECT_EQ(c.ReadToken(), "Hello");
  }
  {
    std::stringstream ss(" \n\tHello");
    BasicIStreamContext c(ss, WhitespaceStrictness::kPrecise);
    c.ReadWhitespace(Whitespace::kSpace);
    c.ReadWhitespace(Whitespace::kNewline);
    c.ReadWhitespace(Whitespace::kTab);
    EXPECT_EQ(c.ReadToken(), "Hello");
  }
}

TEST(BasicIStreamTest, ReadWhitespaceShouldCareWhichWhitespaceInStrictMode) {
  {  // Happy
    std::stringstream ss(" \n\t");
    BasicIStreamContext c(ss, WhitespaceStrictness::kPrecise);
    c.ReadWhitespace(Whitespace::kSpace);
    c.ReadWhitespace(Whitespace::kNewline);
    c.ReadWhitespace(Whitespace::kTab);
  }
  {  // Not space
    std::stringstream ss("\n");
    BasicIStreamContext c(ss, WhitespaceStrictness::kPrecise);
    EXPECT_THROW({ c.ReadWhitespace(Whitespace::kSpace); }, IOError);
  }
  {  // Not newline
    std::stringstream ss(" ");
    BasicIStreamContext c(ss, WhitespaceStrictness::kPrecise);
    EXPECT_THROW({ c.ReadWhitespace(Whitespace::kNewline); }, IOError);
  }
  {  // Not tab
    std::stringstream ss(" ");
    BasicIStreamContext c(ss, WhitespaceStrictness::kPrecise);
    EXPECT_THROW({ c.ReadWhitespace(Whitespace::kTab); }, IOError);
  }
  {  // At EOF
    std::stringstream ss("");
    BasicIStreamContext c(ss, WhitespaceStrictness::kPrecise);
    EXPECT_THROW({ c.ReadWhitespace(Whitespace::kSpace); }, IOError);
  }
}

TEST(BasicIStreamTest,
     ReadWhitespaceShouldNotCareWhichWhitespaceInFlexibleMode) {
  std::stringstream ss("\t  ");
  BasicIStreamContext c(ss, WhitespaceStrictness::kFlexible);
  c.ReadWhitespace(Whitespace::kSpace);
  c.ReadWhitespace(Whitespace::kNewline);
  c.ReadWhitespace(Whitespace::kTab);
  EXPECT_THROW({ c.ReadWhitespace(Whitespace::kSpace); }, IOError);
}

TEST(BasicIStreamTest, ReadFunctionBehaveEndToEnd) {
  {
    std::stringstream ss("1 2 3");
    BasicIStreamContext c(ss, WhitespaceStrictness::kFlexible);
    EXPECT_EQ(c.ReadToken(), "1");
    EXPECT_EQ(c.ReadToken(), "2");
    EXPECT_EQ(c.ReadToken(), "3");
    EXPECT_NO_THROW({ c.ReadEof(); });
  }
  {
    std::stringstream ss("1 2 3");
    BasicIStreamContext c(ss, WhitespaceStrictness::kPrecise);
    EXPECT_EQ(c.ReadToken(), "1");
    c.ReadWhitespace(Whitespace::kSpace);
    EXPECT_EQ(c.ReadToken(), "2");
    c.ReadWhitespace(Whitespace::kSpace);
    EXPECT_EQ(c.ReadToken(), "3");
    EXPECT_NO_THROW({ c.ReadEof(); });
  }
  {
    std::stringstream ss("1 2 3");
    BasicIStreamContext c(ss, WhitespaceStrictness::kPrecise);
    EXPECT_EQ(c.ReadToken(), "1");
    EXPECT_THROW({ (void)c.ReadToken(); }, IOError);
  }
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
