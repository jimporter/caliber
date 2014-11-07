#include <mettle.hpp>
#include "../src/paths.hpp"
using namespace mettle;

suite<> paths("path utilities", [](auto &_) {

  subsuite<>(_, "which()", [](auto &_) {
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

    _.test("filename", [&test_data]() {
      expect(caliber::which("program"), equal_to(test_data + "/program"));
    });

    _.test("absolute path", [&test_data]() {
      std::string abspath = test_data + "/program";
      expect(caliber::which(abspath), equal_to(abspath));
    });

    _.test("nonexistent file", []() {
      expect(
        []() { caliber::which("this_file_doesnt_exist"); },
        thrown<std::runtime_error>(
          "unable to locate command `this_file_doesnt_exist`"
        )
      );
    });
  });

  subsuite<>(_, "parent_path()", [](auto &_) {
    _.test("empty string", []() {
      expect(caliber::parent_path(""), equal_to(""));
    });

    _.test("filename", []() {
      expect(caliber::parent_path("file"), equal_to(""));
    });

    _.test("absolute path", []() {
      expect(caliber::parent_path("/path/to/file"), equal_to("/path/to/"));
    });

    _.test("relative path", []() {
      expect(caliber::parent_path("path/to/file"), equal_to("path/to/"));
    });
  });

  subsuite<>(_, "leafname()", [](auto &_) {
    _.test("empty string", []() {
      expect(caliber::leafname(""), equal_to(""));
    });

    _.test("filename", []() {
      expect(caliber::leafname("file"), equal_to("file"));
    });

    _.test("absolute path", []() {
      expect(caliber::leafname("/path/to/file"), equal_to("file"));
    });

    _.test("relative path", []() {
      expect(caliber::leafname("path/to/file"), equal_to("file"));
    });
  });

});
