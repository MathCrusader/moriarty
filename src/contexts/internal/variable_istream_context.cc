#include "src/contexts/internal/variable_istream_context.h"

#include <memory>

#include "src/internal/abstract_variable.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/io_config.h"
#include "src/test_case.h"

namespace moriarty {
namespace moriarty_internal {

VariableIStreamContext::VariableIStreamContext(
    std::reference_wrapper<InputCursor> input,
    std::reference_wrapper<const VariableSet> variables,
    std::reference_wrapper<const ValueSet> values)
    : variables_(variables), values_(values), input_(input) {}

void VariableIStreamContext::ReadVariableTo(std::string_view variable_name,
                                            ConcreteTestCase& test_case) {
  ValueSet values = UnsafeExtractConcreteTestCaseInternals(test_case);
  const AbstractVariable* variable =
      variables_.get().GetAnonymousVariable(variable_name);
  variable->ReadValue(variable_name, input_, variables_, values);
  test_case.UnsafeSetAnonymousValue(variable_name,
                                    values.UnsafeGet(variable_name));
}

std::unique_ptr<moriarty_internal::PartialReader>
VariableIStreamContext::GetPartialReader(std::string_view variable_name,
                                         int calls,
                                         ConcreteTestCase& test_case) const {
  const AbstractVariable* variable =
      variables_.get().GetAnonymousVariable(variable_name);
  return variable->GetPartialReader(variable_name, calls, input_, variables_,
                                    test_case.UnsafeGetValues());
}

}  // namespace moriarty_internal
}  // namespace moriarty
