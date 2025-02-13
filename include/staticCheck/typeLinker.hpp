#pragma once

#include "ast/ast.hpp"
#include "globalEnvironment.hpp"

namespace static_check {

class TypeLinker {
public:
  TypeLinker(std::unique_ptr<parsetree::ast::ASTManager> astManager)
      : astManager(std::move(astManager)) {
    buildSymbolTable();
  }

  ////////////////////// Resolvers ////////////////////
  void resolve();

  // Resolver method to call for each AST
  // Essentially, AST nodes that represent a "use" will be decorated with a
  // Decl object (that is in global env)?

  // Resolver for single-type import

  // Resolver for type (class/interface)

  ////////////////////// Checkers ////////////////////

private:
  void buildSymbolTable();

  std::stack<Environment> envs;
  std::unique_ptr<parsetree::ast::ASTManager> astManager;
  // Env manager (for local envs)

  // Global env? (package - classes/interfaces - fields/methods - variables)
};

} // namespace static_check
