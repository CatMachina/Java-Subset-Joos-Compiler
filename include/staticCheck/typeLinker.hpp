#pragma once

#include "ast/ast.hpp"
#include "environment.hpp"
#include <memory>
#include <stack>

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
  // First pass?
  void buildSymbolTable();

  std::stack<std::shared_ptr<Environment>> envs;
  std::unique_ptr<parsetree::ast::ASTManager> astManager;

  // Global env? (package - classes/interfaces - fields/methods - variables)
  std::unique_ptr<GlobalEnvironment> globalEnv;

  void enterScope() { envs.push( std::make_shared<Environment>()); };
  void leaveScope() { envs.pop(); };

};

} // namespace static_check
