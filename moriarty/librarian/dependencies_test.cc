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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace moriarty {
namespace {

using ::testing::ElementsAre;
using ::testing::IsEmpty;

TEST(DependenciesTest, GetDependenciesReturnsInitialDependencies) {
  Dependencies deps({"a", "b", "c"});
  EXPECT_THAT(deps.GetDependencies(), ElementsAre("a", "b", "c"));
  EXPECT_THAT(Dependencies().GetDependencies(), IsEmpty());
}

TEST(DependenciesTest, MergeCombinesDependencies) {
  Dependencies deps1({"a", "b"});
  Dependencies deps2({"b", "c"});
  Dependencies deps3 = deps1;
  deps1.Merge(deps2);
  EXPECT_THAT(deps1.GetDependencies(), ElementsAre("a", "b", "c"));
  EXPECT_THAT(deps2.GetDependencies(), ElementsAre("b", "c"));
  EXPECT_THAT(deps3.GetDependencies(), ElementsAre("a", "b"));
}

}  // namespace
}  // namespace moriarty
