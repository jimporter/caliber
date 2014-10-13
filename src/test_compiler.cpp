#include "test_compiler.hpp"

#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>

#include <mettle/driver/scoped_pipe.hpp>

namespace caliber {

namespace {
  inline int parent_failed() {
    return 1;
  }

  [[noreturn]] inline void child_failed() {
    _exit(128);
  }
}

test_compiler::test_compiler(std::string temp_dir)
  : temp_dir_(std::move(temp_dir)) {
  mkdtemp(&temp_dir_.front());
}

test_compiler::~test_compiler() {
  // XXX: Jesus Christ how horrifying.
  system(("rm -rf '" + temp_dir_ + "'").c_str());
}

int test_compiler::operator ()(const std::string &file,
                               mettle::log::test_output &output) {
  mettle::scoped_pipe stdout_pipe, stderr_pipe;
  if(stdout_pipe.open() < 0 ||
     stderr_pipe.open() < 0)
    return parent_failed();

  fflush(nullptr);

  pid_t pid;
  if((pid = fork()) < 0)
    return parent_failed();

  if(pid == 0) {
    if(stdout_pipe.close_read() < 0 ||
       stderr_pipe.close_read() < 0)
      child_failed();

    if(dup2(stdout_pipe.write_fd, STDOUT_FILENO) < 0 ||
       dup2(stderr_pipe.write_fd, STDERR_FILENO) < 0)
      child_failed();

    if(stdout_pipe.close_write() < 0 ||
       stderr_pipe.close_write() < 0)
      child_failed();

    char abs_file[PATH_MAX+1];
    if(realpath(file.c_str(), abs_file) == 0)
      child_failed();

    if(chdir(temp_dir_.c_str()) < 0)
      child_failed();

    execlp("clang++", "clang++", "-c", abs_file, nullptr);
    child_failed();
  }
  else {
    if(stdout_pipe.close_write() < 0 ||
       stderr_pipe.close_write() < 0)
      return parent_failed();

    ssize_t size;
    char buf[BUFSIZ];

    // Read from the piped stdout and stderr.
    int rv;
    pollfd fds[2] = { {stdout_pipe.read_fd, POLLIN, 0},
                      {stderr_pipe.read_fd, POLLIN, 0} };
    std::string *dests[] = {&output.stdout, &output.stderr};
    int open_fds = 2;
    while(open_fds && (rv = poll(fds, 2, -1)) > 0) {
      for(size_t i = 0; i < 2; i++) {
        if(fds[i].revents & POLLIN) {
          if((size = read(fds[i].fd, buf, sizeof(buf))) < 0)
            return parent_failed();
          dests[i]->append(buf, size);
        }
        if(fds[i].revents & POLLHUP) {
          fds[i].fd = -fds[i].fd;
          open_fds--;
        }
      }
    }
    if(rv < 0) // poll() failed!
      return parent_failed();

    int status;
    if(waitpid(pid, &status, 0) < 0)
      return parent_failed();

    if(WIFEXITED(status))
      return WEXITSTATUS(status);
    else
      return 128;
  }
}

} // namespace caliber
