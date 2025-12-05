// Copyright 2025 Darcy Best
// Copyright 2024 Google LLC
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

#ifndef MORIARTY_SRC_INTERNAL_EXPRESSIONS_H_
#define MORIARTY_SRC_INTERNAL_EXPRESSIONS_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "absl/numeric/int128.h"

namespace moriarty::moriarty_internal {
class ExpressionProgram;
}  // namespace moriarty::moriarty_internal

namespace moriarty {

class Expression {
 public:
  using LookupFn = std::function<int64_t(std::string_view)>;

  explicit Expression(std::string_view str);

  [[nodiscard]] int64_t Evaluate() const;

  [[nodiscard]] int64_t Evaluate(LookupFn lookup_variable) const;

  [[nodiscard]] std::string ToString() const;

  [[nodiscard]] std::vector<std::string> GetDependencies() const;

  [[nodiscard]] bool operator==(const Expression& other) const;

 private:
  std::shared_ptr<const moriarty_internal::ExpressionProgram> program_;
};

namespace moriarty_internal {

class ExpressionProgram {
 public:
  using LookupFn = std::function<int64_t(std::string_view)>;

  ExpressionProgram(const ExpressionProgram&) = delete;
  ExpressionProgram& operator=(const ExpressionProgram&) = delete;

  [[nodiscard]] absl::int128 Evaluate(const LookupFn& fn) const;
  [[nodiscard]] std::string_view ExpressionString() const;
  [[nodiscard]] const std::vector<std::string>& Dependencies() const;
  static std::shared_ptr<const ExpressionProgram> Parse(
      std::string_view expression);

  enum class NodeKind {
    kInteger,
    kVariable,
    kBinaryAdd,
    kBinarySubtract,
    kBinaryMultiply,
    kBinaryDivide,
    kBinaryModulo,
    kBinaryExponentiate,
    kUnaryPlus,
    kUnaryNegate,
    kFunction,
  };

  enum class SingleArgFunction { kAbs };

  enum class MultiArgFunction { kMax, kMin };

  [[nodiscard]] size_t AddIntegerNode(std::string_view span);
  [[nodiscard]] size_t AddVariableNode(std::string_view span);
  [[nodiscard]] size_t AddUnaryNode(NodeKind kind, size_t child,
                                    std::string_view op_span);
  [[nodiscard]] size_t AddBinaryNode(NodeKind kind, size_t lhs, size_t rhs,
                                     std::string_view op_span);
  [[nodiscard]] size_t AddFunctionNode(std::string_view fn_name_span,
                                       std::vector<size_t> args,
                                       std::string_view total_span);
  void Finalize(size_t root_index);

  [[nodiscard]] std::string_view NodeSpan(size_t index) const;
  void UpdateSpan(size_t index, std::string_view span);

 private:
  struct Node {
    struct IntegerData {
      absl::int128 value;
    };
    struct VariableData {
      std::string name;
    };
    struct BinaryData {
      size_t lhs;
      size_t rhs;
    };
    struct UnaryData {
      size_t child;
    };
    struct SingleArgFunctionData {
      SingleArgFunction function;
      size_t argument;
    };
    struct MultiArgFunctionData {
      MultiArgFunction function;
      std::vector<size_t> arguments;
    };
    using Payload =
        std::variant<IntegerData, VariableData, BinaryData, UnaryData,
                     SingleArgFunctionData, MultiArgFunctionData>;

    NodeKind kind;
    std::string_view span;
    Payload payload;
  };

  explicit ExpressionProgram(std::string_view expression);

  absl::int128 EvaluateNode(size_t index, const LookupFn& fn) const;

  std::string expression_;
  std::vector<Node> nodes_;
  size_t root_index_;
  std::vector<std::string> dependencies_;
};

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_EXPRESSIONS_H_
