#include "EscapeAnalyser.h"
#include "fusionAccumulate.h"
#include "variantMatch.h"

namespace tiger {

void EscapeAnalyser::analyse(ast::Expression &exp) {
  helpers::match(exp)(
    [this](ast::VarExpression &varExp) { analyseVar(varExp); },
    [this](ast::LetExpression &letExp) { analyseLet(letExp); },
    [this](ast::ForExpression &forExp) { analyseFor(forExp); },
    [this](auto& exp) {this->analyse(exp);}
  );
}

void
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

  for (auto& r : exp.rest)
  {
    helpers::match(r)(
      [this](ast::Subscript& s) {analyse(s.exp);},
      [](auto& /*default*/) {}
    );
  }
}

void
EscapeAnalyser::analyseLet(ast::LetExpression &exp) {
  m_environments.emplace_back();

  for (auto &dec : exp.decs) {
    helpers::match(dec)(
        [&](ast::FunctionDeclarations &decs) { analyseFuncDec(decs); },
        [&](ast::VarDeclaration &dec) { analyseVarDec(dec); },
        [&](auto & /*default*/) {});
  }

  analyse(exp.body);

  m_environments.pop_back();
}

void
EscapeAnalyser::analyseFuncDec(ast::FunctionDeclarations &decs) {
  for (auto &dec : decs) {
    Environment newEnv;
    for (auto &param : dec.params) {
      param.escapes = false;
      newEnv.emplace(param.name.name, param.escapes);
    }

    m_environments.push_back(std::move(newEnv));
    
    analyse(dec.body);

    m_environments.pop_back();
  }
}

void
EscapeAnalyser::analyseVarDec(ast::VarDeclaration &dec) {
  dec.escapes = false;
  m_environments.back().emplace(dec.name, dec.escapes);
}

void
EscapeAnalyser::analyseFor(ast::ForExpression &exp) {
  exp.escapes = false;
  Environment newEnv;
  newEnv.emplace(exp.var.name, exp.escapes);

  m_environments.push_back(std::move(newEnv));

  analyse(exp.body);

  m_environments.pop_back();
}

} // namespace tiger
