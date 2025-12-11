#include "src/contexts/internal/variable_ostream_context.h"

#include <ostream>
#include <string_view>

#include "src/internal/abstract_variable.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/util/ref.h"
#include "src/test_case.h"

namespace moriarty {
namespace moriarty_internal {

VariableOStreamContext::VariableOStreamContext(Ref<std::ostream> os,
                                               Ref<const VariableSet> variables,
                                               Ref<const ValueSet> values)
    : variables_(variables), values_(values), os_(os) {}

void VariableOStreamContext::PrintVariable(std::string_view variable_name) {
  const AbstractVariable* variable =
      variables_.get().GetAnonymousVariable(variable_name);

  variable->PrintValue(variable_name, os_, variables_, values_);
}

void VariableOStreamContext::PrintVariableFrom(std::string_view variable_name,
                                               const TestCase& test_case) {
  const AbstractVariable* variable =
      variables_.get().GetAnonymousVariable(variable_name);

  ValueSet values = UnsafeExtractTestCaseInternals(test_case);
  variable->PrintValue(variable_name, os_, variables_, values);
}

}  // namespace moriarty_internal
}  // namespace moriarty
