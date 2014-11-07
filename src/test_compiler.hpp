#ifndef INC_CALIBER_SRC_TEST_COMPILER_HPP
#define INC_CALIBER_SRC_TEST_COMPILER_HPP

#include <string>
#include <vector>

#include <boost/program_options/option.hpp>
#include <mettle/suite/compiled_suite.hpp>
#include <mettle/driver/log/core.hpp>

#include "detail/optional.hpp"

namespace caliber {

struct tool {
  tool(const char *filename) : tool(std::string(filename)) {}
  tool(std::string filename)
    : path(std::move(filename)), name(tool_name(path)) {}

  std::string path, name;
private:
  static std::string tool_name(const std::string &filename);
};

bool is_cxx(const std::string &file);

using compiler_args = std::vector<boost::program_options::option>;

std::vector<std::string>
translate_args(const compiler_args &args, const std::string &path);

class test_compiler {
public:
  using timeout_t = CALIBER_OPTIONAL_NS::optional<std::chrono::milliseconds>;

  test_compiler(tool cc, tool cxx, timeout_t timeout = {})
    : cc_(std::move(cc)), cxx_(std::move(cxx)), timeout_(timeout) {}
  test_compiler(const test_compiler &) = delete;
  test_compiler & operator =(const test_compiler &) = delete;

  mettle::test_result
  operator ()(const std::string &file, const compiler_args &args,
              bool expect_fail, mettle::log::test_output &output) const;
private:
  static void fork_watcher(std::chrono::milliseconds timeout);

  const tool cc_, cxx_;
  timeout_t timeout_;
};

} // namespace caliber

#endif
