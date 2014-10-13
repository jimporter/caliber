#include "run_test_files.hpp"

#include "test_compiler.hpp"

namespace caliber {

namespace detail {
  void run_test_file(
    const std::string &file, mettle::log::test_logger &logger,
    const mettle::filter_set &/*filter*/, test_compiler &compiler
  ) {
    const mettle::test_name name = {{"Compilation tests"}, file, 0};
    logger.started_test(name);

    mettle::log::test_output output;

    using namespace std::chrono;
    auto then = steady_clock::now();
    int err = compiler(file, output);
    auto now = steady_clock::now();
    auto duration = duration_cast<mettle::log::test_duration>(now - then);

    if(!err)
      logger.passed_test(name, output, duration);
    else
      logger.failed_test(name, "Compilation failed", output, duration);
  }
}

void run_test_files(
  const std::vector<std::string> &files, mettle::log::test_logger &logger,
  const mettle::filter_set &filter
) {
  logger.started_run();
  logger.started_suite({"Compilation tests"});

  test_compiler compiler("/tmp/caliber-XXXXXX");

  for(const auto &file : files)
    detail::run_test_file(file, logger, filter, compiler);

  logger.ended_suite({"Compilation tests"});
  logger.ended_run();
}

} // namespace caliber
