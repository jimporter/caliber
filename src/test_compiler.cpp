#include "test_compiler.hpp"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <regex>
#include <sstream>

#include <mettle/driver/exit_code.hpp>
#include <mettle/driver/posix/scoped_pipe.hpp>
#include <mettle/driver/posix/scoped_signal.hpp>
#include <mettle/driver/posix/subprocess.hpp>
#include <mettle/output.hpp>

#include "paths.hpp"

namespace caliber {

namespace {
  pid_t test_pgid = 0;
  struct sigaction old_sigint, old_sigquit;

  void sig_handler(int signum) {
    assert(test_pgid != 0);
    killpg(test_pgid, signum);

    // Restore the previous signal action and re-raise the signal.
    struct sigaction *old_act = signum == SIGINT ? &old_sigint : &old_sigquit;
    sigaction(signum, old_act, nullptr);
    raise(signum);
  }

  void sig_chld(int) {}

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
    if(test_pgid)
      killpg(test_pgid, SIGKILL);
    test_pgid = 0;
    return { false, err_string(errno) };
  }

  [[noreturn]] inline void child_failed() {
    _exit(mettle::exit_code::fatal);
  }

  std::unique_ptr<char *[]>
  make_argv(const std::vector<std::string> &argv) {
    auto real_argv = std::make_unique<char *[]>(argv.size() + 1);
    for(size_t i = 0; i != argv.size(); i++)
      real_argv[i] = const_cast<char*>(argv[i].c_str());
    return real_argv;
  }
}

mettle::test_result
test_compiler::operator ()(
  const std::string &file, const compiler_options &args,
  const raw_options &raw_args, bool expect_fail,
  mettle::log::test_output &output
) const {
  using namespace mettle::posix;
  assert(test_pgid == 0);

  scoped_pipe stdout_pipe, stderr_pipe, pgid_pipe;
  if(stdout_pipe.open() < 0 ||
     stderr_pipe.open() < 0 ||
     pgid_pipe.open(O_CLOEXEC) < 0)
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

  scoped_sigprocmask mask;
  if(mask.push(SIG_BLOCK, SIGCHLD) < 0 ||
     mask.push(SIG_BLOCK, {SIGINT, SIGQUIT}) < 0)
    return parent_failed();

  pid_t pid;
  if((pid = fork()) < 0)
    return parent_failed();

  if(pid == 0) {
    if(mask.clear() < 0)
      child_failed();

    if(stdout_pipe.close_read() < 0 ||
       stderr_pipe.close_read() < 0 ||
       pgid_pipe.close_read() < 0)
      child_failed();

    if(stdout_pipe.move_write(STDOUT_FILENO) < 0 ||
       stderr_pipe.move_write(STDERR_FILENO) < 0)
      child_failed();

    if(timeout_)
      make_timeout_monitor(*timeout_);

    // Make a new process group so we can kill the test and all its children
    // as a group.
    if(setpgid(0, 0) < 0)
      child_failed();

    if(send_pgid(pgid_pipe.write_fd, getpgid(0)) < 0)
      child_failed();

    execvp(compiler_.path.c_str(), make_argv(final_args).get());
    child_failed();
  } else {
    scoped_sigaction sigint, sigquit, sigchld;

    if(stdout_pipe.close_write() < 0 ||
       stderr_pipe.close_write() < 0 ||
       pgid_pipe.close_write() < 0)
      return parent_failed();

    if(recv_pgid(pgid_pipe.read_fd, &test_pgid) < 0)
      return parent_failed();

    if(sigaction(SIGINT, nullptr, &old_sigint) < 0 ||
       sigaction(SIGQUIT, nullptr, &old_sigquit) < 0)
      return parent_failed();

    if(sigint.open(SIGINT, sig_handler) < 0 ||
       sigquit.open(SIGQUIT, sig_handler) < 0 ||
       sigchld.open(SIGCHLD, sig_chld) < 0)
      return parent_failed();

    if(mask.pop() < 0)
      return parent_failed();

    std::vector<readfd> dests = {
      {stdout_pipe.read_fd, &output.stdout_log},
      {stderr_pipe.read_fd, &output.stderr_log}
    };

    // Read from the piped stdout, stderr, and log. If we're interrupted
    // (probably by SIGCHLD), do one last non-blocking read to get any data we
    // might have missed.
    sigset_t empty;
    sigemptyset(&empty);
    if(read_into(dests, nullptr, &empty) < 0) {
      if(errno != EINTR)
        return parent_failed();
      timespec timeout = {0, 0};
      if(read_into(dests, &timeout, nullptr) < 0)
        return parent_failed();
    }

    int status;
    if(waitpid(pid, &status, 0) < 0)
      return parent_failed();

    // Make sure everything in the test's process group is dead. Don't worry
    // about reaping.
    killpg(test_pgid, SIGKILL);
    test_pgid = 0;

    if(WIFEXITED(status)) {
      int exit_status = WEXITSTATUS(status);
      if(exit_status == mettle::exit_code::timeout) {
        std::ostringstream ss;
        ss << "Timed out after " << timeout_->count() << " ms";
        return { false, ss.str() };
      } else {
        std::ostringstream ss;
        for(const auto &i : final_args)
          ss << i << " ";
        bool success = exit_status == mettle::exit_code::success;
        ss << (success ? "\nCompilation successful" : "\nCompilation failed");
        return {expect_fail != success, ss.str()};
      }
    } else { // WIFSIGNALED
      return { false, strsignal(WTERMSIG(status)) };
    }
  }
}

} // namespace caliber
