#include "run_test_files.hpp"

#include <fstream>
#include <iostream>

#include <boost/program_options.hpp>

#include "cmd_line.hpp"
#include "test_compiler.hpp"

namespace caliber {

namespace {
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
    for(const auto &option : parsed.options) {
      if(desc.find_nothrow(option.string_key, false))
        filtered.push_back(option);
    }
    return filtered;
  }

  bool tool_match_any(const tool &t, const std::vector<std::string> &names) {
    if(names.empty())
      return true;
    for(const auto &name : names) {
      if(tool_match(t, name))
        return true;
    }
    return false;
  }

  void run_test_file(
    const std::vector<std::string> &test_suite, const std::string &file,
    mettle::log::test_logger &logger, const test_compiler &compiler,
    const mettle::filter_set &filter
  ) {
    mettle::test_name name = {test_suite, file, generate_id()};
    mettle::log::test_output output;

    per_file_options args;
    compiler_options comp_args;
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
      comp_args = filter_options(parsed, compiler_opts);
    } catch(const std::exception &e) {
      logger.started_test(name);
      return logger.failed_test(
        name, std::string("Invalid command: ") + e.what(), output,
        mettle::log::test_duration(0)
      );
    }

    if(!args.name.empty())
      name.test = std::move(args.name);

    auto attrs = make_attributes(args.attrs);
    auto action = filter(name, attrs);
    if(action.action == mettle::test_action::indeterminate)
      action = filter_by_attr(attrs);

    if(action.action == mettle::test_action::hide)
      return;

    logger.started_test(name);

    if(action.action == mettle::test_action::skip)
      return logger.skipped_test(name, action.message);
    if(!tool_match_any(compiler.tool(), args.tools)) {
      return logger.skipped_test(
        name, "test skipped for " + compiler.tool().identity[0]
      );
    }

    using namespace std::chrono;
    auto then = steady_clock::now();
    auto result = compiler(file, comp_args, args.raw_args, args.expect_fail,
                           output);
    auto now = steady_clock::now();
    auto duration = duration_cast<mettle::log::test_duration>(now - then);

    if(result.passed)
      logger.passed_test(name, output, duration);
    else
      logger.failed_test(name, result.message, output, duration);
  }
}

void run_test_files(
  const std::string &suite_name, const std::vector<std::string> &files,
  mettle::log::test_logger &logger, const test_compiler &compiler,
  const mettle::filter_set &filter
) {
  const std::vector<std::string> test_suite = {suite_name};

  logger.started_run();
  logger.started_suite(test_suite);

  for(const auto &file : files)
    run_test_file(test_suite, file, logger, compiler, filter);

  logger.ended_suite(test_suite);
  logger.ended_run();
}

} // namespace caliber
