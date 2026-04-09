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

#include "moriarty/librarian/dependencies.h"

#include <algorithm>
#include <ranges>
#include <vector>

namespace moriarty {

Dependencies::Dependencies(std::vector<VariableName> dependencies)
    : deps_(std::move(dependencies)) {
  auto& deps = deps_.Mutable();
  std::ranges::sort(deps);
  auto last = std::ranges::unique(deps);
  deps.erase(last.begin(), last.end());
}

void Dependencies::Merge(const Dependencies& other) {
  // We expect these lists to be very small.
  auto& deps = deps_.Mutable();
  deps.insert(deps.end(), other.deps_->begin(), other.deps_->end());
  std::ranges::sort(deps);
  auto last = std::ranges::unique(deps);
  deps.erase(last.begin(), last.end());
}

const std::vector<Dependencies::VariableName>& Dependencies::GetDependencies()
    const {
  return *deps_;
}

}  // namespace moriarty
