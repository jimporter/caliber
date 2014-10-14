#ifndef INC_CALIBER_SRC_CMD_LINE_HPP
#define INC_CALIBER_SRC_CMD_LINE_HPP

#include <istream>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

namespace caliber {

inline std::vector<std::string>
extract_comment(std::istream &s, const std::string &name) {
  std::string line;
  std::getline(s, line);

  // XXX: Improve comment parsing!
  if(line.find("// " + name) == 0)
    return boost::program_options::split_unix(line.substr(3));
  else
    return {name};
}

class comment_parser : public boost::program_options::command_line_parser {
private:
  using base_type = boost::program_options::command_line_parser;
public:
  comment_parser(std::istream &&s, const std::string &name)
    : base_type(extract_comment(s, name)) {}
  comment_parser(std::istream &s, const std::string &name)
    : base_type(extract_comment(s, name)) {}
};

inline boost::program_options::parsed_options
parse_comment(std::istream &s, const std::string &name,
              const boost::program_options::options_description &opts) {
  return comment_parser(s, name).options(opts).run();
}

inline boost::program_options::parsed_options
parse_comment(std::istream &&s, const std::string &name,
              const boost::program_options::options_description &opts) {
  return parse_comment(s, name, opts);
}

struct per_file_options {
  bool expect_fail = false;
  std::string name;
};

inline boost::program_options::options_description
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

#endif
