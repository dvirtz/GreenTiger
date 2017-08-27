#pragma once
#include "AbstractSyntaxTree.h"
#include "variantMatch.h"
#include <vector>

namespace tiger {

///////////////////////////////////////////////////////////////////////////////
//  The annotation handler links the AST to a map of iterator positions
//  for the purpose of subsequent semantic error handling when the
//  program is being compiled.
///////////////////////////////////////////////////////////////////////////////
template <typename Iterator> 
class Annotation {
public:
  template <typename, typename> struct result { typedef void type; };

  void operator()(ast::Tagged &ast, Iterator pos) const {
    int id = iters.size();
    iters.push_back(pos);
    ast.id = id;
  }

  void operator()(ast::FunctionDeclaration &function, Iterator pos) const {
    (*this)(function.name, pos);
  }

  void operator()(ast::TypeDeclaration &type, Iterator pos) const {
    (*this)(type.name, pos);
  }

  void operator()(ast::VarDeclaration &var, Iterator pos) const {
    (*this)(var.name, pos);
  }

  void operator()(ast::Declaration& decl, Iterator pos) const {
    helpers::match(decl)(
      [&](ast::FunctionDeclarations& funcDecs) { (*this)(funcDecs, pos); },
      [&](ast::TypeDeclarations& typeDecs) { (*this)(typeDecs, pos); },
      [&](ast::VarDeclaration& varDec) { (*this)(varDec, pos); }
      );
  }

  void operator()(ast::Expression& expression, Iterator pos) const {
    helpers::match(expression)(
      [&](ast::VarExpression& e) { (*this)(e.first, pos); },
      [&](ast::CallExpression& e) { (*this)(e.func, pos); },
      [&](ast::ArithmeticExpression& e) { (*this)(e.first, pos); },
      [&](ast::RecordExpression& e) { (*this)(e.type, pos); },
      [&](ast::AssignExpression& e) { (*this)(e.var.first, pos); },
      [&](ast::IfExpression& e) { (*this)(e.test, pos); },
      [&](ast::WhileExpression& e) { (*this)(e.test, pos); },
      [&](ast::ForExpression& e) { (*this)(e.var, pos); },
      [&](ast::LetExpression& e) { (*this)(e.decs, pos); },
      [&](ast::ArrayExpression& e) { (*this)(e.type, pos); },
      [&](auto& e) {/*noop*/}
      );
  }

  template<typename T>
  void operator()(std::vector<T>& vec, Iterator pos) const {
    if (!vec.empty()) {
      (*this)(vec.front(), pos);
    }
  }

private:
  mutable std::vector<Iterator> iters;
};
} // namespace tiger