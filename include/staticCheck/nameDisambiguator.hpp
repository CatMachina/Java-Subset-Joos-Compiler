#pragma once

#include "ast/ast.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace static_check {

class NameDisambiguator {
public:
  NameDisambiguator(std::shared_ptr<parsetree::ast::ASTManager> astManager)
      : astManager(std::move(astManager)) {}

  void resolve();

  void disambiguate(
    std::shared_ptr<parsetree::ast::Expr> expr,
    std::vector<std::shared_ptr<parsetree::ast::MemberName>> &memberNames
  );

private:
  // For scoping
  std::shared_ptr<parsetree::ast::ProgramDecl> currentProgram;
  std::shared_ptr<parsetree::ast::CodeBody> currentContext;
  std::vector<std::unordered_map<std::string, std::shared_ptr<parsetree::ast::Decl>>> scopes;

  void enterProgram(std::shared_ptr<parsetree::ast::ProgramDecl> programDecl);
  void enterContext(std::shared_ptr<parsetree::ast::CodeBody> context);
  void enterScope();
  void leaveScope();
  void addToScope(std::string name, std::shared_ptr<parsetree::ast::Decl> decl);
  std::shared_ptr<parsetree::ast::Decl> findInScopes(std::string name);

  // All ASTs
  std::shared_ptr<parsetree::ast::ASTManager> astManager;

  // AST traversal helpers
  void resolveAST(std::shared_ptr<parsetree::ast::AstNode> node);
  void resolveExpr(std::shared_ptr<parsetree::ast::Expr> expr);
  void resolveVarDecl(std::shared_ptr<parsetree::ast::VarDecl> decl);
};

} // namespace static_check