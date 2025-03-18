/*
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

#include "src/variables/constraints/io_constraints.h"

#include <stdexcept>
#include <string>

#include "src/io_config.h"

namespace moriarty {

IOSeparator::IOSeparator(Whitespace separator) : separator_(separator) {}

Whitespace IOSeparator::GetSeparator() const { return separator_; }

std::string IOSeparator::ToString() const {
  switch (separator_) {
    case Whitespace::kSpace:
      return "IOSeparator(Space)";
    case Whitespace::kTab:
      return "IOSeparator(Tab)";
    case Whitespace::kNewline:
      return "IOSeparator(Newline)";
  }
  throw std::runtime_error("Invalid/Unknown IOSeparator");
}

}  // namespace moriarty
