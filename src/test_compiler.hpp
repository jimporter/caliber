#ifndef INC_CALIBER_SRC_TEST_COMPILER_HPP
#define INC_CALIBER_SRC_TEST_COMPILER_HPP

#include <chrono>
#include <optional>

#include <mettle/driver/log/core.hpp>
#include <mettle/suite/compiled_suite.hpp>

#include "compiler.hpp"

namespace caliber {

  class test_compiler {
  public:
    using timeout_t = std::optional<std::chrono::milliseconds>;

    test_compiler(std::unique_ptr<const caliber::compiler> compiler,
                  timeout_t timeout = {})
      : compiler_(std::move(compiler)), timeout_(timeout) {}
    test_compiler(const test_compiler &) = delete;
    test_compiler & operator =(const test_compiler &) = delete;

    mettle::test_result
    operator ()(const std::string &file, const compiler_options &args,
                const raw_options &raw_args, bool expect_fail,
                mettle::log::test_output &output) const;

    const caliber::compiler & compiler() const {
      return *compiler_;
    }
  private:
    std::unique_ptr<const caliber::compiler> compiler_;
    timeout_t timeout_;
  };

} // namespace caliber

#endif
