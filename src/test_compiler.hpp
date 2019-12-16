#ifndef INC_CALIBER_SRC_TEST_COMPILER_HPP
#define INC_CALIBER_SRC_TEST_COMPILER_HPP

#include <chrono>
#include <optional>

#include <mettle/driver/log/core.hpp>
#include <mettle/suite/compiled_suite.hpp>

#include "tool.hpp"

namespace caliber {

class test_compiler {
public:
  using timeout_t = std::optional<std::chrono::milliseconds>;

  test_compiler(caliber::tool compiler, timeout_t timeout = {})
    : compiler_(std::move(compiler)), timeout_(timeout) {}
  test_compiler(const test_compiler &) = delete;
  test_compiler & operator =(const test_compiler &) = delete;

  mettle::test_result
  operator ()(const std::string &file, const compiler_options &args,
              const raw_options &raw_args, bool expect_fail,
              mettle::log::test_output &output) const;

  const caliber::tool & tool() const {
    return compiler_;
  }
private:
  const struct caliber::tool compiler_;
  timeout_t timeout_;
};

} // namespace caliber

#endif
