#ifndef INC_CALIBER_SRC_TEST_COMPILER_HPP
#define INC_CALIBER_SRC_TEST_COMPILER_HPP

#include <string>
#include <vector>

#include <boost/program_options/option.hpp>
#include <mettle/driver/log/core.hpp>

namespace caliber {

class test_compiler {
public:
  using args_type = std::vector<boost::program_options::option>;

  test_compiler() = default;
  test_compiler(const test_compiler &) = delete;
  test_compiler & operator =(const test_compiler &) = delete;

  int operator ()(const std::string &file, const args_type &args,
                  mettle::log::test_output &output) const;
};

} // namespace caliber

#endif
