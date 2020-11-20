#include "compiler.hpp"

#include <cassert>
#include <stdexcept>

#ifndef _WIN32
#  include "posix/subprocess.hpp"
namespace platform = caliber::posix;
#else
#  include "windows/subprocess.hpp"
namespace platform = caliber::windows;
#endif

#ifdef CALIBER_BOOST_FILESYSTEM
#  include <boost/filesystem.hpp>
#  define FILESYSTEM_NS boost::filesystem
#else
#  include <filesystem>
#  define FILESYSTEM_NS std::filesystem
#endif

namespace caliber {

  namespace {

    struct cc_compiler : compiler {
      cc_compiler(std::vector<std::string> command, std::string brand)
        : compiler(std::move(command), std::move(brand), "cc") {}

      virtual std::vector<std::string>
      translate_args(const std::string &src, const compiler_options &args,
                     const raw_options &raw_args) const override {
        auto base_path = FILESYSTEM_NS::path(src).parent_path();
        std::vector<std::string> result = command;
        for(const auto &arg : args) {
          if(arg.string_key == "std") {
            result.push_back("-std=" + arg.value.front());
          } else if(arg.string_key == "-I") {
            result.push_back("-I" + (base_path / arg.value.front()).string());
          } else if(arg.string_key == "-D" || arg.string_key == "-U") {
            result.push_back(arg.string_key + arg.value.front());
          } else {
            assert(false && "unrecognized compiler option");
          }
        }

        for(const auto &arg : raw_args) {
          if(match_flavor(arg.flavor))
            result.push_back(arg.value);
        }

        result.insert(result.end(), {"-fsyntax-only", src});
        return result;
      }
    };

    struct msvc_compiler : compiler {
      msvc_compiler(std::vector<std::string> command, std::string brand)
        : compiler(std::move(command), std::move(brand), "msvc") {}

      virtual std::vector<std::string>
      translate_args(const std::string &src, const compiler_options &args,
                     const raw_options &raw_args) const override {
        auto base_path = FILESYSTEM_NS::path(src).parent_path();
        std::vector<std::string> result = command;
        for(const auto &arg : args) {
          if(arg.string_key == "std") {
            result.push_back("/std:" + arg.value.front());
          } else if(arg.string_key == "-I") {
            result.push_back("/I" + (base_path / arg.value.front()).string());
          } else if(arg.string_key == "-D" || arg.string_key == "-U") {
            result.push_back("/" + arg.string_key.substr(1) +
                             arg.value.front());
          } else {
            assert(false && "unrecognized compiler option");
          }
        }

        for(const auto &arg : raw_args) {
          if(match_flavor(arg.flavor))
            result.push_back(arg.value);
        }

        result.insert(result.end(), {"/Zs", src});
        return result;
      }
    };

    std::string
    call_detect(const std::vector<std::string> &cmd, const char *arg) {
      auto argv = std::make_unique<const char *[]>(cmd.size() + 2);
      for(size_t i = 0; i != cmd.size(); i++)
        argv[i] = const_cast<char*>(cmd[i].c_str());
      argv[cmd.size()] = arg;
      argv[cmd.size() + 1] = nullptr;

      return platform::slurp(argv.get());
    }

    std::pair<std::string, std::string>
    detect_flavor(const std::vector<std::string> &command) {
      try {
        auto output = call_detect(command, "-?");
        if(output.find("Microsoft (R)") != std::string::npos)
          return {"msvc", "msvc"};
        else if(output.find("clang LLVM compiler") != std::string::npos)
          return {"clang-cl", "msvc"}; // XXX: Maybe brand this as "clang"?
        else
          return {"unknown", "msvc"};
      } catch (const std::runtime_error &) {
        try {
          auto output = call_detect(command, "--version");
          if(output.find("Free Software Foundation") != std::string::npos)
            return {"gcc", "cc"};
          else if(output.find("clang") != std::string::npos)
            return {"clang", "cc"};
          else
            return {"unknown", "cc"};
        } catch (const std::runtime_error &) {
          throw std::runtime_error("unable to determine compiler flavor");
        }
      }
    }

  }

// MSVC doesn't understand [[noreturn]], so just ignore the warning here.
#if defined(_MSC_VER) && !defined(__clang__)
#  pragma warning(push)
#  pragma warning(disable:4715)
#endif

  std::unique_ptr<const compiler>
  make_compiler(const std::vector<std::string> &command) {
    auto [brand, flavor] = detect_flavor(command);
    if(flavor == "cc")
      return std::make_unique<cc_compiler>(command, std::move(brand));
    else if(flavor == "msvc")
      return std::make_unique<msvc_compiler>(command, std::move(brand));

    assert(false && "unknown compiler flavor");
  }

#if defined(_MSC_VER) && !defined(__clang__)
#  pragma warning(pop)
#endif

} // namespace caliber
