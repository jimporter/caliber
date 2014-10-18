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

  template<typename Char>
  std::vector<boost::program_options::basic_option<Char>> filter_options(
    const boost::program_options::basic_parsed_options<Char> &parsed,
    const boost::program_options::options_description &desc
  ) {
    std::vector<boost::program_options::basic_option<Char>> filtered;
    for(auto &&option : parsed.options) {
      if(desc.find_nothrow(option.string_key, false))
        filtered.push_back(option);
    }
    return filtered;
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
    test_compiler::args_type compiler_args;
    try {
      namespace opts = boost::program_options;
      auto options = make_per_file_options(args);
      auto compiler_opts = make_compiler_options();
      options.add(compiler_opts);

      opts::positional_options_description pos;
      auto parsed = comment_parser(std::ifstream(file), "caliber")
        .options(options).positional(pos).run();

      opts::variables_map vm;
      opts::store(parsed, vm);
      opts::notify(vm);
      compiler_args = filter_options(parsed, compiler_opts);
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
    auto result = compiler(file, compiler_args, args.expect_fail, output);
    auto now = steady_clock::now();
    auto duration = duration_cast<mettle::log::test_duration>(now - then);

    if(result.passed)
      logger.passed_test(name, output, duration);
    else
      logger.failed_test(name, result.message, output, duration);
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
