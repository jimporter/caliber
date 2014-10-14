#include "cmd_line.hpp"

namespace caliber {

namespace detail {
  inline std::string readline(std::istream &is, char delim = '\n') {
    std::string line;
    std::getline(is, line, delim);
    return line;
  }
}

std::vector<std::string>
extract_comment(std::istream &is, const std::string &name) {
  if(is.get() == '/') {
    char c = is.get();
    if(c == '/') {
      return boost::program_options::split_unix(detail::readline(is));
    }
    else if(c == '*') {
      std::string comment;
      while(true) {
        comment += detail::readline(is, '*');
        if(is.peek() == '/')
          return boost::program_options::split_unix(comment, " \t\n\r");
        else
          comment += '*';
      }
    }
  }

  return {name};
}

boost::program_options::options_description
make_per_file_options(per_file_options &opts) {
  using namespace boost::program_options;
  options_description child("Per-file options");
  child.add_options()
    ("fail,F", value(&opts.expect_fail)->zero_tokens(),
     "expect the test to fail")
    ("name,n", value(&opts.name), "the test's name")
  ;
  return child;
}

} // namespace caliber
