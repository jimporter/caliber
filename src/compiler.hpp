#ifndef INC_CALIBER_SRC_COMPILER_HPP
#define INC_CALIBER_SRC_COMPILER_HPP

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <boost/program_options/option.hpp>

namespace caliber {

  struct raw_option {
    std::string flavor;
    std::string value;
  };

  using compiler_options = std::vector<boost::program_options::option>;
  using raw_options = std::vector<raw_option>;

  struct compiler {
    compiler(std::string path, std::vector<std::string> flavor);

    std::string path;
    std::vector<std::string> flavor;

    inline bool match_flavor(const std::string &query) const {
      return std::find(flavor.begin(), flavor.end(), query) != flavor.end();
    }

    // Eventually, this should be virtual so we can implement different flavors
    // of compiler (i.e. msvc vs cc).
    std::vector<std::string>
    translate_args(const std::string &src, const compiler_options &args,
                   const raw_options &raw_args) const;
  };

  std::unique_ptr<compiler>
  make_compiler(const std::string &path);

} // namespace caliber

#endif
