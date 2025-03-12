#pragma once

#include "ast/ast.hpp"
#include "ast/astManager.hpp"
#include "typeResolver.hpp"

namespace static_check {

class ASTValidator {
public:
  ASTValidator(std::shared_ptr<TypeResolver> typeResolver)
      : typeResolver(typeResolver) {}

  void validate(std::shared_ptr<parsetree::ast::ASTManager> astManager) {
    for (auto ast : astManager->getASTs()) {
      validateProgram(ast);
    }
  }

private:
  void validateProgram(std::shared_ptr<parsetree::ast::ProgramDecl> program);
  void validateMethod(std::shared_ptr<parsetree::ast::MethodDecl> method);
  void validateStmt(std::shared_ptr<parsetree::ast::Stmt> stmt);
  void validateReturnStmt(std::shared_ptr<parsetree::ast::ReturnStmt> stmt);
  void validateVarDecl(std::shared_ptr<parsetree::ast::VarDecl> varDecl);
  std::shared_ptr<parsetree::ast::Type>
  getTypeFromExpr(std::shared_ptr<parsetree::ast::Expr> expr);

private:
  std::shared_ptr<TypeResolver> typeResolver;
  std::shared_ptr<parsetree::ast::ProgramDecl> currentProgram;
  std::shared_ptr<parsetree::ast::MethodDecl> currentMethod;
};

} // namespace static_check
