#ifndef INC_CALIBER_SRC_TOOL_HPP
#define INC_CALIBER_SRC_TOOL_HPP

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

using compiler_args = std::vector<boost::program_options::option>;

std::vector<std::string>
translate_args(const compiler_args &args, const std::string &path);

} // namespace caliber

#endif
