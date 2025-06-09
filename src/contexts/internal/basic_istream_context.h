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

#ifndef MORIARTY_SRC_CONTEXTS_INTERNAL_BASIC_ISTREAM_CONTEXT_H_
#define MORIARTY_SRC_CONTEXTS_INTERNAL_BASIC_ISTREAM_CONTEXT_H_

#include <cstdint>
#include <functional>
#include <string>

#include "src/librarian/io_config.h"
#include "src/librarian/policies.h"

namespace moriarty {
namespace moriarty_internal {

// BasicIStreamContext
//
// A class to handle reading from an input stream in a uniform way.
// The held istream will have its exceptions enabled.
class BasicIStreamContext {
 public:
  explicit BasicIStreamContext(std::reference_wrapper<InputCursor> input);

  // ReadToken()
  //
  // Reads the next token in the input stream. If there is whitespace before the
  // next token:
  //
  //  * `WhitespaceStrictness::kFlexible`: leading whitespace will be ignored
  //  * `WhitespaceStrictness::kPrecise` : an exception will be thrown
  //
  // End of file will throw an exception.
  [[nodiscard]] std::string ReadToken();

  // ReadEof()
  //
  // Reads the end of file character from the input stream. If the end of file
  // character is not found, an exception is thrown.
  void ReadEof();

  // ReadWhitespace()
  //
  // Reads the next character from the input stream.
  //
  // An exception is thrown in the following cases:
  // * reading fails (e.g., EOF or failed stream),
  // * the read character is not a whitespace character,
  // * if whitespace strictness is `WhitespaceStrictness::kPrecise` and the
  //   read character is not the one provided.
  void ReadWhitespace(moriarty::Whitespace whitespace);

  // ThrowIOError()
  //
  // Throws an `IOError` exception with the current cursor position and the
  // provided message.
  void ThrowIOError(std::string_view message) const;

  // ReadInteger()
  //
  // Reads the next token from the input stream and casts it to an integer.
  int64_t ReadInteger();

  // TODO: Add helper functions. ReadSpace(), ReadEoln(), ReadTokens(),
  // ReadInt64(), etc.
 private:
  std::reference_wrapper<InputCursor> input_;

  InputCursor& GetCursor();
  std::istream& GetIStream();
  WhitespaceStrictness GetStrictness() const;

  // TODO: Determine if we should provide an API for:
  //  * update/access strictness and is_.
  //  * Query the state of is_ (e.g., IsEOF()).
};

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_CONTEXTS_INTERNAL_BASIC_ISTREAM_CONTEXT_H_
