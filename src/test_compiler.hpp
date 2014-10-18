#ifndef INC_CALIBER_SRC_TEST_COMPILER_HPP
#define INC_CALIBER_SRC_TEST_COMPILER_HPP

#include <string>
#include <vector>

#include <boost/program_options/option.hpp>
#include <mettle/suite/compiled_suite.hpp>
#include <mettle/driver/log/core.hpp>

#include "detail/optional.hpp"

namespace caliber {

class test_compiler {
public:
  using timeout_t = CALIBER_OPTIONAL_NS::optional<std::chrono::milliseconds>;
  using args_type = std::vector<boost::program_options::option>;

  test_compiler(std::string cc, std::string cxx,
                timeout_t timeout = {})
    : cc_(cc), cxx_(cxx), timeout_(timeout) {}
  test_compiler(const test_compiler &) = delete;
  test_compiler & operator =(const test_compiler &) = delete;

  mettle::test_result
  operator ()(const std::string &file, const args_type &args, bool expect_fail,
              mettle::log::test_output &output) const;
private:
  static void fork_watcher(std::chrono::milliseconds timeout);
  static bool is_cxx(const std::string &file);

  std::string cc_, cxx_;
  timeout_t timeout_;
};

} // namespace caliber

#endif
