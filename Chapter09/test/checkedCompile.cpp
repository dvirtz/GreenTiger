#include "Test.h"
#include "Program.h"
#include <catch/catch.hpp>

std::string checkedCompile(const std::string &string) {
  auto res = tiger::compile(arch, string);
  REQUIRE(res);
  return *res;
}