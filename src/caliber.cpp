#include <cstdlib>
#include <iostream>
#include <vector>

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/program_options.hpp>

#include <mettle/driver/cmd_line.hpp>
#include <mettle/driver/log/child.hpp>
#include <mettle/driver/log/summary.hpp>
#include <mettle/driver/log/term.hpp>

#include "cmd_line.hpp"
#include "run_test_files.hpp"
#include "test_compiler.hpp"

namespace caliber {

namespace {
  struct all_options : mettle::generic_options, mettle::output_options,
                       mettle::child_options {
    METTLE_OPTIONAL_NS::optional<int> child_fd;
    std::vector<std::string> files;
  };

  const char program_name[] = "caliber";
  void report_error(const std::string &message) {
    std::cerr << program_name << ": " << message << std::endl;
  }
}

} // namespace caliber

int main(int argc, const char *argv[]) {
  using namespace mettle;
  namespace opts = boost::program_options;

  auto factory = make_logger_factory();

  caliber::all_options args;
  auto generic = make_generic_options(args);
  auto output = make_output_options(args, factory);
  auto child = make_child_options(args);

  opts::options_description hidden("Hidden options");
  hidden.add_options()
    ("child", opts::value(&args.child_fd), "run this file as a child process")
    ("file", opts::value(&args.files), "input file")
  ;
  opts::positional_options_description pos;
  pos.add("file", -1);

  opts::variables_map vm;
  try {
    opts::options_description all;
    all.add(generic).add(output).add(child).add(hidden);
    auto parsed = opts::command_line_parser(argc, argv)
      .options(all).positional(pos).run();

    opts::store(parsed, vm);
    opts::notify(vm);
  } catch(const std::exception &e) {
    caliber::report_error(e.what());
    return 2;
  }

  if(args.show_help) {
    caliber::per_file_options tmp;
    auto per_file = caliber::make_per_file_options(tmp);
    per_file.add(caliber::make_compiler_options());

    opts::options_description displayed;
    displayed.add(generic).add(output).add(child).add(per_file);
    std::cout << displayed << std::endl;
    return 0;
  }

  if(args.files.empty()) {
    caliber::report_error("no inputs specified");
    return 1;
  }

  try {
    auto cxx = getenv("CXX");
    caliber::test_compiler compiler(cxx ? cxx : "c++", args.timeout);

    if(args.child_fd) {
      if(auto output_opt = has_option(output, vm)) {
        using namespace opts::command_line_style;
        caliber::report_error(output_opt->canonical_display_name(allow_long) +
                              " can't be used with --child");
        return 2;
      }

      namespace io = boost::iostreams;
      io::stream<io::file_descriptor_sink> fds(
        *args.child_fd, io::never_close_handle
      );
      log::child logger(fds);
      caliber::run_test_files(args.files, logger, compiler, args.filters);
      return 0;
    }

    if(args.no_fork && args.show_terminal) {
      caliber::report_error("--show-terminal requires forking tests");
      return 2;
    }

    term::enable(std::cout, args.color);
    indenting_ostream out(std::cout);

    log::summary logger(
      out, factory.make(args.output, out, args), args.show_time,
      args.show_terminal
    );
    caliber::run_test_files(args.files, logger, compiler, args.filters);

    logger.summarize();
    return !logger.good();
  }
  catch(const std::exception &e) {
    caliber::report_error(e.what());
    return 3;
  }
}
