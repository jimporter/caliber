#ifndef INC_CALIBER_SRC_TOOL_HPP
#define INC_CALIBER_SRC_TOOL_HPP

#include <algorithm>
#include <string>
#include <vector>

#include <boost/program_options/option.hpp>

namespace caliber {

struct tool {
  tool(const char *filename) : tool(std::string(filename)) {}
  tool(std::string filename);

  std::string path;
  std::vector<std::string> identity;
};

inline bool tool_match(const tool &t, const std::string &name) {
  return std::find(t.identity.begin(), t.identity.end(), name) !=
    t.identity.end();
}

struct raw_option {
  std::string tool;
  std::string value;
};

using compiler_options = std::vector<boost::program_options::option>;
using raw_options = std::vector<raw_option>;

std::vector<std::string>
translate_args(const std::string &file, const compiler_options &args);

} // namespace caliber

#endif
