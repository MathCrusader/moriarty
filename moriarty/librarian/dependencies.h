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

#ifndef MORIARTY_LIBRARIAN_DEPENDENCIES_H_
#define MORIARTY_LIBRARIAN_DEPENDENCIES_H_

#include <string>
#include <vector>

#include "moriarty/librarian/util/cow_ptr.h"

namespace moriarty {

// Dependencies
//
// Represents a set of variables that an object depends on.
class Dependencies {
 public:
  using VariableName = std::string;

  // Creates an empty set of dependencies.
  Dependencies() = default;
  // Creates a set of dependencies from a list of variable names.
  explicit Dependencies(std::vector<VariableName> dependencies);

  // Merges another set of dependencies into this one.
  void Merge(const Dependencies& other);

  // Returns the list of variable names that we depend on.
  const std::vector<VariableName>& GetDependencies() const;

 private:
  librarian::CowPtr<std::vector<VariableName>> deps_;
};

}  // namespace moriarty

#endif  // MORIARTY_LIBRARIAN_DEPENDENCIES_H_
