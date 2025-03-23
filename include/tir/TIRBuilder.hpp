#pragma once

#include "ast.hpp"
#include "tir.hpp"
#include <memory>

namespace tir {

class TIRBuilder {
public:
  TIRBuilder();

  // Statement Builders
  std::shared_ptr<Stmt> buildStmt(std::shared_ptr<parsetree::ast::Stmt> node);

  std::shared_ptr<Stmt> buildBlock(std::shared_ptr<parsetree::ast::Block> node);

  std::shared_ptr<Stmt>
  buildIfStmt(std::shared_ptr<parsetree::ast::IfStmt> node);

  std::shared_ptr<Stmt>
  buildWhileStmt(std::shared_ptr<parsetree::ast::WhileStmt> node);

  std::shared_ptr<Stmt>
  buildForStmt(std::shared_ptr<parsetree::ast::ForStmt> node);

  std::shared_ptr<Stmt>
  buildReturnStmt(std::shared_ptr<parsetree::ast::ReturnStmt> node);

  std::shared_ptr<Expr>
  buildExpressionStmt(std::shared_ptr<parsetree::ast::ExpressionStmt> node);

  std::shared_ptr<Node>
  buildDeclStmt(std::shared_ptr<parsetree::ast::DeclStmt> node);

private:
  int labelCounter = 0;

  // Helpers
  std::string getNextLabelName();
  std::shared_ptr<Label> newLabel();
};

} // namespace tir