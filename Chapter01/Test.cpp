#include "Exercises.h"
#include "Program.h"
#define CATCH_CONFIG_MAIN
#include <catch.hpp>

using namespace GreenTiger;

TEST_CASE("program") {
  auto program = CompoundStatement(
      AssignmentStatement("a", BinaryOperation(5, BinOp::PLUS, 3)),
      CompoundStatement(
          AssignmentStatement(
              "b",
              ExpressionSequence(
                  PrintStatement("a", BinaryOperation("a", BinOp::MINUS, 1)),
                  BinaryOperation(10, BinOp::TIMES, "a"))),
          PrintStatement("b")));

  REQUIRE(maxargs(program) == 2);
  VariableValues values;
  interpStm(program, values);
  REQUIRE(values.size() == 2);
  REQUIRE(values["a"] == 8);
  REQUIRE(values["b"] == 80);
}

TEST_CASE("sanity") {
  REQUIRE(maxargs(PrintStatement()) == 0);

  VariableValues values;
  REQUIRE(interpExp(0, values) == 0);
  REQUIRE(interpExp(-1, values) == -1);
  REQUIRE(interpExp(std::numeric_limits<int>::min(), values) ==
          std::numeric_limits<int>::min());
  REQUIRE(interpExp(std::numeric_limits<int>::max(), values) ==
          std::numeric_limits<int>::max());

  values["a"] = 8;
  REQUIRE(interpExp("a", values) == 8);
  REQUIRE(interpExp(BinaryOperation("a", BinOp::MINUS, "a"), values) == 0);
}

TEST_CASE("tree") {
  using namespace std::string_literals;
  auto tree = insertKeys("t"s, "s"s, "p"s, "i"s, "p"s, "f"s, "b"s, "s"s, "t"s);
  REQUIRE(member("p"s, tree));
  REQUIRE_FALSE(member("x"s, tree));
}

TEST_CASE("tree with payload") {
  using namespace std::string_literals;
  auto tree = insertPayload("a"s, 2, insertPayload("b"s, 3));
  REQUIRE(lookup("a"s, tree) == 2);
  REQUIRE(lookup("b"s, tree) == 3);
}