#include "tool.hpp"

#include <regex>
#include <stdexcept>

#include "paths.hpp"

namespace caliber {

namespace {

  struct tool_def {
    std::regex match;
    std::vector<std::string> parents;
  };

  std::vector<std::string> get_identity(const std::string &name)  {
    static std::vector<tool_def> known = {
      { std::regex(R"/(.*clang\+\+(-\d(\.\d)?)?)/"), {"clang++", "c++"} },
      { std::regex(R"/(.*clang(-\d(\.\d)?)?)/"),     {"clang",   "cc" } },
      { std::regex(R"/(.*g\+\+(-\d(\.\d)?)?)/"),     {"g++",     "c++"} },
      { std::regex(R"/(.*gcc(-\d(\.\d)?)?)/"),       {"gcc",     "cc" } }
    };

    for(const auto &i : known) {
      if(std::regex_match(name, i.match)) {
        std::vector<std::string> result = {name};
        result.insert(result.end(), i.parents.begin(), i.parents.end());
        return result;
      }
    }
    return {name};
  }

  std::string tool_name(const std::string &filename) {
    std::unique_ptr<char, void (*)(void*)> linkname(
      realpath(which(filename).c_str(), nullptr),
      std::free
    );
    if(!linkname)
      throw std::system_error(errno, std::system_category());

    return leafname(linkname.get());
  }
}

tool::tool(std::string filename)
  : path(std::move(filename)), identity(get_identity(tool_name(path))) {}

std::vector<std::string>
translate_args(const std::string &file, const compiler_options &args,
               const std::string &base_path) {
  // XXX: This will eventually need to support different compiler front-ends.
  std::vector<std::string> result;
  for(const auto &arg : args) {
    if(arg.string_key == "std") {
      result.push_back("-std=" + arg.value.front());
    }
    else if(arg.string_key == "-I") {
      result.push_back("-I");
      result.push_back(base_path + arg.value.front());
    }
    else {
      result.push_back(arg.string_key);
      result.insert(result.end(), arg.value.begin(), arg.value.end());
    }
  }

  result.insert(result.end(), {"-fsyntax-only", file});
  return result;
}

} // namespace caliber
