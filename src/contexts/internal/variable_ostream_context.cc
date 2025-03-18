#include "src/contexts/internal/variable_ostream_context.h"

#include <functional>
#include <ostream>
#include <stdexcept>
#include <string_view>

#include "absl/status/statusor.h"
#include "src/contexts/librarian/printer_context.h"
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
  absl::StatusOr<const AbstractVariable*> variable =
      variables_.get().GetAbstractVariable(variable_name);
  if (!variable.ok()) throw std::runtime_error(variable.status().ToString());

  librarian::PrinterContext ctx(variable_name, os_, variables_, values_);
  (*variable)->PrintValue(ctx);
}

void VariableOStreamContext::PrintVariableFrom(
    std::string_view variable_name, const ConcreteTestCase& test_case) {
  absl::StatusOr<const AbstractVariable*> variable =
      variables_.get().GetAbstractVariable(variable_name);
  if (!variable.ok()) throw std::runtime_error(variable.status().ToString());

  ValueSet values = UnsafeExtractConcreteTestCaseInternals(test_case);
  librarian::PrinterContext ctx(variable_name, os_, variables_, values);
  (*variable)->PrintValue(ctx);
}

}  // namespace moriarty_internal
}  // namespace moriarty
