#include <mettle.hpp>
#include "../src/paths.hpp"
using namespace mettle;

suite<> paths("path utilities", [](auto &_) {

  subsuite<>(_, "which()", [](auto &_) {
    _.test("filename", []() {
      expect(caliber::which("sh"), equal_to("/bin/sh"));
    });

    _.test("absolute path", []() {
      expect(caliber::which("/bin/sh"), equal_to("/bin/sh"));
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
