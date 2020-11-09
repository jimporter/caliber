#include <mettle.hpp>
using namespace mettle;

#include "env_helper.hpp"
#include "../src/compiler.hpp"

using compiler_ptr = std::unique_ptr<const caliber::compiler>;

auto equal_cmd(const compiler_ptr &c, const std::vector<std::string> &args) {
  auto cmd = c->command;
  cmd.insert(cmd.end(), args.begin(), args.end());
  return equal_to(cmd);
}

suite<test_env> test_compiler("compilers", [](auto &_) {
  subsuite(_, "construction", [](auto &_) {
    _.test("gcc", [](test_env &e) {
      auto c = caliber::make_compiler({"python", e.test_data + "/g++.py"});
      expect(c->command, array("python", e.test_data + "/g++.py"));
      expect(c->brand, equal_to("gcc"));
      expect(c->flavor, equal_to("cc"));

      expect(c->match_flavor("gcc"), equal_to(true));
      expect(c->match_flavor("cc"), equal_to(true));
      expect(c->match_flavor("msvc"), equal_to(false));
    });

    _.test("clang", [](test_env &e) {
      auto c = caliber::make_compiler({"python", e.test_data + "/clang++.py"});
      expect(c->command, array("python", e.test_data + "/clang++.py"));
      expect(c->brand, equal_to("clang"));
      expect(c->flavor, equal_to("cc"));

      expect(c->match_flavor("clang"), equal_to(true));
      expect(c->match_flavor("cc"), equal_to(true));
      expect(c->match_flavor("msvc"), equal_to(false));
    });

    _.test("generic cc", [](test_env &e) {
      auto c = caliber::make_compiler({"python", e.test_data + "/c++.py"});
      expect(c->command, array("python", e.test_data + "/c++.py"));
      expect(c->brand, equal_to("unknown"));
      expect(c->flavor, equal_to("cc"));

      expect(c->match_flavor("gcc"), equal_to(false));
      expect(c->match_flavor("cc"), equal_to(true));
      expect(c->match_flavor("msvc"), equal_to(false));
    });

    _.test("cl (microsoft)", [](test_env &e) {
      auto c = caliber::make_compiler({"python", e.test_data + "/cl.py"});
      expect(c->command, array("python", e.test_data + "/cl.py"));
      expect(c->brand, equal_to("msvc"));
      expect(c->flavor, equal_to("msvc"));

      expect(c->match_flavor("clang"), equal_to(false));
      expect(c->match_flavor("cc"), equal_to(false));
      expect(c->match_flavor("msvc"), equal_to(true));
    });

    _.test("generic msvc", [](test_env &e) {
      auto c = caliber::make_compiler({"python", e.test_data + "/msvc.py"});
      expect(c->command, array("python", e.test_data + "/msvc.py"));
      expect(c->brand, equal_to("unknown"));
      expect(c->flavor, equal_to("msvc"));

      expect(c->match_flavor("clang"), equal_to(false));
      expect(c->match_flavor("cc"), equal_to(false));
      expect(c->match_flavor("msvc"), equal_to(true));
    });

    _.test("unknown compiler", [](test_env &e) {
      expect([&e]() {
        caliber::make_compiler({"python", e.test_data + "/program.py"});
      }, thrown<std::runtime_error>("unable to determine compiler flavor"));
    });
  });

  subsuite<compiler_ptr>(_, "translate args (cc)", [](auto &_) {
    _.setup([](test_env &e, compiler_ptr &c) {
      c = caliber::make_compiler({"python", e.test_data + "/g++.py"});
    });

    _.test("empty", [](test_env &, compiler_ptr &c) {
      expect(c->translate_args("src.cpp", {}, {}),
             equal_cmd(c, {"-fsyntax-only", "src.cpp"}));
    });

    _.test("--std", [](test_env &, compiler_ptr &c) {
      expect(c->translate_args("src.cpp", {{"std", {"c++11"}}}, {}),
             equal_cmd(c, {"-std=c++11", "-fsyntax-only", "src.cpp"}));
    });

    _.test("-I", [](test_env &, compiler_ptr &c) {
      expect(c->translate_args("src.cpp", {{"-I", {"include"}}}, {}),
             equal_cmd(c, {"-Iinclude", "-fsyntax-only", "src.cpp"}));
    });

    _.test("-D/-U", [](test_env &, compiler_ptr &c) {
      expect(c->translate_args("src.cpp", {{"-D", {"foo"}}}, {}),
             equal_cmd(c, {"-Dfoo", "-fsyntax-only", "src.cpp"}));
      expect(c->translate_args("src.cpp", {{"-U", {"foo"}}}, {}),
             equal_cmd(c, {"-Ufoo", "-fsyntax-only", "src.cpp"}));
    });

    _.test("raw args", [](test_env &, compiler_ptr &c) {
      expect(
        c->translate_args("src.cpp", {}, {{"cc", "-Wall"}, {"msvc", "/WX"}}),
        equal_cmd(c, {"-Wall", "-fsyntax-only", "src.cpp"})
      );
    });
  });

  subsuite<compiler_ptr>(_, "translate args (msvc)", [](auto &_) {
    _.setup([](test_env &e, compiler_ptr &c) {
      c = caliber::make_compiler({"python", e.test_data + "/cl.py"});
    });

    _.test("empty", [](test_env &, compiler_ptr &c) {
      expect(c->translate_args("src.cpp", {}, {}),
             equal_cmd(c, {"/Zs", "src.cpp"}));
    });

    _.test("--std", [](test_env &, compiler_ptr &c) {
      expect(c->translate_args("src.cpp", {{"std", {"c++11"}}}, {}),
             equal_cmd(c, {"/std:c++11", "/Zs", "src.cpp"}));
    });

    _.test("-I", [](test_env &, compiler_ptr &c) {
      expect(c->translate_args("src.cpp", {{"-I", {"include"}}}, {}),
             equal_cmd(c, {"/Iinclude", "/Zs", "src.cpp"}));
    });

    _.test("-D/-U", [](test_env &, compiler_ptr &c) {
      expect(c->translate_args("src.cpp", {{"-D", {"foo"}}}, {}),
             equal_cmd(c, {"/Dfoo", "/Zs", "src.cpp"}));
      expect(c->translate_args("src.cpp", {{"-U", {"foo"}}}, {}),
             equal_cmd(c, {"/Ufoo", "/Zs", "src.cpp"}));
    });

    _.test("raw args", [](test_env &, compiler_ptr &c) {
      expect(
        c->translate_args("src.cpp", {}, {{"cc", "-Wall"}, {"msvc", "/WX"}}),
        equal_cmd(c, {"/WX", "/Zs", "src.cpp"})
      );
    });
  });
});
