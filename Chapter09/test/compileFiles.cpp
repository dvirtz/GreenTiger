#include "Program.h"
#include "testsHelper.h"
#include <boost/optional/optional_io.hpp>
#include <catch2/catch.hpp>

extern std::string arch;

TEST_CASE("compile test files") {
  namespace fs = boost::filesystem;
  tiger::forEachTigerTest(
    [](const fs::path &filepath, bool parseError, bool compilationError) {
      SECTION(filepath.filename().string()) {
        if (parseError || compilationError) {
          REQUIRE_FALSE(tiger::compileFile(arch, filepath.string()));
        } else {
          REQUIRE(tiger::compileFile(arch, filepath.string()));
        }
      }
    });
}