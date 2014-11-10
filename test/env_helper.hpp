#ifndef INC_CALIBER_TEST_ENV_HELPER_HPP
#define INC_CALIBER_TEST_ENV_HELPER_HPP

#include <cstdlib>
#include <string>

struct test_env {
  test_env() {
    const char *old = getenv("PATH");
    const char *data = getenv("TEST_DATA");
    if(old) old_path = old;

    expect("TEST_DATA is in environment", data, is_not(nullptr));
    test_data = data;

    setenv("PATH", (test_data + ":" + old_path).c_str(), true);
  }

  ~test_env() {
    setenv("PATH", old_path.c_str(), true);
  }

  std::string test_data, old_path;
};

#endif
