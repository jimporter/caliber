#include "../compilation_test_runner.hpp"

#include <windows.h>

#include <cassert>
#include <sstream>

#include <mettle/driver/exit_code.hpp>
#include <mettle/driver/windows/scoped_handle.hpp>
#include <mettle/driver/windows/scoped_pipe.hpp>
#include <mettle/driver/windows/subprocess.hpp>
#include <mettle/output.hpp>

// XXX: Use std::source_location instead when we're able.
#define CALIBER_FAILED() failed(__FILE__, __LINE__)

namespace caliber {

  namespace {
    inline std::string err_string(DWORD errnum) {
      char *tmp;
      FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errnum, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<char*>(&tmp), 0, nullptr
      );
      std::string msg(tmp);
      LocalFree(tmp);
      return msg;
    }

    mettle::test_result failed(const char *file, std::size_t line) {
      std::ostringstream ss;
      ss << "Fatal error at " << file << ":" << line << "\n"
         << err_string(GetLastError());
      return {false, ss.str()};
    }

    std::string make_cmd_line(const std::vector<std::string> &argv) {
      assert(argv.size() > 0);
      std::ostringstream cmd_line;
      cmd_line << argv[0];
      for(std::size_t i = 1; i != argv.size(); i++)
        cmd_line << " " << argv[i];
      return cmd_line.str();
    }
  }

  mettle::test_result
  compilation_test_runner::operator ()(
    const std::string &file, const compiler_options &args,
    const raw_options &raw_args, bool expect_fail,
    mettle::log::test_output &output
  ) const {
    using namespace mettle::windows;

    scoped_pipe stdout_pipe, stderr_pipe;
    if(!stdout_pipe.open(true, false) ||
       !stderr_pipe.open(true, false))
      return CALIBER_FAILED();

    if(!stdout_pipe.set_write_inherit(true) ||
       !stderr_pipe.set_write_inherit(true))
      return CALIBER_FAILED();

    auto cmd_line = make_cmd_line(
      compiler_->translate_args(file, args, raw_args)
    );

    STARTUPINFOA startup_info = { sizeof(STARTUPINFOA) };
    startup_info.dwFlags = STARTF_USESTDHANDLES;
    startup_info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup_info.hStdOutput = stdout_pipe.write_handle;
    startup_info.hStdError = stderr_pipe.write_handle;

    PROCESS_INFORMATION proc_info;

    scoped_handle job;
    if(!(job = CreateJobObject(nullptr, nullptr)))
      return CALIBER_FAILED();

    scoped_handle timeout_event;
    if(timeout_) {
      if(!(timeout_event = CreateWaitableTimer(nullptr, true, nullptr)))
        return CALIBER_FAILED();
      LARGE_INTEGER t;
      // Convert from ms to 100s-of-nanoseconds (negative for relative time).
      t.QuadPart = -timeout_->count() * 10000;
      if(!SetWaitableTimer(timeout_event, &t, 0, nullptr, nullptr, false))
        return CALIBER_FAILED();
    }

    if(!CreateProcessA(
         nullptr, const_cast<char*>(cmd_line.c_str()), nullptr, nullptr, true,
         CREATE_SUSPENDED, nullptr, nullptr, &startup_info, &proc_info
       )) {
      return CALIBER_FAILED();
    }
    scoped_handle subproc_handles[] = {proc_info.hProcess, proc_info.hThread};

    // Assign a job object to the child process (so we can kill the job later)
    // and then let it start running.
    if(!AssignProcessToJobObject(job, proc_info.hProcess))
      return CALIBER_FAILED();
    if(!ResumeThread(proc_info.hThread))
      return CALIBER_FAILED();

    if(!stdout_pipe.close_write() ||
       !stderr_pipe.close_write())
      return CALIBER_FAILED();

    std::string message;
    std::vector<readhandle> dests = {
      {stdout_pipe.read_handle, &output.stdout_log},
      {stderr_pipe.read_handle, &output.stderr_log}
    };
    std::vector<HANDLE> interrupts = {proc_info.hProcess};
    if(timeout_)
      interrupts.push_back(timeout_event);

    HANDLE finished = read_into(dests, INFINITE, interrupts);
    if(!finished)
      return CALIBER_FAILED();
    // Do one last non-blocking read to get any data we might have missed.
    read_into(dests, 0, interrupts);

    // By now, the child process's main thread has returned, so kill any stray
    // processes in the job.
    TerminateJobObject(job, 1);

    if(finished == timeout_event) {
      std::ostringstream ss;
      ss << "Timed out after " << timeout_->count() << " ms";
      return { false, ss.str() };
    } else {
      DWORD exit_status;
      if(!GetExitCodeProcess(proc_info.hProcess, &exit_status))
        return CALIBER_FAILED();
      std::ostringstream ss;
      ss << cmd_line << " ";
      bool success = exit_status == mettle::exit_code::success;
      ss << (success ? "\nCompilation successful" : "\nCompilation failed");
      return {expect_fail != success, ss.str()};
    }
  }

} // namespace caliber
