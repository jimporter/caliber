#include "subprocess.hpp"

#include <unistd.h>
#include <sys/wait.h>

#include <stdexcept>
#include <system_error>

#include <mettle/driver/exit_code.hpp>
#include <mettle/driver/posix/scoped_pipe.hpp>

namespace caliber::posix {

  namespace {
    [[noreturn]] inline void child_failed() {
      _exit(mettle::exit_code::fatal);
    }
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

} // namespace caliber::posix
