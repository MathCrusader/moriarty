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

#ifndef MORIARTY_SRC_VARIABLES_MNONE_H_
#define MORIARTY_SRC_VARIABLES_MNONE_H_

#include <string>

#include "src/contexts/librarian_context.h"
#include "src/librarian/errors.h"
#include "src/librarian/mvariable.h"
#include "src/types/no_type.h"

namespace moriarty {

class MNone;
namespace librarian {
template <>
struct MVariableValueTypeTrait<MNone> {
  using type = NoType;
};
}  // namespace librarian

// MNone
//
// A placeholder variable that represents no value. You may not do anything with
// it.
class MNone : public librarian::MVariable<MNone> {
 public:
  ~MNone() override = default;

  [[nodiscard]] std::string Typename() const override { return "MNone"; }

  struct CoreConstraints {};

 private:
  // ---------------------------------------------------------------------------
  //  MVariable overrides
  NoType GenerateImpl(librarian::ResolverContext ctx) const override {
    throw ConfigurationError(
        "MNone::Generate",
        "MNone variable cannot be generated as it represents no value.");
  }
  NoType ReadImpl(librarian::ReaderContext ctx) const override {
    throw ConfigurationError(
        "MNone::Read",
        "MNone variable cannot be read as it represents no value.");
  }
  void PrintImpl(librarian::PrinterContext ctx,
                 const NoType& value) const override {
    throw ConfigurationError(
        "MNone::Print",
        "MNone variable cannot be printed as it represents no value.");
  }
  // ---------------------------------------------------------------------------
};

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_MNONE_H_
