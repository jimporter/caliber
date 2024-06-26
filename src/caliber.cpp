#include <cstdlib>
#include <iostream>
#include <vector>

#define NOMINMAX

// Ignore warnings about deprecated implicit copy constructor.
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wdeprecated"
#endif

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

#include <boost/program_options.hpp>

#include <mettle/driver/cmd_line.hpp>
#include <mettle/driver/exit_code.hpp>
#include <mettle/driver/log/child.hpp>
#include <mettle/driver/log/summary.hpp>
#include <mettle/driver/log/term.hpp>
#include <mettle/driver/subprocess_test_runner.hpp>

#include "cmd_line.hpp"
#include "run_test_files.hpp"
#include "compilation_test_runner.hpp"

namespace caliber {

  namespace {
    struct all_options : mettle::generic_options, mettle::driver_options,
                         mettle::output_options {
      all_options() {
        auto cxx = getenv("CXX");
        compiler = cxx ? cxx : "c++";
      }

      std::string suite_name = "compilation tests";
      std::string compiler;
      std::optional<mettle::fd_type> output_fd;
      std::vector<std::string> files;
    };

    const char program_name[] = "caliber";
    void report_error(const std::string &message) {
      std::cerr << program_name << ": " << message << std::endl;
    }

    std::vector<std::string> split_command(const std::string &command) {
#ifndef _WIN32
      return boost::program_options::split_unix(command);
#else
      return boost::program_options::split_winmain(command);
#endif
    }
  }

} // namespace caliber

int main(int argc, const char *argv[]) {
  using namespace mettle;
  namespace opts = boost::program_options;

  auto factory = make_logger_factory();

  caliber::all_options args;
  auto generic = make_generic_options(args);
  auto driver = make_driver_options(args);
  auto output = make_output_options(args, factory);

  driver.add_options()
    ("suite-name,S", opts::value(&args.suite_name)->value_name("NAME"),
     "the name of the suite containing these tests")
    ("compiler", opts::value(&args.compiler)->value_name("CMD"),
     "the compiler to use for these tests")
  ;

  opts::options_description hidden("Hidden options");
  hidden.add_options()
    ("output-fd", opts::value(&args.output_fd),
     "pipe the results to this file descriptor")
    ("input-file", opts::value(&args.files), "input file")
  ;
  opts::positional_options_description pos;
  pos.add("input-file", -1);

  opts::variables_map vm;
  try {
    opts::options_description all;
    all.add(generic).add(driver).add(output).add(hidden);
    auto parsed = opts::command_line_parser(argc, argv)
      .options(all).positional(pos).run();

    opts::store(parsed, vm);
    opts::notify(vm);
  } catch(const std::exception &e) {
    caliber::report_error(e.what());
    return exit_code::bad_args;
  }

  if(args.show_help) {
    caliber::per_file_options tmp;
    auto per_file = caliber::make_per_file_options(tmp);
    per_file.add(caliber::make_compiler_options());

    opts::options_description displayed;
    displayed.add(generic).add(driver).add(output).add(per_file);
    std::cout << displayed << std::endl;
    return exit_code::success;
  }

  if(args.files.empty()) {
    caliber::report_error("no inputs specified");
    return exit_code::no_inputs;
  }

  try {
    caliber::compilation_test_runner runner(
      caliber::make_compiler(caliber::split_command(args.compiler)),
      args.timeout
    );

    if(args.output_fd) {
      if(auto output_opt = has_option(output, vm)) {
        using namespace opts::command_line_style;
        caliber::report_error(output_opt->canonical_display_name(allow_long) +
                              " can't be used with --output-fd");
        return exit_code::bad_args;
      }

      make_fd_private(*args.output_fd);
      namespace io = boost::iostreams;
      io::stream<io::file_descriptor_sink> fds(
        *args.output_fd, io::never_close_handle
      );
      log::child logger(fds);
      caliber::run_test_files(args.suite_name, args.files, logger, runner,
                              args.filters);
      return exit_code::success;
    }

    term::enable(std::cout, color_enabled(args.color));
    indenting_ostream out(std::cout);

    log::summary logger(
      out, factory.make(args.output, out, args), args.show_time,
      args.show_terminal
    );
    caliber::run_test_files(args.suite_name, args.files, logger, runner,
                            args.filters);

    logger.summarize();
    return logger.good() ? exit_code::success : exit_code::failure;
  } catch(const std::exception &e) {
    caliber::report_error(e.what());
    return exit_code::unknown_error;
  }
}
