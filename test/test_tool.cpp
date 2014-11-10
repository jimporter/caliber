#include <mettle.hpp>
using namespace mettle;

#include "env_helper.hpp"
#include "../src/tool.hpp"

suite<test_env> test_tool("tool construction", [](auto &_) {
  _.test("from hard link", [](test_env &) {
    caliber::tool x("g++-1.0");
    expect(x.path, equal_to("g++-1.0"));
    expect(x.identity, array("g++-1.0", "g++", "c++"));
  });

  _.test("from symlink", [](test_env &) {
    caliber::tool x("g++");
    expect(x.path, equal_to("g++"));
    expect(x.identity, array("g++-1.0", "g++", "c++"));
  });
});
