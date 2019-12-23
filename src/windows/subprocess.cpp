#include "subprocess.hpp"

#include <windows.h>

#include <cassert>
#include <sstream>
#include <stdexcept>
#include <system_error>

#include <mettle/driver/exit_code.hpp>
#include <mettle/driver/windows/scoped_handle.hpp>
#include <mettle/driver/windows/scoped_pipe.hpp>

namespace caliber::windows {

  namespace {
    const std::size_t BUFSIZE = 4096;
  }

  std::string slurp(const char *argv[]) {
    assert(argv && "empty argv");

    mettle::windows::scoped_pipe stdout_pipe;
    if(!stdout_pipe.open() ||
       !stdout_pipe.set_write_inherit(true))
      throw std::system_error(GetLastError(), std::system_category());

    std::ostringstream cmd_line;
    cmd_line << argv[0];
    for(const char **i = argv + 1; *i; i++)
      cmd_line << " " << *i;

    STARTUPINFOA startup_info = { sizeof(STARTUPINFOA) };
    startup_info.dwFlags = STARTF_USESTDHANDLES;
    startup_info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup_info.hStdOutput = stdout_pipe.write_handle;
    startup_info.hStdError = stdout_pipe.write_handle;

    PROCESS_INFORMATION proc_info;

    if(!CreateProcessA(
         nullptr, const_cast<char*>(cmd_line.str().c_str()), nullptr, nullptr,
         true, 0, nullptr, nullptr, &startup_info, &proc_info
       )) {
      throw std::system_error(GetLastError(), std::system_category());
    }
    mettle::windows::scoped_handle subproc_handles[] = {
      proc_info.hProcess, proc_info.hThread
    };

    if(!stdout_pipe.close_write())
      throw std::system_error(GetLastError(), std::system_category());

    std::string output;
    DWORD size;
    char buf[BUFSIZE];

    do {
      if(!ReadFile(stdout_pipe.read_handle, buf, sizeof(buf), &size, nullptr)) {
        DWORD err = GetLastError();
        if(err != ERROR_BROKEN_PIPE)
          throw std::system_error(GetLastError(), std::system_category());
      }
      output.append(buf, size);
    } while(size != 0);

    DWORD exit_status;
    if(!GetExitCodeProcess(proc_info.hProcess, &exit_status))
      throw std::system_error(GetLastError(), std::system_category());
    if(exit_status != 0)
      throw std::runtime_error(output);

    return output;
  }

} // namespace caliber::windows
