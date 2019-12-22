#include <mettle.hpp>
using namespace mettle;

#include "env_helper.hpp"
#include "../src/compiler.hpp"

suite<test_env> test_compiler("compiler construction", [](auto &_) {
  _.test("gcc", [](test_env &) {
    auto x = caliber::make_compiler("g++");
    expect(x->path, equal_to("g++"));
    expect(x->flavor, array("gcc", "cc"));
  });

  _.test("clang", [](test_env &) {
    auto x = caliber::make_compiler("clang++");
    expect(x->path, equal_to("clang++"));
    expect(x->flavor, array("clang", "cc"));
  });

  _.test("generic cc", [](test_env &) {
    auto x = caliber::make_compiler("c++");
    expect(x->path, equal_to("c++"));
    expect(x->flavor, array("cc"));
  });

  _.test("unknown compiler", [](test_env &) {
    auto x = caliber::make_compiler("program");
    expect(x->path, equal_to("program"));
    expect(x->flavor, array());
  });
});
