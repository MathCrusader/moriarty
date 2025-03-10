#include "src/contexts/internal/variable_istream_context.h"

#include <istream>

#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/io_config.h"

namespace moriarty {
namespace moriarty_internal {

VariableIStreamContext::VariableIStreamContext(
    std::istream& is, WhitespaceStrictness whitespace_strictness,
    const VariableSet& variables, const ValueSet& values)
    : variables_(variables),
      values_(values),
      is_(is),
      whitespace_strictness_(whitespace_strictness) {}

void VariableIStreamContext::ReadVariableTo(std::string_view variable_name,
                                            ConcreteTestCase& test_case) {
  ValueSet values;
  MutableValuesContext mutable_values_ctx(values);
  librarian::ReaderContext reader_ctx(
      variable_name, is_, whitespace_strictness_, variables_, values);

  absl::StatusOr<const AbstractVariable*> variable =
      variables_.get().GetAbstractVariable(variable_name);
  if (!variable.ok()) throw std::runtime_error(variable.status().ToString());

  auto status = (*variable)->ReadValue(reader_ctx, mutable_values_ctx);
  if (!status.ok()) throw std::runtime_error(status.ToString());

  // * hides a StatusOr<>
  test_case.UnsafeSetAnonymousValue(variable_name,
                                    *values.UnsafeGet(variable_name));
}

}  // namespace moriarty_internal
}  // namespace moriarty
