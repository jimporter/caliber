#include <mettle.hpp>
#include <stdlib.h>
#include "../src/test_compiler.hpp"
using namespace mettle;

suite<> compilation("compilation utilities", [](auto &_) {

  subsuite<>(_, "tool construction", [](auto &_) {
    std::string old_path;
    std::string test_data;

    _.setup([&old_path, &test_data]() {
      const char *old = getenv("PATH");
      const char *data = getenv("TEST_DATA");
      if(old) old_path = old;

      expect("TEST_DATA is in environment", data, is_not(nullptr));
      test_data = data;

      setenv("PATH", (test_data + ":" + old_path).c_str(), true);
    });

    _.teardown([&old_path]() {
      setenv("PATH", old_path.c_str(), true);
    });

    _.test("from hard link", []() {
      caliber::tool x("program");
      expect(x.path, equal_to("program"));
      expect(x.name, equal_to("program"));
    });

    _.test("from symlink", []() {
      caliber::tool x("proglink");
      expect(x.path, equal_to("proglink"));
      expect(x.name, equal_to("program"));
    });
  });

});
