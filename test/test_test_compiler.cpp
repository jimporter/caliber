#include <mettle.hpp>
#include <stdlib.h>
#include "../src/test_compiler.hpp"
using namespace mettle;

auto is_cxx(bool cxx) {
  return make_matcher(
    [expected = cxx](const auto &filename) -> match_result {
      bool actual = caliber::is_cxx(filename);
      std::ostringstream ss;
      ss << to_printable(filename) << (actual ? " is " : " is not ") << "C++";
      return {actual == expected, ss.str()};
    }, cxx ? "is C++" : "is not C++"
  );
}

suite<> blah("compilation utilities", [](auto &_) {

  subsuite<>(_, "is_cxx", [](auto &_) {
    _.test("C++ files", []() {
      for(const auto &file : {
        "foo.cpp", "foo.cc", "foo.cp", "foo.cxx", "foo.CPP", "foo.c++", "foo.C",
        "foo.ii", "foo.mm", "foo.M", "foo.mii"
      }) {
        expect(file, is_cxx(true));
      }
    });

    _.test("non-C++ files", []() {
      for(const auto &file : {"foo.c", "foo.i", "foo.m", "foo.mi", "foo.f"}) {
        expect(file, is_cxx(false));
      }
    });
  });

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
