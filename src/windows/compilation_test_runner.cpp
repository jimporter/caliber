#include "../compilation_test_runner.hpp"

#include <stdexcept>

#include <mettle/output.hpp>

namespace caliber {

  mettle::test_result
  compilation_test_runner::operator ()(
    const std::string &file, const compiler_options &args,
    const raw_options &raw_args, bool expect_fail,
    mettle::log::test_output &output
  ) const {
    throw std::runtime_error("not implemented");
  }

} // namespace caliber
