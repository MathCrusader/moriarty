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

#include <ostream>

#include "src/io_config.h"

namespace moriarty {
namespace moriarty_internal {

BasicOStreamContext::BasicOStreamContext(
    std::reference_wrapper<std::ostream> os)
    : os_(os) {
  os_.get().exceptions(std::ostream::failbit | std::ostream::badbit);
}

void BasicOStreamContext::PrintToken(std::string_view token) {
  os_.get() << token;
}

void BasicOStreamContext::PrintWhitespace(Whitespace whitespace) {
  switch (whitespace) {
    case Whitespace::kSpace:
      os_.get() << ' ';
      break;
    case Whitespace::kNewline:
      os_.get() << '\n';
      break;
    case Whitespace::kTab:
      os_.get() << '\t';
      break;
  }
}

}  // namespace moriarty_internal
}  // namespace moriarty
