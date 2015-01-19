#include "paths.hpp"

#include <poll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <stdexcept>
#include <system_error>

#include <mettle/driver/posix/scoped_pipe.hpp>

namespace caliber {

std::string which(const std::string &command) {
  mettle::scoped_pipe stdout_pipe;
  if(stdout_pipe.open() < 0)
    throw std::system_error(errno, std::system_category());

  pid_t pid;
  if((pid = fork()) < 0)
    throw std::system_error(errno, std::system_category());
  if(pid == 0) {
    if(stdout_pipe.close_read() < 0)
      _exit(128);

    if(dup2(stdout_pipe.write_fd, STDOUT_FILENO) < 0)
      _exit(128);

    if(stdout_pipe.close_write() < 0)
      _exit(128);

    execlp("which", "which", "--", command.c_str(), nullptr);
    _exit(128);
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

std::string parent_path(const CALIBER_STRING_VIEW &filename) {
  size_t slash = filename.rfind('/');
  if(slash != CALIBER_STRING_VIEW::npos)
    return std::string(filename.substr(0, slash + 1));
  else
    return "";
}

std::string leafname(const CALIBER_STRING_VIEW &filename) {
  size_t slash = filename.rfind('/');
  if(slash != CALIBER_STRING_VIEW::npos)
    return std::string(filename.substr(slash + 1));
  else
    return std::string(filename);
}

} // namespace caliber
