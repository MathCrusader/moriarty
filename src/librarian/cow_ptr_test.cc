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

#include "src/librarian/cow_ptr.h"

#include "gtest/gtest.h"

namespace moriarty {
namespace librarian {
namespace {

// Simple Integer wrapper that counts the number of copies made.
class Integer {
 public:
  Integer() : value_(0) {}
  explicit Integer(int value) : value_(value) {}

  Integer(const Integer& other) : value_(other.value_) { copy_counter_++; }

  Integer& operator=(const Integer&) {
    copy_counter_++;
    return *this;
  }

  Integer(Integer&&) = default;
  Integer& operator=(Integer&&) = default;
  ~Integer() = default;

  int Value() const { return value_; }
  int NumCopies() const { return copy_counter_; }

  void UpdateValue(int new_value) { value_ = new_value; }

 private:
  int value_;
  int copy_counter_ = 0;
};

TEST(CowPtrTest, DefaultConstructorWorks) {
  CowPtr<Integer> M;

  EXPECT_EQ(M->Value(), 0);
}

TEST(CowPtrTest, CopyWithNoWriteWorks) {
  CowPtr<Integer> M(Integer(5));
  CowPtr<Integer> M2 = M;

  EXPECT_EQ(M->Value(), 5);
  EXPECT_EQ(M2->Value(), 5);
}

TEST(CowPtrTest, CopyWithNoWriteDoesntCopy) {
  CowPtr<Integer> M(Integer(5));
  CowPtr<Integer> M2 = M;

  EXPECT_EQ(M->NumCopies(), 0);
  EXPECT_EQ(M2->NumCopies(), 0);
}

TEST(CowPtrTest, WriteWithOnlyOneEntityDoesntCopy) {
  CowPtr<Integer> M(Integer(5));
  M.Mutable().UpdateValue(10);

  EXPECT_EQ(M->NumCopies(), 0);
  EXPECT_EQ(M->Value(), 10);
}

TEST(CowPtrTest, WriteWithOnlyMultipleEntitiesCopies) {
  CowPtr<Integer> M(Integer(5));
  CowPtr<Integer> M2 = M;
  M.Mutable().UpdateValue(10);

  EXPECT_EQ(M->NumCopies(), 1);
  EXPECT_EQ(M2->NumCopies(), 0);

  EXPECT_EQ(M->Value(), 10);
  EXPECT_EQ(M2->Value(), 5);
}

TEST(CowPtrTest, NoCopyIfUniqueObject) {
  CowPtr<Integer> M(Integer(5));
  CowPtr<Integer> M2 = M;
  M.Mutable().UpdateValue(10);
  M2.Mutable().UpdateValue(20);

  EXPECT_EQ(M->NumCopies(), 1);
  EXPECT_EQ(M2->NumCopies(), 0);  // Key check

  EXPECT_EQ(M->Value(), 10);
  EXPECT_EQ(M2->Value(), 20);
}

}  // namespace
}  // namespace librarian
}  // namespace moriarty
