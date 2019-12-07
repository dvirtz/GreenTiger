#include "Program.h"
#include "testsHelper.h"
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

TEST_CASE("lex test files") {
  namespace fs = boost::filesystem;
  tiger::forEachTigerTest([](const fs::path &filepath, bool /*parseError*/,
                             bool /*compilationError*/) {
    auto filename = filepath.filename();
    CAPTURE(filename);
    REQUIRE(lexFile(filepath.string()));
  });
}