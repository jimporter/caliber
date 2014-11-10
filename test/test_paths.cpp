#include <mettle.hpp>
using namespace mettle;

#include "env_helper.hpp"
#include "../src/paths.hpp"

suite<> paths("path utilities", [](auto &_) {

  subsuite<test_env>(_, "which()", [](auto &_) {
    _.test("filename", [](test_env &env) {
      expect(caliber::which("g++-1.0"), equal_to(env.test_data + "/g++-1.0"));
    });

    _.test("absolute path", [](test_env &env) {
      std::string abspath = env.test_data + "/g++-1.0";
      expect(caliber::which(abspath), equal_to(abspath));
    });

    _.test("nonexistent file", [](test_env &) {
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
