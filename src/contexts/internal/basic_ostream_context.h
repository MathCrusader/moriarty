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

#ifndef MORIARTY_CONTEXTS_INTERNAL_BASIC_OSTREAM_CONTEXT_H_
#define MORIARTY_CONTEXTS_INTERNAL_BASIC_OSTREAM_CONTEXT_H_

#include <ostream>
#include <string_view>

#include "src/librarian/io_config.h"
#include "src/librarian/util/ref.h"

namespace moriarty {
namespace moriarty_internal {

// BasicOStreamContext
//
// A class to handle writing tokens to an output stream in a uniform way.
// The held ostream will have its exceptions enabled.
class BasicOStreamContext {
 public:
  explicit BasicOStreamContext(Ref<std::ostream> os);

  // WriteToken()
  //
  // Writes a single token to the output stream.
  void WriteToken(std::string_view token);

  // WriteWhitespace()
  //
  // Writes the whitespace character to the output stream.
  void WriteWhitespace(moriarty::Whitespace whitespace);

 protected:
  void UpdateBasicOStream(Ref<std::ostream> os) { os_ = os; }

 private:
  Ref<std::ostream> os_;

  // TODO: Determine if we should provide an API for:
  //  * update/access os_.
};

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_CONTEXTS_INTERNAL_BASIC_OSTREAM_CONTEXT_H_
