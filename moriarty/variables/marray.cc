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

#include "moriarty/variables/marray.h"

#include "moriarty/librarian/io_config.h"

namespace moriarty {

MArrayFormat& MArrayFormat::WithSeparator(Whitespace separator) {
  separator_ = separator;
  return *this;
}

Whitespace MArrayFormat::GetSeparator() const {
  return separator_.value_or(Whitespace::kSpace);
}

MArrayFormat& MArrayFormat::SpaceSeparated() {
  return WithSeparator(Whitespace::kSpace);
}

MArrayFormat& MArrayFormat::NewlineSeparated() {
  return WithSeparator(Whitespace::kNewline);
}

MArrayFormat& MArrayFormat::LengthPrefixed(Whitespace after_length) {
  length_prefix_separator_ = after_length;
  return *this;
}

std::optional<Whitespace> MArrayFormat::GetLengthPrefixSeparator() const {
  return length_prefix_separator_;
}

void MArrayFormat::Merge(const MArrayFormat& other) {
  if (other.separator_) separator_ = other.separator_;
  if (other.length_prefix_separator_)
    length_prefix_separator_ = other.length_prefix_separator_;
}

}  // namespace moriarty
