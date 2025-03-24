#include "src/contexts/internal/variable_istream_context.h"

#include <istream>

#include "src/contexts/internal/mutable_values_context.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/policies.h"

namespace moriarty {
namespace moriarty_internal {

VariableIStreamContext::VariableIStreamContext(
    std::reference_wrapper<std::istream> is,
    WhitespaceStrictness whitespace_strictness,
    std::reference_wrapper<const VariableSet> variables,
    std::reference_wrapper<const ValueSet> values)
    : variables_(variables),
      values_(values),
      is_(is),
      whitespace_strictness_(whitespace_strictness) {}

void VariableIStreamContext::ReadVariableTo(std::string_view variable_name,
                                            ConcreteTestCase& test_case) {
  ValueSet values = values_;
  MutableValuesContext mutable_values_ctx(values);
  librarian::ReaderContext reader_ctx(
      variable_name, is_, whitespace_strictness_, variables_, values);

  const AbstractVariable* variable =
      variables_.get().GetAnonymousVariable(variable_name);

  variable->ReadValue(reader_ctx, mutable_values_ctx);

  test_case.UnsafeSetAnonymousValue(variable_name,
                                    values.UnsafeGet(variable_name));
}

}  // namespace moriarty_internal
}  // namespace moriarty
