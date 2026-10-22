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

#include "moriarty/librarian/case_normalization.h"

#include <cstddef>
#include <string>
#include <string_view>

#include "moriarty/librarian/policies.h"

namespace moriarty {
namespace librarian {

char NormalizeCase(char value, CaseNormalization normalization) {
  if (normalization == CaseNormalization::kLowercase && value >= 'A' &&
      value <= 'Z')
    return value + ('a' - 'A');
  if (normalization == CaseNormalization::kUppercase && value >= 'a' &&
      value <= 'z')
    return value - ('a' - 'A');
  return value;
}

std::string NormalizeCase(std::string_view value,
                          CaseNormalization normalization) {
  std::string result(value);
  for (char& c : result) c = NormalizeCase(c, normalization);
  return result;
}

bool case_normalization_internal::EqualText(std::string_view lhs,
                                            std::string_view rhs,
                                            CaseNormalization normalization) {
  if (normalization == CaseNormalization::kNone) return lhs == rhs;
  if (lhs.size() != rhs.size()) return false;
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    if (NormalizeCase(lhs[i], normalization) !=
        NormalizeCase(rhs[i], normalization))
      return false;
  }
  return true;
}

}  // namespace librarian
}  // namespace moriarty
