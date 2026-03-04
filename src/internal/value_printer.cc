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

#include "src/internal/value_printer.h"

#include <format>
#include <string>
#include <string_view>

namespace moriarty {
namespace moriarty_internal {

std::string InternalValuePrinterString(std::string_view value, int max_len) {
  if (value.empty()) return "\"\"";
  max_len -= 2;  // for the quotes
  std::string prefix = "\"", suffix = "\"";
  if (value.size() > max_len) max_len -= 3, suffix = "...\"";
  max_len = std::max(max_len, 2);  // Always show at least 2 characters
  std::string result;
  for (int i = 0; i < value.size() && result.size() < max_len; i++)
    result += InternalValuePrinterChar(static_cast<unsigned char>(value[i]));
  return std::format("{}{}{}", prefix, result, suffix);
}

}  // namespace moriarty_internal
}  // namespace moriarty
