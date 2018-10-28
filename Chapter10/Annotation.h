#pragma once
#include "AbstractSyntaxTree.h"
#include "variantMatch.h"
#include <vector>

#define ANNOTATE_NODE_A(r, _, name)                                            \
  on_success(name, phoenix::bind(phoenix::cref(annotation), _val, _1));        \
  /***/

#define ANNOTATE_NODES(seq)                                                    \
  BOOST_PP_SEQ_FOR_EACH(ANNOTATE_NODE_A, _, seq)                               \
  /***/

namespace tiger {

///////////////////////////////////////////////////////////////////////////////
//  The annotation handler links the AST to a map of iterator positions
//  for the purpose of subsequent semantic error handling when the
//  program is being compiled.
///////////////////////////////////////////////////////////////////////////////
template <typename Iterator> class Annotation {
public:
  template <typename, typename> struct result { typedef void type; };

  Annotation()                   = default;
  Annotation(const Annotation &) = delete;
  Annotation &operator=(const Annotation &) = delete;

  void operator()(ast::Tagged &ast, Iterator pos) const {
    auto id = iters.size();
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

  void operator()(ast::Declaration &decl, Iterator pos) const {
    helpers::match(decl)(
      [&](ast::FunctionDeclarations &funcDecs) { (*this)(funcDecs, pos); },
      [&](ast::TypeDeclarations &typeDecs) { (*this)(typeDecs, pos); },
      [&](ast::VarDeclaration &varDec) { (*this)(varDec, pos); });
  }

  //   void operator()(ast::Expression& expression, Iterator pos) const {
  //     helpers::match(expression)(
  //       [&](ast::Tagged& e) { (*this)(e, pos); }
  //       );
  //   }
  //
  //   template<typename T>
  //   void operator()(std::vector<T>& vec, Iterator pos) const {
  //     if (!vec.empty()) {
  //       (*this)(vec.front(), pos);
  //     }
  //   }

  Iterator iteratorFromId(size_t id) { return iters[id]; }

private:
  mutable std::vector<Iterator> iters;
};
} // namespace tiger