#include "src/contexts/internal/view_only_context.h"

#include <functional>
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
  return *variables_.get().GetAnonymousVariable(variable_name);
}

const absl::flat_hash_map<std::string, std::unique_ptr<AbstractVariable>>&
ViewOnlyContext::ListVariables() const {
  return variables_.get().ListVariables();
}

bool ViewOnlyContext::ValueIsKnown(std::string_view variable_name) const {
  return values_.get().Contains(variable_name);
}

}  // namespace moriarty_internal
}  // namespace moriarty
