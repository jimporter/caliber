#ifndef INC_CALIBER_SRC_CMD_LINE_HPP
#define INC_CALIBER_SRC_CMD_LINE_HPP

#include <istream>
#include <string>
#include <vector>

#include <boost/program_options.hpp>
#include <mettle/suite/attributes.hpp>

#include "tool.hpp"

namespace caliber {

  std::vector<std::string>
  extract_comment(std::istream &is, const std::string &name);

  class comment_parser : public boost::program_options::command_line_parser {
  private:
    using base_type = boost::program_options::command_line_parser;
  public:
    comment_parser(std::istream &s, const std::string &name)
      : base_type(extract_comment(s, name)) {}
    comment_parser(std::istream &&s, const std::string &name)
      : comment_parser(s, name) {}
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
    std::vector<std::string> attrs;
    std::vector<std::string> tools;
    raw_options raw_args;
  };

  boost::program_options::options_description
  make_per_file_options(per_file_options &opts);

  boost::program_options::options_description
  make_compiler_options();

  mettle::attributes make_attributes(const std::vector<std::string> &attrs);

  void validate(boost::any &, const std::vector<std::string> &, raw_option *,
                int);

} // namespace caliber

#endif
