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

#include "src/variables/mvariant.h"

#include <vector>

#include "src/librarian/errors.h"
#include "src/librarian/io_config.h"

namespace moriarty {

MVariantFormat& MVariantFormat::WithSeparator(Whitespace separator) {
  separator_ = separator;
  return *this;
}

Whitespace MVariantFormat::GetSeparator() const { return separator_; }

MVariantFormat& MVariantFormat::SpaceSeparated() {
  return WithSeparator(Whitespace::kSpace);
}

MVariantFormat& MVariantFormat::NewlineSeparated() {
  return WithSeparator(Whitespace::kNewline);
}

MVariantFormat& MVariantFormat::Discriminator(
    std::vector<std::string> options) {
  for (const std::string& option : options) {
    if (option.empty()) {
      throw ConfigurationError("MVariantFormat::Discriminator",
                               "Empty string is not a valid Discriminator.");
    }
    for (char c : option) {
      if (!std::isprint(c) || std::isspace(c)) {
        throw ConfigurationError(
            "MVariantFormat::Discriminator",
            "Discriminator options cannot contain whitespace.");
      }
    }
  }

  for (size_t i = 0; i < options.size(); ++i) {
    // Compare options against their tolower versions to ensure case-insensitive
    // uniqueness.
    for (size_t j = i + 1; j < options.size(); ++j) {
      if (std::ranges::equal(options[i], options[j], [](char a, char b) {
            return std::tolower(a) == std::tolower(b);
          })) {
        throw ConfigurationError(
            "MVariantFormat::Discriminator",
            std::format(
                "Duplicate discriminator option (case-insensitive): '{}'",
                options[i]));
      }
    }
  }

  discriminator_options_ = std::move(options);
  return *this;
}

const std::vector<std::string>& MVariantFormat::GetDiscriminatorOptions()
    const {
  return discriminator_options_;
}

void MVariantFormat::Merge(const MVariantFormat& other) {
  if (other.separator_ != Whitespace::kSpace) {
    separator_ = other.separator_;
  }
  if (!other.discriminator_options_.empty()) {
    discriminator_options_ = other.discriminator_options_;
  }
}

}  // namespace moriarty
