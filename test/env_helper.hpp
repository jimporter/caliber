#ifndef INC_CALIBER_TEST_ENV_HELPER_HPP
#define INC_CALIBER_TEST_ENV_HELPER_HPP

#include <optional>
#include <string>
#include <system_error>

#ifndef _WIN32
#  include <cstdlib>
#  define PATHSEP ":"

inline std::optional<std::string> get_env(const std::string &name) {
  const char *e = getenv(name.c_str());
  if(e) return e;
  return std::nullopt;
}

inline void set_env(const std::string &name, const std::string &value) {
  if(setenv(name.c_str(), value.c_str(), true) < 0)
    throw std::system_error(errno, std::system_category());
}

#else
#  include <windows.h>
#  define PATHSEP ";"

inline std::optional<std::string> get_env(const std::string &name) {
  char buf[16384];
  if(!GetEnvironmentVariableA(name.c_str(), buf, sizeof(buf))) {
    DWORD err = GetLastError();
    if(err == ERROR_ENVVAR_NOT_FOUND)
      return std::nullopt;
    throw std::system_error(err, std::system_category());
  }
  return buf;
}

inline void set_env(const std::string &name, const std::string &value) {
  if(!SetEnvironmentVariableA(name.c_str(), value.c_str()))
    throw std::system_error(GetLastError(), std::system_category());
}

#endif

struct test_env {
  test_env() {
    auto data = get_env("TEST_DATA");
    expect("TEST_DATA is in environment", data, is_not(std::nullopt));
    test_data = *data;
  }

  std::string test_data;
};

#endif
