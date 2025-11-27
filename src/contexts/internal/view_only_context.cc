#include "src/contexts/internal/view_only_context.h"

#include <cstdint>
#include <format>
#include <optional>

#include "src/internal/expressions.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/util/ref.h"

namespace moriarty {
namespace moriarty_internal {

ViewOnlyContext::ViewOnlyContext(Ref<const VariableSet> variables,
                                 Ref<const ValueSet> values)
    : variables_(variables), values_(values) {}

const AbstractVariable& ViewOnlyContext::GetAnonymousVariable(
    std::string_view variable_name) const {
  return *variables_.get().GetAnonymousVariable(variable_name);
}

const VariableSet::Map& ViewOnlyContext::ListVariables() const {
  return variables_.get().ListVariables();
}

bool ViewOnlyContext::ValueIsKnown(std::string_view variable_name) const {
  return values_.get().Contains(variable_name);
}

std::optional<int64_t> ViewOnlyContext::GetUniqueInteger(
    std::string_view variable_name) const {
  if (ValueIsKnown(variable_name)) {
    std::any value = values_.get().UnsafeGet(variable_name);
    try {
      return std::any_cast<int64_t>(value);
    } catch (const std::bad_any_cast&) {
      return std::nullopt;
    }
  }

  return GetAnonymousVariable(variable_name)
      .UniqueInteger(variable_name, variables_, values_);
}

int64_t ViewOnlyContext::EvaluateExpression(const Expression& expr) const {
  return expr.Evaluate([&](std::string_view variable_name) {
    auto value = GetUniqueInteger(variable_name);
    if (!value) {
      throw std::runtime_error(std::format(
          "Cannot evaluate expression because variable '{}' does not have a "
          "unique integer value.",
          variable_name));
    }
    return *value;
  });
}

const ValueSet& ViewOnlyContext::UnsafeGetValues() const {
  return values_.get();
}

const VariableSet& ViewOnlyContext::UnsafeGetVariables() const {
  return variables_.get();
}

}  // namespace moriarty_internal
}  // namespace moriarty
