#include "Program.h"
#include "testsHelper.h"
#define CATCH_CONFIG_MAIN
#include <catch.hpp>

using namespace tiger;

TEST_CASE("parse test files") {
  namespace fs = boost::filesystem;
  forEachTigerTest([](const fs::path &filepath, bool parseError, bool /*compilationError*/) {
    auto filename = filepath.filename();
    CAPTURE(filename);
    if (parseError)
    {
      REQUIRE_FALSE(parseFile(filepath.string()));
    }
    else
    {
      REQUIRE(parseFile(filepath.string()));
    }
  });
}

TEST_CASE("lvalue") {
  SECTION("identfier") {
    REQUIRE(parse("i"));
  }
  SECTION("field") {
    REQUIRE(parse("i.m"));
  }
  SECTION("array element") {
    REQUIRE(parse("i[m]"));
  }
}

TEST_CASE("nil") {
  REQUIRE(parse("nil"));
}

TEST_CASE("sequence") {
  SECTION("empty") {
    REQUIRE(parse("()"));
  }
  SECTION("single expression") {
    REQUIRE(parse("(i)"));
  }
  SECTION("multiple expressions") {
    REQUIRE(parse("(i;j)"));
  }
}

TEST_CASE("integer") {
  SECTION("positive") {
    REQUIRE(parse("42"));
  }
  SECTION("negative") {
    REQUIRE(parse("-42"));
  }
}

TEST_CASE("string") {
  SECTION("empty") {
    REQUIRE(parse(R"("")"));
  }
  SECTION("not empty") {
    auto program = R"("\tHello \"World\"!\n")";
    REQUIRE(parse(program));
  }
}

TEST_CASE("function call") {
  SECTION("with no arguments") {
    REQUIRE(parse("f()"));
  }
  SECTION("with arguments") {
    REQUIRE(parse("f(a)"));
  }
  SECTION("with subexpressions") {
    REQUIRE(parse(R"(f(42, "hello"))"));
  }
}

TEST_CASE("arithmetic") {
  SECTION("simple") {
    REQUIRE(parse("2+3"));
    REQUIRE(parse("2-3"));
    REQUIRE(parse("2*3"));
    REQUIRE(parse("2/3"));
  }
  SECTION("compound") {
    REQUIRE(parse("2*(3+(5-1))/24"));
  }
  SECTION("with subexpression") {
    REQUIRE(parse("f(2)+a/(4 + b)"));
  }
}

TEST_CASE("comparison") {
  SECTION("simple") {
    REQUIRE(parse("2=3"));
    REQUIRE(parse("2<>3"));
    REQUIRE(parse("2>3"));
    REQUIRE(parse("2<3"));
    REQUIRE(parse("2>=3"));
    REQUIRE(parse("2<=3"));
  }
  SECTION("compound") {
    REQUIRE(parse("2<(3+4)"));
    REQUIRE_FALSE(parse("2=3=4"));
  }
  SECTION("with subexpression") {
    REQUIRE(parse("(f(2)+a)>=(4/2)"));
  }
}

TEST_CASE("boolean") {
  SECTION("simple") {
    REQUIRE(parse("2&3"));
    REQUIRE(parse("2|3"));
  }
  SECTION("compound") {
    REQUIRE(parse("2&(3|(2-4))"));
  }
  SECTION("with subexpression") {
    REQUIRE(parse("f(2)&((3=4|2/b))"));
  }
}

TEST_CASE("record") {
  SECTION("empty") {
    REQUIRE(parse("t{}"));
  }
  SECTION("not empty") {
    REQUIRE(parse("t{a=2}"));
    REQUIRE(parse("tt{a=3, b=a}"));
  }
  SECTION("with subexpression") {
    REQUIRE(parse("t{a=f(2+3), b=(a&(b/2))}"));
  }
}

TEST_CASE("array") {
  SECTION("simple") {
    REQUIRE(parse("a[2] of 3"));
  }
  SECTION("with subexpression") {
    REQUIRE(parse("a[f(2+b)] of \"hello\""));
  }
}

TEST_CASE("assignment") {
  SECTION("simple") {
    REQUIRE(parse("a := 3"));
  }
  SECTION("with subexpression") {
    REQUIRE(parse("a[b.d] := f(5/d)"));
  }
}

TEST_CASE("if then") {
  SECTION("no else") {
    REQUIRE(parse("if a then a := 3"));
    SECTION("with subexpression") {
      REQUIRE(parse("if f(b)=2/d then a[2] := g(b.d)"));
    }
    SECTION("nested") {
      REQUIRE(parse("if 2>3 then if 2<3 then a := 3"));
      REQUIRE(parse("if 2>3 then (if 2<3 then a := 3)"));
    }
  }
  SECTION("with else") {
    REQUIRE(parse("if a then a := 3 else a := 2"));
    REQUIRE(parse("if a then 5 else 7"));
    SECTION("with subexpression") {
      REQUIRE(parse("if f(b)=2/d then 2/3 else f(5)-b"));
    }
    SECTION("nested") {
      REQUIRE(parse("if 2>3 then if 2<3 then 4 else 6 else 7"));
      REQUIRE(parse("if 2>3 then (if 2<3 then 4 else 6) else 7"));
      REQUIRE(parse("if 2>3 then (if 2<3 then a := 4) else b := 4"));
    }
  }
}

TEST_CASE("while") {
  REQUIRE(parse("while b > 0 do (a:=2;b:=b-1)"));
}

TEST_CASE("for") {
  REQUIRE(parse("for a := 0 to 4 do f(a)"));
}

TEST_CASE("break") {
  SECTION("simple") {
    REQUIRE(parse("break"));
  }
  SECTION("in a loop") {
    REQUIRE(parse("while b > 0 do (if(x) then break;a:=2;b:=b-1)"));
  }
}

TEST_CASE("let") {
  REQUIRE(parse(R"(
let
  /* types */
  type t1 = int
  type t2 = {a:int, b:t1}
  type t3 = array of t2
  /* vars */
  var a := 2
  var b : t3 := nil
  var c : t2 := t2{a=2}
  /* function */
  function f(i : int, b : t3) = b[2] := i
  function g() : t2 = 4
in
  f(a + g(), b);
  c.a := a
end
)"));
}