#define CATCH_CONFIG_RUNNER
#include <catch/catch.hpp>

std::string arch;

int main(int argc, char *argv[]) {
  Catch::Session session; // There must be exactly one instance

  // Build a new parser on top of Catch's
  using namespace Catch::clara;
  auto cli =
    session.cli() // Get Catch's composite command line parser
    | Opt(
        [&](const std::string &val) {
          if (val == "m68k" || val == "x64") {
            arch = val;
            return detail::ParserResult::ok(detail::ParseResultType::Matched);
          }

          return detail::ParserResult::runtimeError("unknown architecture "
                                                    + val);
        },
        "architecture") // bind variable to a new option, with a hint string
        ["--arch"]      // the option names it will respond to
    ("target architecture: x64/m68k. Defaults to m68k");

  // Now pass the new composite back to Catch so it uses that
  session.cli(cli);

  auto return_code = session.applyCommandLine(argc, argv);

  if (return_code) {
    return return_code;
  }

  if (arch.empty()) {
    arch = "m68k";
  }

  return session.run();
}