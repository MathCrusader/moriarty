/*
 * Copyright 2025 Darcy Best
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

#ifndef MORIARTY_SRC_LIBRARIAN_CONVERSIONS_H_
#define MORIARTY_SRC_LIBRARIAN_CONVERSIONS_H_

#include "src/internal/abstract_variable.h"
#include "src/librarian/errors.h"

namespace moriarty {
namespace librarian {

// ConvertTo<>
//
//  Converts an AbstractVariable to some derived variable type. Throws
//  MVariableTypeMismatch on failures.
template <typename Type>
Type& ConvertTo(moriarty_internal::AbstractVariable& var) {
  Type* typed_var = dynamic_cast<Type*>(&var);
  if (typed_var == nullptr) {
    throw MVariableTypeMismatch(var.Typename(), Type().Typename());
  }
  return *typed_var;
}

// ConvertTo<>
//
//  Converts an AbstractVariable to some derived variable type. Throws
//  MVariableTypeMismatch on failures.
template <typename Type>
const Type& ConvertTo(const moriarty_internal::AbstractVariable& var) {
  const Type* typed_var = dynamic_cast<const Type*>(&var);
  if (typed_var == nullptr) {
    throw MVariableTypeMismatch(var.Typename(), Type().Typename());
  }
  return *typed_var;
}

}  // namespace librarian
}  // namespace moriarty

#endif  // MORIARTY_SRC_LIBRARIAN_CONVERSIONS_H_
