# GreenTiger
Solutions to exercises of [Modern Compiler Implementation in C](https://www.amazon.com/Modern-Compiler-Implementation-Andrew-Appel/dp/0521607655) book

## Build status
[![AppVeyor](https://ci.appveyor.com/api/projects/status/f2d26mo5vi2ds2sl?svg=true)](https://ci.appveyor.com/project/dvirtz/greentiger)
[![Travis CI](https://travis-ci.org/dvirtz/GreenTiger.svg?branch=master)](https://travis-ci.org/dvirtz/GreenTiger)

## Overview
Each chapter contains a program which is a step towards building a compiler for the [Tiger](https://www.lrde.epita.fr/~tiger/tiger.html#Tiger-Language-Reference-Manual)
programming language as well as some solutions to other exercises from the book.

The compiler is written in C++ and uses [Boost Spirit](http://www.boost.org/doc/libs/1_64_0/libs/spirit/doc/html/index.html) to parse source code and genarate AST.
