#include "run_test_files.hpp"

#include <fstream>
#include <iostream>
#include <boost/program_options.hpp>

#include "cmd_line.hpp"
#include "test_compiler.hpp"

namespace caliber {

namespace detail {
  inline uint64_t generate_id() {
    static std::atomic<uint64_t> id_(0);
    return id_++;
  }

  namespace {
    const std::vector<std::string> test_suite = {"Compilation tests"};
  }

  void run_test_file(
    const std::string &file, mettle::log::test_logger &logger,
    const test_compiler &compiler, const mettle::filter_set &/*filter*/
  ) {
    mettle::test_name name = {test_suite, file, generate_id()};
    mettle::log::test_output output;

    per_file_options args;
    try {
      namespace opts = boost::program_options;
      opts::positional_options_description pos;
      auto options = make_per_file_options(args);
      auto parsed = comment_parser(std::ifstream(file), "caliber")
        .options(options).positional(pos).run();

      opts::variables_map vm;
      opts::store(parsed, vm);
      opts::notify(vm);
    } catch(const std::exception &e) {
      logger.started_test(name);
      logger.failed_test(name, std::string("Invalid command: ") + e.what(),
                         output, mettle::log::test_duration(0));
      return;
    }

    if(!args.name.empty())
      name.test = std::move(args.name);

    logger.started_test(name);

    using namespace std::chrono;
    auto then = steady_clock::now();
    int err = compiler(file, output);
    auto now = steady_clock::now();
    auto duration = duration_cast<mettle::log::test_duration>(now - then);

    if(bool(err) == args.expect_fail)
      logger.passed_test(name, output, duration);
    else if(err)
      logger.failed_test(name, "Compilation failed", output, duration);
    else
      logger.failed_test(name, "Compilation successful", output, duration);
  }
}

void run_test_files(
  const std::vector<std::string> &files, mettle::log::test_logger &logger,
  const test_compiler &compiler, const mettle::filter_set &filter
) {
  logger.started_run();
  logger.started_suite(detail::test_suite);

  for(const auto &file : files)
    detail::run_test_file(file, logger, compiler, filter);

  logger.ended_suite(detail::test_suite);
  logger.ended_run();
}

} // namespace caliber
