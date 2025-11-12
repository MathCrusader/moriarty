#include "src/contexts/internal/view_only_context.h"

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

const ValueSet& ViewOnlyContext::UnsafeGetValues() const {
  return values_.get();
}

const VariableSet& ViewOnlyContext::UnsafeGetVariables() const {
  return variables_.get();
}

}  // namespace moriarty_internal
}  // namespace moriarty
