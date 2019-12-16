#include "paths.hpp"

#include <poll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <stdexcept>
#include <system_error>

#include <mettle/driver/exit_code.hpp>
#include <mettle/driver/posix/scoped_pipe.hpp>

namespace caliber {

namespace {
  [[noreturn]] inline void child_failed() {
    _exit(mettle::exit_code::fatal);
  }
}

std::string which(const std::string &command) {
  mettle::posix::scoped_pipe stdout_pipe;
  if(stdout_pipe.open() < 0)
    throw std::system_error(errno, std::system_category());

  pid_t pid;
  if((pid = fork()) < 0)
    throw std::system_error(errno, std::system_category());
  if(pid == 0) {
    if(stdout_pipe.close_read() < 0)
      child_failed();

    if(dup2(stdout_pipe.write_fd, STDOUT_FILENO) < 0)
      child_failed();

    if(stdout_pipe.close_write() < 0)
      child_failed();

    execlp("which", "which", "--", command.c_str(), nullptr);
    child_failed();
  }
  else {
    if(stdout_pipe.close_write() < 0)
      throw std::system_error(errno, std::system_category());

    std::string path;
    ssize_t size;
    char buf[BUFSIZ];

    do {
      if((size = read(stdout_pipe.read_fd, buf, sizeof(buf))) < 0)
        throw std::system_error(errno, std::system_category());
      path.append(buf, size);
    } while(size != 0);

    int status;
    if(waitpid(pid, &status, 0) < 0)
      throw std::system_error(errno, std::system_category());
    if(!WIFEXITED(status) || WEXITSTATUS(status) != 0)
      throw std::runtime_error("unable to locate command `" + command + "`");

    if(path.back() == '\n')
      path.pop_back();
    return path;
  }
}

std::string parent_path(const std::string_view &filename) {
  size_t slash = filename.rfind('/');
  if(slash != std::string_view::npos)
    return std::string(filename.substr(0, slash + 1));
  else
    return "";
}

std::string leafname(const std::string_view &filename) {
  size_t slash = filename.rfind('/');
  if(slash != std::string_view::npos)
    return std::string(filename.substr(slash + 1));
  else
    return std::string(filename);
}

} // namespace caliber
