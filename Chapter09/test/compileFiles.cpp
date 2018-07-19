#include <catch/catch.hpp>
#include <boost/optional/optional_io.hpp>
#include "testsHelper.h"
#include "Program.h"

extern std::string arch;

TEST_CASE("compile test files")
{
    namespace fs = boost::filesystem;
    tiger::forEachTigerTest(
        [](const fs::path &filepath, bool parseError, bool compilationError) {
            SECTION(filepath.filename().string())
            {
                if (parseError || compilationError)
                {
                    REQUIRE_FALSE(tiger::compileFile(arch, filepath.string()));
                }
                else
                {
                    REQUIRE(tiger::compileFile(arch, filepath.string()));
                }
            }
        });
}