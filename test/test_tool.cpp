#include <mettle.hpp>
using namespace mettle;

#include "env_helper.hpp"
#include "../src/tool.hpp"

suite<test_env> test_tool("tool construction", [](auto &_) {
  _.test("gcc", [](test_env &) {
    caliber::tool x("g++");
    expect(x.path, equal_to("g++"));
    expect(x.identity, array("gcc", "cc"));
  });

  _.test("clang", [](test_env &) {
    caliber::tool x("clang++");
    expect(x.path, equal_to("clang++"));
    expect(x.identity, array("clang", "cc"));
  });

  _.test("generic cc", [](test_env &) {
    caliber::tool x("c++");
    expect(x.path, equal_to("c++"));
    expect(x.identity, array("cc"));
  });

  _.test("unknown tool", [](test_env &) {
    caliber::tool x("program");
    expect(x.path, equal_to("program"));
    expect(x.identity, array());
  });
});
