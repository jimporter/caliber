#include "cmd_line.hpp"

namespace caliber {

namespace detail {
  inline std::string readline(std::istream &is, char delim = '\n') {
    std::string line;
    std::getline(is, line, delim);
    return line;
  }

  inline bool matches_name(std::istream &is, const std::string &name,
                           const char *ws = " \t") {
    while(strchr(ws, is.peek()))
      is.get();
    for(auto c : name) {
      if(is.get() != c)
        return false;
    }
    return strchr(ws, is.get());
  }
}

std::vector<std::string>
extract_comment(std::istream &is, const std::string &name) {
  if(is.get() == '/') {
    char c = is.get();
    if(c == '/') {
      if(detail::matches_name(is, name))
        return boost::program_options::split_unix(detail::readline(is));
    }
    else if(c == '*') {
      if(detail::matches_name(is, name, " \t\n\r")) {
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
  }

  return {};
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
