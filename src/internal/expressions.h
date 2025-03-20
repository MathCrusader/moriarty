/*
 * Copyright 2025 Darcy Best
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

#ifndef MORIARTY_SRC_INTERNAL_EXPRESSIONS_H_
#define MORIARTY_SRC_INTERNAL_EXPRESSIONS_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "absl/numeric/int128.h"

namespace moriarty::moriarty_internal {
class ExpressionNode;  // Forward declaring ExpressionNode
}  // namespace moriarty::moriarty_internal

namespace moriarty {

class Expression {
 public:
  explicit Expression(std::string_view str);

  [[nodiscard]] int64_t Evaluate() const;

  [[nodiscard]] int64_t Evaluate(
      std::function<int64_t(std::string_view)> lookup_variable) const;

  [[nodiscard]] std::string ToString() const;

  [[nodiscard]] std::vector<std::string> GetDependencies() const;

 private:
  // Note: We are using a shared_ptr here since this type is completely
  // immutable after construction. It is passed to several functions, but since
  // it is immutable, the copying is overkill.
  std::shared_ptr<moriarty_internal::ExpressionNode> expr_;
  std::string str_;
  std::vector<std::string> dependencies_;
};

namespace moriarty_internal {

class ExpressionNode {
 public:
  using LookupFn = std::function<int64_t(std::string_view)>;
  virtual ~ExpressionNode() = default;

  [[nodiscard]] virtual absl::int128 Evaluate(const LookupFn& fn) const = 0;

  // These functions are only safe to call during parsing. After that, they are
  // undefined behaviour.
  [[nodiscard]] std::string_view ToString() const;
  void SetString(std::string_view new_prefix);
  std::vector<std::string> ReleaseDependencies();
  void AddDependencies(std::vector<std::string>&& dependencies);

 protected:
  ExpressionNode(std::string_view str);

 private:
  std::string_view str_;
  std::vector<std::string> dependencies_;
};

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_EXPRESSIONS_H_
