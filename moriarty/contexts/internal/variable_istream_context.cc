#include "moriarty/contexts/internal/variable_istream_context.h"

#include <memory>

#include "moriarty/internal/abstract_variable.h"
#include "moriarty/internal/value_set.h"
#include "moriarty/internal/variable_set.h"
#include "moriarty/librarian/io_config.h"
#include "moriarty/librarian/util/ref.h"
#include "moriarty/test_case.h"

namespace moriarty {
namespace moriarty_internal {

VariableIStreamContext::VariableIStreamContext(Ref<InputCursor> input,
                                               Ref<const VariableSet> variables,
                                               Ref<const ValueSet> values)
    : variables_(variables), values_(values), input_(input) {}

void VariableIStreamContext::ReadVariableTo(std::string_view variable_name,
                                            TestCase& test_case) {
  const AbstractVariable* variable =
      variables_.get().GetAnonymousVariable(variable_name);
  variable->ReadValue(variable_name, input_, variables_,
                      test_case.UnsafeGetValues());
}

std::unique_ptr<moriarty_internal::ChunkedReader>
VariableIStreamContext::GetChunkedReader(std::string_view variable_name,
                                         int calls, TestCase& test_case) const {
  const AbstractVariable* variable =
      variables_.get().GetAnonymousVariable(variable_name);
  return variable->GetChunkedReader(variable_name, calls, input_, variables_,
                                    test_case.UnsafeGetValues());
}

}  // namespace moriarty_internal
}  // namespace moriarty
