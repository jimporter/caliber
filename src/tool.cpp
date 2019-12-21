#include "tool.hpp"

#include <unistd.h>
#include <sys/wait.h>

#include <cassert>
#include <filesystem>
#include <stdexcept>
#include <system_error>

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

  std::vector<std::string> get_identity(const std::string &name) {
    const char *argv[] = {name.c_str(), "--version", nullptr};
    try {
      auto stdout = slurp(argv);
      if(stdout.find("Free Software Foundation") != std::string::npos)
        return {"gcc", "cc"};
      else if(stdout.find("clang") != std::string::npos)
        return {"clang", "cc"};
      else
        return {"cc"};
    } catch (const std::runtime_error &) {
      return {};
    }
  }
}

tool::tool(std::string filename)
  : path(std::move(filename)), identity(get_identity(path)) {}

std::vector<std::string>
translate_args(const std::string &file, const compiler_options &args) {
  // XXX: This will eventually need to support different compiler front-ends.

  auto base_path = std::filesystem::path(file).parent_path();
  std::vector<std::string> result;
  for(const auto &arg : args) {
    if(arg.string_key == "std") {
      result.push_back("-std=" + arg.value.front());
    } else if(arg.string_key == "-I") {
      result.push_back("-I");
      result.push_back(base_path / arg.value.front());
    } else {
      result.push_back(arg.string_key);
      result.insert(result.end(), arg.value.begin(), arg.value.end());
    }
  }

  result.insert(result.end(), {"-fsyntax-only", file});
  return result;
}

} // namespace caliber
