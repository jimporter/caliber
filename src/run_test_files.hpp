#ifndef INC_CALIBER_SRC_RUN_TEST_FILES_HPP
#define INC_CALIBER_SRC_RUN_TEST_FILES_HPP

#include <mettle/driver/filters.hpp>
#include <mettle/driver/log/core.hpp>

#include "test_compiler.hpp"

namespace caliber {

  void run_test_files(
    const std::string &suite_name, const std::vector<std::string> &files,
    mettle::log::test_logger &logger, const test_compiler &compiler,
    const mettle::filter_set &filter
  );

} // namespace caliber

#endif
