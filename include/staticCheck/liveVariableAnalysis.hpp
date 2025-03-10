#pragma once

#include "ast/ast.hpp"
#include "cfgBuilder.hpp"
#include <unordered_set>

namespace static_check {

// TODO
class LiveVariableAnalysisInfo {
  // variables that are used at node n
  std::unordered_set<parsetree::ast::VarDecl> use;

  // variables that are written at node n
  std::unordered_set<parsetree::ast::VarDecl> def;

  // variables live on entry
  std::unordered_set<parsetree::ast::VarDecl> in;

  // variables live on exit
  std::unordered_set<parsetree::ast::VarDecl> out;
};

// TODO
class LiveVariableAnalysis {};

} // namespace static_check
