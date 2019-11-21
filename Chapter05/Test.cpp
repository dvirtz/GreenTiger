#include "Program.h"
#include "testsHelper.h"
#define CATCH_CONFIG_MAIN
#include <boost/format.hpp>
#include <boost/variant/get.hpp>
#include <catch2/catch.hpp>

using namespace tiger;
using boost::get;

TEST_CASE("compile test files") {
  namespace fs = boost::filesystem;
  forEachTigerTest(
    [](const fs::path &filepath, bool parseError, bool compilationError) {
      auto filename = filepath.string();
      CAPTURE(filename);
      if (parseError || compilationError) {
        REQUIRE_FALSE(compileFile(filename));
      } else {
        REQUIRE(compileFile(filename));
      }
    });
}

TEST_CASE("lvalue") {
  SECTION("identifier") {
    REQUIRE(compile(R"(
let
  var i := 0
in
  i
end
)"));
    SECTION("undefined") { REQUIRE_FALSE(compile("i")); }
  }
  SECTION("field") {
    REQUIRE(compile(R"(
let
  type rec = {i : int}
  var r : rec := nil
in
  r.i
end
)"));
    SECTION("undefined record") {
      REQUIRE_FALSE(compile(R"(
let
  type rec = {i : int}
  var r : rec := nil
in
  rr.i
end
)"));
    }
    SECTION("undefined field") {
      REQUIRE_FALSE(compile(R"(
let
  type rec = {i : int}
  var r : rec := nil
in
  r.ii
end
)"));
    }
  }
  SECTION("array element") {
    REQUIRE(compile(R"(
let
  type arr = array of int
  var a := arr[2] of 3
in
  a[0]
end
)"));
    SECTION("undefined array") {
      REQUIRE_FALSE(compile(R"(
let
  type arr = array of int
  var a : arr := nil
in
  aa[0]
end
)"));
    }
    SECTION("subscript is not integral") {
      REQUIRE_FALSE(compile(R"(
let
  type arr = array of int
  var a : arr := nil
in
  a["0"]
end
)"));
    }
  }
}

TEST_CASE("nil") { REQUIRE(compile("nil")); }

TEST_CASE("sequence") {
  SECTION("empty") { REQUIRE(compile("()")); }
  SECTION("fails on a single expression") { REQUIRE_FALSE(compile("(i)")); }
  SECTION("fails on second expression") { REQUIRE_FALSE(compile("(0;j)")); }
}

TEST_CASE("integer") { REQUIRE(compile("42")); }

TEST_CASE("string") {
  auto program = R"("\tHello \"World\"!\n")";
  REQUIRE(compile(program));
}

TEST_CASE("function call") {
  SECTION("with no arguments") {
    REQUIRE(compile(R"(
let
  function f() = ()
in
  f()
end
)"));
    SECTION("return type mismatch") {
      REQUIRE_FALSE(compile(R"(
let
  function f() : int = ()
  var s : string := nil
in
  s := f()
end
)"));
    }
  }
  SECTION("with arguments") {
    REQUIRE(compile(R"(
let
  function f(a: int) = ()
in
  f(2)
end
)"));
    SECTION("missing argument") {
      REQUIRE_FALSE(compile(R"(
let
  function f(a: int) = ()
in
  f()
end
)"));
    }
    SECTION("wrong argument type") {
      REQUIRE_FALSE(compile(R"(
let
  function f(a: int) = ()
in
  f("")
end
)"));
    }
    SECTION("too many arguments") {
      REQUIRE_FALSE(compile(R"(
let
  function f(a: int) = ()
in
  f(2, 3)
end
)"));
    }
  }
}

TEST_CASE("arithmetic comparison and boolean") {
  SECTION("integer") {
    for (auto operation :
         {"+", "-", "*", "/", "=", "<>", ">", "<", "<=", ">=", "&", "|"}) {
      CAPTURE(operation);
      REQUIRE(compile(boost::str(boost::format(R"(
  let
    var i : int := 2 %1% 3
  in
  end
  )") % operation)));
    }
  }

  SECTION("string") {
    for (auto operation : {"=", "<>", ">", "<", "<=", ">="}) {
      CAPTURE(operation);
      REQUIRE(compile(boost::str(boost::format(R"(
  let
    var i : int := "2" %1% "3"
  in
  end
  )") % operation)));
    }
  }

  SECTION("array and record") {
    for (auto operation : {"=", "<>"}) {
      CAPTURE(operation);
      REQUIRE(compile(boost::str(boost::format(R"(
let
  type t = {}
  var i := t{}
  var j := t{}
  var k : int := i %1% j
in
end
)") % operation)));

      REQUIRE(compile(boost::str(boost::format(R"(
let
  type t = array of int
  var i := t[2] of 3
  var j := i
  var k : int := i %1% j
in
end
)") % operation)));
    }
  }

  SECTION("type mismatch") {
    SECTION("in expression") {
      for (auto operation : {"+", "<>", "&"}) {
        CAPTURE(operation);
        REQUIRE_FALSE(compile("2<>\"3\""));
      }
    }
    SECTION("in result") {
      for (auto operation : {"*", "<=", "|"}) {
        REQUIRE_FALSE(compile(boost::str(boost::format(R"(
  let
    var i : string := 2 %1% 3
  in
  end
  )") % operation)));
      }
    }
  }
}

TEST_CASE("record") {
  SECTION("empty") {
    REQUIRE(compile(R"(
let 
  type t = {}
in
  t{}
end
)"));
  }

  SECTION("not empty") {
    REQUIRE(compile(R"(
let 
  type t = {a: int, b: string}
in
  t{a = 2, b = "hello"}
end
)"));
  }

  SECTION("undeclared record type") { REQUIRE_FALSE(compile("t{}")); }

  SECTION("not a record type") { REQUIRE_FALSE(compile("int{}")); }

  SECTION("undeclared field type") {
    REQUIRE_FALSE(compile(R"(
let 
  type t = {a: s}
in
end
)"));
  }

  SECTION("missing fields") {
    REQUIRE_FALSE(compile(R"(
let 
  type t = {a: int, b: string}
in
  t{a = 2}
end
)"));
  }

  SECTION("wrong field type") {
    REQUIRE_FALSE(compile(R"(
let 
  type t = {a: int}
in
  t{a = "hello"}
end
)"));
  }

  SECTION("too many fields") {
    REQUIRE_FALSE(compile(R"(
let 
  type t = {}
in
  t{a = 0}
end
)"));
  }
}

TEST_CASE("array") {
  SECTION("simple") {
    REQUIRE(compile(R"(
let
  type t = array of int
in
  t[2] of 3
end
)"));
  }

  SECTION("undeclared array type") { REQUIRE_FALSE(compile(R"(t[2] of 3)")); }

  SECTION("undeclared element type")
  REQUIRE_FALSE(compile(R"(
let
  type t = array of tt
in
end
)"));

  SECTION("size is not int") {
    REQUIRE_FALSE(compile(R"(
let
  type t = array of int
in
  t["2"] of 3
end
)"));
  }

  SECTION("element type mismatch") {
    REQUIRE_FALSE(compile(R"(
let
  type t = array of int
in
  t[2] of "2"
end
)"));
  }
}

TEST_CASE("assignment") {
  SECTION("simple") {
    REQUIRE(compile(R"(
let
  var a := 3
  var b := 0
in
  b := a
end
)"));
  }

  SECTION("type mismatch") {
    SECTION("in declaration") {
      REQUIRE_FALSE(compile(R"(
let
  var a : string := 3
in
end
)"));
    }

    SECTION("in body") {
      REQUIRE_FALSE(compile(R"(
let
  var a := 3
  var b : string := ""
in
  b := a
end
)"));
    }
  }
}

TEST_CASE("if then") {
  SECTION("no else") {
    REQUIRE(compile(R"(
let
  var a := 0
in
  if 1 then a := 3
end
)"));

    SECTION("test is not of type int") {
      REQUIRE_FALSE(compile(R"(
let
  var a := 0
in
  if "1" then a := 3
end
)"));
    }

    SECTION("expression not of type void") {
      REQUIRE_FALSE(compile(R"(
let
  var a := 0
in
  if 1 then a
end
)"));
    }

    SECTION("entire expression produces no value") {
      REQUIRE_FALSE(compile(R"(
let
  var a := 0
  var b := if 1 then a := 3
end
)"));
    }
  }

  SECTION("with else") {
    SECTION("both expression are of type void") {
      REQUIRE(compile(R"(
let
  var a := 0
in
  if 0 then a := 3 else a := 4
end
)"));
    }

    SECTION("both expression are of the same type") {
      REQUIRE(compile(R"(
let
  var a := if 1 then 2 else 3
in
end
)"));
    }

    SECTION("test is not of type int") {
      REQUIRE_FALSE(compile(R"(
let
  var a := 0
in
  if "0" then a := 3 else a := 4
end
)"));
    }

    SECTION("expressions are not of the same type") {
      REQUIRE_FALSE(compile(R"(
let
  var a := if 1 then 2 else "3"
in
end
)"));
    }
  }
}

TEST_CASE("while") {
  SECTION("positive") {
    REQUIRE(compile(R"(
let
  var b := 2
  var a := 0
in
  while b > 0 do (a:=a+1)
end
)"));
  }

  SECTION("test is not intergral") {
    REQUIRE_FALSE(compile(R"(
let
  var a := 0
in
  while "true" do (a:=a+1)
end
)"));
  }

  SECTION("body produces a value") {
    REQUIRE_FALSE(compile(R"(
let
  var b := 2
  var a := 0
in
  while b > 0 do (a:=a+1;b)
end
)"));
  }
}

TEST_CASE("for") {
  SECTION("positive") {
    REQUIRE(compile(R"(
let
  function f(a: int) = ()
in
  for a := 0 to 4 do f(a)
end
)"));
  }

  SECTION("upper and lower bounds are not integral") {
    REQUIRE_FALSE(compile(R"(
let
  function f(a: int) = ()
in
  for a := "0" to 4 do f(a)
end
)"));

    REQUIRE_FALSE(compile(R"(
let
  function f(a: int) = ()
in
  for a := 0 to "4" do f(a)
end
)"));
  }

  SECTION("body produces a value") {
    REQUIRE_FALSE(compile(R"(
let
  function f(a: int) : int = (a)
in
  for a := 0 to 4 do f(a)
end
)"));
  }
}

TEST_CASE("break") {
  SECTION("posistive") {
    REQUIRE(compile("for a := 0 to 4 do break"));

    REQUIRE(compile("while 1 do break"));
  }

  SECTION("outside a loop") {
    REQUIRE_FALSE(compile(R"(
for a := 0 to 4 do ()
break
)"));

    REQUIRE_FALSE(compile(R"(
while 1 do () 
break
)"));
  }
}

TEST_CASE("type declarations") {
  SECTION("name") {
    REQUIRE(compile(R"(
let
  type a = int
  var b : a := 0
in
end
)"));

    SECTION("outside scope") {
      REQUIRE_FALSE(compile(R"(
let
  type a = int
in
end
let
  var b : a := 0
in
end
)"));
    }
  }

  SECTION("record") {
    REQUIRE(compile(R"(
let
  type rec = {i : int}
  var r := rec{i=2}
in
end
)"));

    SECTION("outside scope") {
      REQUIRE_FALSE(compile(R"(
let
  type rec = {i : int}
in
end
let
  var r := rec{i=2}
in
end
)"));
    }
  }

  SECTION("array") {
    REQUIRE(compile(R"(
let
  type arr = array of string
  var a := arr[3] of "a"
in
end
)"));

    SECTION("outside scope") {
      REQUIRE_FALSE(compile(R"(
let
  type arr = array of string
in
end
let
  var a := arr[3] of "a"
in
end
)"));
    }
  }

  SECTION("recursive") {
    SECTION("single") {
      REQUIRE(compile(R"(
let
  type intlist = {hd: int, tl: intlist}
in
end
)"));
    }

    SECTION("mutually") {
      REQUIRE(compile(R"(
let
  type tree = {key: int, children: treelist}
  type treelist = {hd: tree, tl: treelist}
in
end
)"));
    }

    SECTION("cyclic") {
      REQUIRE_FALSE(compile(R"(
let
  type b = c
  type c = b
in
end
)"));
    }

    SECTION("separated") {
      REQUIRE_FALSE(compile(R"(
let
  type tree = {key: int, children: treelist}
  var t : tree := nil
  type treelist = {hd: tree, tl: treelist}
in
end
)"));
    }
  }
}

TEST_CASE("function declarations") {
  SECTION("simple") {
    REQUIRE(compile(R"(
let
  function f() = ()
in
end
)"));
  }

  SECTION("with parameters") {
    REQUIRE(compile(R"(
let
  function f(a:int, b:string) = ()
in
end
)"));
  }

  SECTION("with return value") {
    REQUIRE(compile(R"(
let
  function f(a:int, b:string) : string = (a=1;b)
in
end
)"));
  }

  SECTION("call outside scope") {
    REQUIRE_FALSE(compile(R"(
let
function f() = ()
in
end
f()
)"));
  }

  SECTION("access parameter outside scope") {
    REQUIRE_FALSE(compile(R"(
let
  function f(a:int) = ()
  var b = a
in
end
)"));
  }

  SECTION("return value mismatch") {
    SECTION("void return value") {
      REQUIRE_FALSE(compile(R"(
let
  function f() = (4)
in
end
)"));
    }

    SECTION("int return value") {
      REQUIRE_FALSE(compile(R"(
let
  function f() : int = ("s")
in
end
)"));
    }
  }

  SECTION("recursive") {
    SECTION("single") {
      REQUIRE(compile(R"(
let
  function fib(n:int) : int = (
    if (n = 0 | n = 1) then n else fib(n-1)+fib(n-2)
  )
in
end
)"));
    }

    SECTION("mutually") {
      REQUIRE(compile(R"(
let
  function f() = (g())
  function g() = (h())
  function h() = (f())
in
end
)"));
    }

    SECTION("separated") {
      REQUIRE_FALSE(compile(R"(
let
  function f() = (g())
  var t : tree := nil
  function g() = (f())
in
end
)"));
    }
  }
}

TEST_CASE("standard library") {
  // function print(s : string)
  REQUIRE(compile(R"(
print("")
)"));

  // function flush()
  REQUIRE(compile(R"(
flush()
)"));

  // function getchar() : string
  REQUIRE(compile(R"(
let
  var v : string := getchar()
in
end
)"));

  // function ord(s: string) : int
  REQUIRE(compile(R"(
let
  var v : int := ord("")
in
end
)"));

  // function chr(i: int) : string
  REQUIRE(compile(R"(
let
  var v : string := chr(0)
in
end
)"));

  // function size(s: string) : int
  REQUIRE(compile(R"(
let
  var v : int := size("")
in
end
)"));

  // function substring(s:string, first:int, n:int) : string
  REQUIRE(compile(R"(
let
  var v : string := substring("", 0, 0)
in
end
)"));

  // function concat(s1: string, s2: string) : string
  REQUIRE(compile(R"(
let
  var v : string := concat("", "")
in
end
)"));

  // function not(i : integer) : integer
  REQUIRE(compile(R"(
let
  var v : int := not(0)
in
end
)"));

  // function exit(i: int)
  REQUIRE(compile(R"(
exit(0)
)"));
}