#include <mettle.hpp>
using namespace mettle;

#include "env_helper.hpp"
#include "../src/compiler.hpp"

// Run these commands in a cmd shell on Windows so that it can find Python to
// execute our dummy compilers. We might want to integrate this into caliber
// itself to be more user-friendly...
#ifndef _WIN32
#  define PREFIX ""
#else
#  define PREFIX "cmd /c "
#endif

using compiler_ptr = std::unique_ptr<const caliber::compiler>;

suite<test_env> test_compiler("compilers", [](auto &_) {
  subsuite(_, "construction", [](auto &_) {
    _.test("gcc", [](test_env &) {
      auto c = caliber::make_compiler(PREFIX "g++.py");
      expect(c->path, equal_to(PREFIX "g++.py"));
      expect(c->brand, equal_to("gcc"));
      expect(c->flavor, equal_to("cc"));

      expect(c->match_flavor("gcc"), equal_to(true));
      expect(c->match_flavor("cc"), equal_to(true));
      expect(c->match_flavor("msvc"), equal_to(false));
    });

    _.test("clang", [](test_env &) {
      auto c = caliber::make_compiler(PREFIX "clang++.py");
      expect(c->path, equal_to(PREFIX "clang++.py"));
      expect(c->brand, equal_to("clang"));
      expect(c->flavor, equal_to("cc"));

      expect(c->match_flavor("clang"), equal_to(true));
      expect(c->match_flavor("cc"), equal_to(true));
      expect(c->match_flavor("msvc"), equal_to(false));
    });

    _.test("generic cc", [](test_env &) {
      auto c = caliber::make_compiler(PREFIX "c++.py");
      expect(c->path, equal_to(PREFIX "c++.py"));
      expect(c->brand, equal_to("unknown"));
      expect(c->flavor, equal_to("cc"));

      expect(c->match_flavor("gcc"), equal_to(false));
      expect(c->match_flavor("cc"), equal_to(true));
      expect(c->match_flavor("msvc"), equal_to(false));
    });

    _.test("cl (microsoft)", [](test_env &) {
      auto c = caliber::make_compiler(PREFIX "cl.py");
      expect(c->path, equal_to(PREFIX "cl.py"));
      expect(c->brand, equal_to("msvc"));
      expect(c->flavor, equal_to("msvc"));

      expect(c->match_flavor("clang"), equal_to(false));
      expect(c->match_flavor("cc"), equal_to(false));
      expect(c->match_flavor("msvc"), equal_to(true));
    });

    _.test("generic msvc", [](test_env &) {
      auto c = caliber::make_compiler(PREFIX "msvc.py");
      expect(c->path, equal_to(PREFIX "msvc.py"));
      expect(c->brand, equal_to("unknown"));
      expect(c->flavor, equal_to("msvc"));

      expect(c->match_flavor("clang"), equal_to(false));
      expect(c->match_flavor("cc"), equal_to(false));
      expect(c->match_flavor("msvc"), equal_to(true));
    });

    _.test("unknown compiler", [](test_env &) {
      expect([]() { caliber::make_compiler(PREFIX "program.py"); },
             thrown<std::runtime_error>("unable to determine compiler flavor"));
    });
  });

  subsuite<compiler_ptr>(_, "translate args (cc)", [](auto &_) {
    _.setup([](test_env &, compiler_ptr &c) {
      c = caliber::make_compiler(PREFIX "g++.py");
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
             array(c->path, "-Dfoo", "-fsyntax-only", "src.cpp"));
      expect(c->translate_args("src.cpp", {{"-U", {"foo"}}}, {}),
             array(c->path, "-Ufoo", "-fsyntax-only", "src.cpp"));
    });

    _.test("raw args", [](test_env &, compiler_ptr &c) {
      expect(c->translate_args(
        "src.cpp", {}, {{"cc", "-Wall"}, {"msvc", "/WX"}}
      ), array(c->path, "-Wall", "-fsyntax-only", "src.cpp"));
    });
  });

  subsuite<compiler_ptr>(_, "translate args (msvc)", [](auto &_) {
    _.setup([](test_env &, compiler_ptr &c) {
      c = caliber::make_compiler(PREFIX "cl.py");
    });

    _.test("empty", [](test_env &, compiler_ptr &c) {
      expect(c->translate_args("src.cpp", {}, {}),
             array(c->path, "/Zs", "src.cpp"));
    });

    _.test("--std", [](test_env &, compiler_ptr &c) {
      expect(c->translate_args("src.cpp", {{"std", {"c++11"}}}, {}),
             array(c->path, "/std:c++11", "/Zs", "src.cpp"));
    });

    _.test("-I", [](test_env &, compiler_ptr &c) {
      expect(c->translate_args("src.cpp", {{"-I", {"include"}}}, {}),
             array(c->path, "/Iinclude", "/Zs", "src.cpp"));
    });

    _.test("-D/-U", [](test_env &, compiler_ptr &c) {
      expect(c->translate_args("src.cpp", {{"-D", {"foo"}}}, {}),
             array(c->path, "/Dfoo", "/Zs", "src.cpp"));
      expect(c->translate_args("src.cpp", {{"-U", {"foo"}}}, {}),
             array(c->path, "/Ufoo", "/Zs", "src.cpp"));
    });

    _.test("raw args", [](test_env &, compiler_ptr &c) {
      expect(c->translate_args(
        "src.cpp", {}, {{"cc", "-Wall"}, {"msvc", "/WX"}}
      ), array(c->path, "/WX", "/Zs", "src.cpp"));
    });
  });
});
