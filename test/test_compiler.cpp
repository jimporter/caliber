#include <mettle.hpp>
using namespace mettle;

#include "env_helper.hpp"
#include "../src/compiler.hpp"

using compiler_ptr = std::unique_ptr<const caliber::compiler>;

suite<test_env> test_compiler("compilers", [](auto &_) {
  subsuite(_, "construction", [](auto &_) {
    _.test("gcc", [](test_env &) {
      auto c = caliber::make_compiler("g++");
      expect(c->path, equal_to("g++"));
      expect(c->brand, equal_to("gcc"));
      expect(c->flavor, equal_to("cc"));

      expect(c->match_flavor("gcc"), equal_to(true));
      expect(c->match_flavor("cc"), equal_to(true));
      expect(c->match_flavor("msvc"), equal_to(false));
    });

    _.test("clang", [](test_env &) {
      auto c = caliber::make_compiler("clang++");
      expect(c->path, equal_to("clang++"));
      expect(c->brand, equal_to("clang"));
      expect(c->flavor, equal_to("cc"));

      expect(c->match_flavor("clang"), equal_to(true));
      expect(c->match_flavor("cc"), equal_to(true));
      expect(c->match_flavor("msvc"), equal_to(false));
    });

    _.test("generic cc", [](test_env &) {
      auto c = caliber::make_compiler("c++");
      expect(c->path, equal_to("c++"));
      expect(c->brand, equal_to("unknown"));
      expect(c->flavor, equal_to("cc"));

      expect(c->match_flavor("gcc"), equal_to(false));
      expect(c->match_flavor("cc"), equal_to(true));
      expect(c->match_flavor("msvc"), equal_to(false));
    });

    _.test("unknown compiler", [](test_env &) {
      expect([]() { caliber::make_compiler("program"); },
             thrown<std::runtime_error>("unable to determine compiler flavor"));
    });
  });

  subsuite<compiler_ptr>(_, "translate args", [](auto &_) {
    _.setup([](test_env &, compiler_ptr &c) {
      c = caliber::make_compiler("g++");
    });

    _.test("empty", [](test_env &, compiler_ptr &c) {
      expect(c->translate_args("src.cpp", {}, {}),
             array(c->path, "-fsyntax-only", "src.cpp"));
    });

    _.test("--std", [](test_env &, compiler_ptr &c) {
      expect(c->translate_args("src.cpp", {{"std", {"c++11"}}}, {}),
             array(c->path, "-std=c++11", "-fsyntax-only", "src.cpp"));
    });

    _.test("-I", [](test_env &, compiler_ptr &c) {
      expect(c->translate_args("src.cpp", {{"-I", {"include"}}}, {}),
             array(c->path, "-Iinclude", "-fsyntax-only", "src.cpp"));
    });

    _.test("-D/-U", [](test_env &, compiler_ptr &c) {
      expect(c->translate_args("src.cpp", {{"-D", {"foo"}}}, {}),
             array(c->path, "-D", "foo", "-fsyntax-only", "src.cpp"));
      expect(c->translate_args("src.cpp", {{"-U", {"foo"}}}, {}),
             array(c->path, "-U", "foo", "-fsyntax-only", "src.cpp"));
    });

    _.test("raw args", [](test_env &, compiler_ptr &c) {
      expect(c->translate_args("src.cpp", {}, {{"cc", "-Wall"}}),
             array(c->path, "-Wall", "-fsyntax-only", "src.cpp"));
    });
  });
});
