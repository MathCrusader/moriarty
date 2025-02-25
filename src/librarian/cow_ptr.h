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

#ifndef MORIARTY_SRC_INTERNAL_LIB_COW_PTR_H_
#define MORIARTY_SRC_INTERNAL_LIB_COW_PTR_H_

#include <memory>

namespace moriarty {
namespace librarian {

// Copy-On-Write Pointer
//
// This pointer will never be nullptr. It will call the default constructor if
// needed.
//
// By default, all operations do not create a copy of the object. If you want to
// modify the object, you must call `Mutable()`. This will create a copy of the
// object if it is shared with other pointers.
template <typename T>
class CowPtr {
 public:
  CowPtr();
  explicit CowPtr(T ptr);

  // Returns a reference to the managed object. It is guaranteed that you are
  // the only one touching the object.
  T& Mutable();

  // Returns a const-reference to the managed object. It is *not* guaranteed
  // that you are the only one looking at the object.
  const T& operator*() const;

  // Returns a const-pointer to the managed object. It is *not* guaranteed that
  // you are the only one looking at the object.
  const T* operator->() const;

 private:
  std::shared_ptr<T> ptr_;

  void Detach();
};

// -----------------------------------------------------------------------------
//  Template implementation below

template <typename T>
CowPtr<T>::CowPtr() : ptr_(std::make_shared<T>()) {}

template <typename T>
CowPtr<T>::CowPtr(T ptr) : ptr_(std::make_shared<T>(std::move(ptr))) {}

template <typename T>
T& CowPtr<T>::Mutable() {
  if (ptr_ && !ptr_.unique()) Detach();
  return *ptr_;
}

template <typename T>
const T& CowPtr<T>::operator*() const {
  return *ptr_;
}

template <typename T>
const T* CowPtr<T>::operator->() const {
  return ptr_.get();
}

template <typename T>
void CowPtr<T>::Detach() {
  if (ptr_) ptr_ = std::make_shared<T>(*ptr_);
}

}  // namespace librarian
}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_LIB_COW_PTR_H_
