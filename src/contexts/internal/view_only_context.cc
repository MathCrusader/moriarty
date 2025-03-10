#include "src/contexts/internal/view_only_context.h"

#include <functional>
#include <stdexcept>
#include <string>

#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"

namespace moriarty {
namespace moriarty_internal {

ViewOnlyContext::ViewOnlyContext(
    std::reference_wrapper<const VariableSet> variables,
    std::reference_wrapper<const ValueSet> values)
    : variables_(variables), values_(values) {}

const AbstractVariable& ViewOnlyContext::GetAnonymousVariable(
    std::string_view variable_name) const {
  auto var = variables_.get().GetAbstractVariable(variable_name);
  if (!var.ok()) throw std::runtime_error(std::string(var.status().message()));
  return *(var.value());
}

const absl::flat_hash_map<std::string, std::unique_ptr<AbstractVariable>>&
ViewOnlyContext::GetAllVariables() const {
  return variables_.get().GetAllVariables();
}

bool ViewOnlyContext::ValueIsKnown(std::string_view variable_name) const {
  return values_.get().Contains(variable_name);
}

}  // namespace moriarty_internal
}  // namespace moriarty
