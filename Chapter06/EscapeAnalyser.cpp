#include "EscapeAnalyser.h"
#include "fusionAccumulate.h"
#include "variantMatch.h"
#include <boost/fusion/include/is_sequence.hpp>

namespace tiger {

EscapeAnalyser::result_type EscapeAnalyser::analyse(ast::Expression &exp) {
  return helpers::match(exp)(
      [this](ast::VarExpression &varExp) { return analyseVar(varExp); },
      [this](ast::LetExpression &letExp) { return analyseLet(letExp); },
      [this](auto &exp) {
        return helpers::accumulate(exp, true)(
            [this](bool state, ast::Expression &subexp) {
              return state && this->analyse(subexp);
            },
            [](bool state, auto & /*default*/) { return state; });
      });
}

EscapeAnalyser::result_type
EscapeAnalyser::analyseVar(ast::VarExpression &exp) {
  // search environments for this variable
  for (auto it = m_environments.rbegin(); it != m_environments.rend(); ++it) {
    auto itt = it->find(exp.first);
    if (itt != it->end()) {
      if (it != m_environments.rbegin()) {
        // variable escapes if it is used in a nested frame
        itt->second.get() = true;
      }
    }
  }

  return true;
}

EscapeAnalyser::result_type
EscapeAnalyser::analyseLet(ast::LetExpression &exp) {
  m_environments.emplace_back();

  for (auto &dec : exp.decs) {
    auto res = helpers::match(dec)(
        [&](ast::FunctionDeclarations &decs) { return analyseFuncDec(decs); },
        [&](ast::VarDeclaration &dec) { return analyseVarDec(dec); },
        [&](auto & /*default*/) { return true; });
    if (res == false) {
      return res;
    }
  }

  for (auto &subExp : exp.body) {
    if (analyse(subExp) == false) {
      return false;
    }
  }

  m_environments.pop_back();

  return true;
}

EscapeAnalyser::result_type
EscapeAnalyser::analyseFuncDec(ast::FunctionDeclarations &decs) {
  for (auto &dec : decs) {
    Environment newEnv;
    for (auto &param : dec.params) {
      param.escapes = false;
      newEnv.emplace(param.name.name, param.escapes);
    }

    m_environments.push_back(newEnv);

    if (analyse(dec.body) == false) {
      return false;
    }

    m_environments.pop_back();
  }

  return true;
}

EscapeAnalyser::result_type
EscapeAnalyser::analyseVarDec(ast::VarDeclaration &dec) {
  dec.escapes = false;
  m_environments.back().emplace(dec.name, dec.escapes);

  return true;
}

} // namespace tiger
