#ifndef INC_CALIBER_SRC_TEST_COMPILER_HPP
#define INC_CALIBER_SRC_TEST_COMPILER_HPP

#include <string>

#include <mettle/driver/log/core.hpp>

namespace caliber {

class test_compiler {
public:
  test_compiler(std::string temp_dir);
  test_compiler(const test_compiler &) = delete;
  test_compiler & operator =(const test_compiler &) = delete;

  ~test_compiler();

  int operator ()(const std::string &file, mettle::log::test_output &output);
private:
  std::string temp_dir_;
};

} // namespace caliber

#endif
