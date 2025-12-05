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

#ifndef MORIARTY_LIBRARIAN_LIB_REF_H_
#define MORIARTY_LIBRARIAN_LIB_REF_H_

#include <functional>

namespace moriarty {

// Ref
//
// A reference that is intended to be stored short-term. We use Ref<T> instead
// of T& to signify this abnormal behaviour.
//
// See comments on each function to see the exact lifetime, but most are passed
// in just until a small task is completed (generate, read, etc).
template <typename T>
using Ref = std::reference_wrapper<T>;

}  // namespace moriarty

#endif  // MORIARTY_LIBRARIAN_LIB_REF_H_
