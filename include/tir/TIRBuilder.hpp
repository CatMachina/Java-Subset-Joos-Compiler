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

  // Declaration Builders
  std::shared_ptr<Node> buildDecl(std::shared_ptr<parsetree::ast::Decl> node);

  std::shared_ptr<tir::CompUnit>
  buildProgram(std::shared_ptr<parsetree::ast::ProgramDecl> node);

  std::shared_ptr<FuncDecl>
  buildMethodDecl(std::shared_ptr<parsetree::ast::MethodDecl> node);

  std::shared_ptr<Stmt>
  buildVarDecl(std::shared_ptr<parsetree::ast::VarDecl> node);

  std::shared_ptr<Stmt>
  buildFieldDecl(std::shared_ptr<parsetree::ast::FieldDecl> node);

  std::vector<std::shared_ptr<Node>>
  buildClassDecl(std::shared_ptr<parsetree::ast::ClassDecl> node);

private:
  int labelCounter = 0;

  // Helpers
  std::string getNextLabelName();
  std::shared_ptr<Label> newLabel();
};

} // namespace tir