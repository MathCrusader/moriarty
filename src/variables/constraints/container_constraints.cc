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

#include "src/variables/constraints/container_constraints.h"

#include <format>
#include <string>
#include <string_view>
#include <vector>

#include "src/variables/constraints/base_constraints.h"
#include "src/variables/minteger.h"

namespace moriarty {

// ====== Length ======
MInteger Length::GetConstraints() const { return length_; }

Length::Length(std::string_view expression) : length_(Exactly(expression)) {}

std::string Length::ToString() const {
  return std::format("has length that {}", length_.ToString());
}

std::vector<std::string> Length::GetDependencies() const {
  return length_.GetDependencies();
}

// ====== DistinctElements ======
std::string DistinctElements::ToString() const {
  return "has distinct elements";
}

std::vector<std::string> DistinctElements::GetDependencies() const {
  return {};
}

}  // namespace moriarty
