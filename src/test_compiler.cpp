#include "test_compiler.hpp"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <regex>
#include <sstream>

#include <mettle/driver/scoped_pipe.hpp>
#include <mettle/driver/scoped_sig_intr.hpp>
#include <mettle/driver/test_monitor.hpp>
#include <mettle/output.hpp>

#include "paths.hpp"

namespace caliber {

std::unique_ptr<char *[]>
make_argv(const std::vector<std::string> &argv) {
  auto real_argv = std::make_unique<char *[]>(argv.size() + 1);
  for(size_t i = 0; i != argv.size(); i++)
    real_argv[i] = const_cast<char*>(argv[i].c_str());
  return real_argv;
}

inline std::string err_string(int errnum) {
  char buf[256];
#ifdef _GNU_SOURCE
  return strerror_r(errnum, buf, sizeof(buf));
#else
  if(strerror_r(errnum, buf, sizeof(buf)) < 0)
    return "";
  return buf;
#endif
}

inline mettle::test_result parent_failed() {
  return { false, err_string(errno) };
}

[[noreturn]] inline void child_failed() {
  _exit(128);
}

mettle::test_result
test_compiler::operator ()(
  const std::string &file, const compiler_options &args,
  const raw_options &raw_args, bool expect_fail,
  mettle::log::test_output &output
) const {
  mettle::scoped_pipe stdout_pipe, stderr_pipe;
  if(stdout_pipe.open() < 0 ||
     stderr_pipe.open() < 0)
    return parent_failed();

  std::string dir = parent_path(file);
  std::vector<std::string> final_args = {compiler_.path.c_str()};
  for(auto &&tok : translate_args(file, args, dir))
    final_args.push_back(std::move(tok));
  for(const auto &arg : raw_args) {
    if(tool_match(compiler_, arg.tool))
      final_args.push_back(arg.value);
  }

  fflush(nullptr);

  mettle::scoped_sig_intr intr;
  intr.open(SIGCHLD);

  pid_t pid;
  if((pid = fork()) < 0)
    return parent_failed();

  if(pid == 0) {
    intr.close();

    // Make a new process group so we can kill the test and all its children
    // as a group.
    setpgid(0, 0);

    if(timeout_)
      mettle::fork_monitor(*timeout_);

    if(stdout_pipe.close_read() < 0 ||
       stderr_pipe.close_read() < 0)
      child_failed();

    if(stdout_pipe.move_write(STDOUT_FILENO) < 0 ||
       stderr_pipe.move_write(STDERR_FILENO) < 0)
      child_failed();

    execvp(compiler_.path.c_str(), make_argv(final_args).get());
    child_failed();
  }
  else {
    if(stdout_pipe.close_write() < 0 ||
       stderr_pipe.close_write() < 0)
      return parent_failed();

    std::vector<mettle::readfd> dests = {
      {stdout_pipe.read_fd, &output.stdout},
      {stderr_pipe.read_fd, &output.stderr}
    };

    // Read from the piped stdout, stderr, and log. If we're interrupted
    // (probably by SIGCHLD), do one last non-blocking read to get any data we
    // might have missed.
    sigset_t empty;
    sigemptyset(&empty);
    if(mettle::read_into(dests, nullptr, &empty) < 0) {
      if(errno != EINTR)
        return parent_failed();
      timespec timeout = {0, 0};
      if(mettle::read_into(dests, &timeout, nullptr) < 0)
        return parent_failed();
    }

    int status;
    if(waitpid(pid, &status, 0) < 0)
      return parent_failed();

    // Make sure everything in the test's process group is dead. Don't worry
    // about reaping.
    kill(-pid, SIGKILL);

    if(WIFEXITED(status)) {
      int exit_code = WEXITSTATUS(status);
      if(exit_code == mettle::err_timeout) {
        std::ostringstream ss;
        ss << "Timed out after " << timeout_->count() << " ms";
        return { false, ss.str() };
      }
      else if(bool(exit_code) == expect_fail) {
        return { true, "" };
      }
      else if(exit_code) {
        return { false, "Compilation failed" };
      }
      else {
        return { false, "Compilation successful" };
      }
    }
    else if(WIFSIGNALED(status)) {
      return { false, strsignal(WTERMSIG(status)) };
    }
    else { // WIFSTOPPED
      kill(pid, SIGKILL);
      return { false, strsignal(WSTOPSIG(status)) };
    }
  }
}

} // namespace caliber
