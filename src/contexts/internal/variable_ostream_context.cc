#include "src/contexts/internal/variable_ostream_context.h"

#include <functional>
#include <ostream>
#include <string_view>

#include "src/internal/abstract_variable.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/test_case.h"

namespace moriarty {
namespace moriarty_internal {

VariableOStreamContext::VariableOStreamContext(
    std::reference_wrapper<std::ostream> os,
    std::reference_wrapper<const VariableSet> variables,
    std::reference_wrapper<const ValueSet> values)
    : variables_(variables), values_(values), os_(os) {}

void VariableOStreamContext::PrintVariable(std::string_view variable_name) {
  const AbstractVariable* variable =
      variables_.get().GetAnonymousVariable(variable_name);

  variable->PrintValue(variable_name, os_, variables_, values_);
}

void VariableOStreamContext::PrintVariableFrom(
    std::string_view variable_name, const ConcreteTestCase& test_case) {
  const AbstractVariable* variable =
      variables_.get().GetAnonymousVariable(variable_name);

  ValueSet values = UnsafeExtractConcreteTestCaseInternals(test_case);
  variable->PrintValue(variable_name, os_, variables_, values);
}

}  // namespace moriarty_internal
}  // namespace moriarty
