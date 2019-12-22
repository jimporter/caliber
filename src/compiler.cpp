#include "compiler.hpp"

#include <unistd.h>
#include <sys/wait.h>

#include <cassert>
#include <stdexcept>
#include <system_error>

#ifdef CALIBER_BOOST_FILESYSTEM
#  include <boost/filesystem.hpp>
#  define FILESYSTEM_NS boost::filesystem
#else
#  include <filesystem>
#  define FILESYSTEM_NS std::filesystem
#endif

#include <mettle/driver/exit_code.hpp>
#include <mettle/driver/posix/scoped_pipe.hpp>

namespace caliber {

  namespace {

    [[noreturn]] inline void child_failed() {
      _exit(mettle::exit_code::fatal);
    }

    std::string slurp(const char *argv[]) {
      mettle::posix::scoped_pipe stdout_pipe;
      if(stdout_pipe.open() < 0)
        throw std::system_error(errno, std::system_category());

      pid_t pid;
      if((pid = fork()) < 0)
        throw std::system_error(errno, std::system_category());
      if(pid == 0) {
        if(stdout_pipe.close_read() < 0 ||
           stdout_pipe.move_write(STDOUT_FILENO) < 0 ||
           dup2(STDOUT_FILENO, STDERR_FILENO) < 0)
          child_failed();

        execvp(argv[0], const_cast<char**>(argv));
        child_failed();
      } else {
        if(stdout_pipe.close_write() < 0)
          throw std::system_error(errno, std::system_category());

        std::string stdout;
        ssize_t size;
        char buf[BUFSIZ];

        do {
          if((size = read(stdout_pipe.read_fd, buf, sizeof(buf))) < 0)
            throw std::system_error(errno, std::system_category());
          stdout.append(buf, size);
        } while(size != 0);

        int status;
        if(waitpid(pid, &status, 0) < 0)
          throw std::system_error(errno, std::system_category());
        if(!WIFEXITED(status) || WEXITSTATUS(status) != 0)
          throw std::runtime_error("subprocess failed");

        return stdout;
      }
    }

    struct cc_compiler : compiler {
      cc_compiler(std::string path, std::string brand)
        : compiler(std::move(path), std::move(brand), "cc") {}

      virtual std::vector<std::string>
      translate_args(const std::string &src, const compiler_options &args,
                     const raw_options &raw_args) const override {
        auto base_path = FILESYSTEM_NS::path(src).parent_path();
        std::vector<std::string> result = {path};
        for(const auto &arg : args) {
          if(arg.string_key == "std") {
            result.push_back("-std=" + arg.value.front());
          } else if(arg.string_key == "-I") {
            result.push_back("-I" + (base_path / arg.value.front()).string());
          } else {
            result.push_back(arg.string_key);
            result.insert(result.end(), arg.value.begin(), arg.value.end());
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

    std::pair<std::string, std::string> detect_flavor(const std::string &path) {
      const char *argv[] = {path.c_str(), "--version", nullptr};
      try {
        auto stdout = slurp(argv);
        if(stdout.find("Free Software Foundation") != std::string::npos)
          return {"gcc", "cc"};
        else if(stdout.find("clang") != std::string::npos)
          return {"clang", "cc"};
        else
          return {"unknown", "cc"};
      } catch (const std::runtime_error &) {
        throw std::runtime_error("unable to determine compiler flavor");
      }
    }
  }

  std::unique_ptr<const compiler>
  make_compiler(const std::string &path) {
    // XXX: Once we support Windows, this will need to handle deciding whether
    // to make an `msvc_compiler` object or a `cc_compiler` one.
    auto [brand, flavor] = detect_flavor(path);
    if(flavor == "cc")
      return std::make_unique<cc_compiler>(path, std::move(brand));
    assert(false && "unknown compiler flavor");
  }

} // namespace caliber
