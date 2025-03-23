#pragma once

#include "ast/ast.hpp"
#include "codeGen/exprIRConverter.hpp"
#include "tir/TIR.hpp"

namespace codegen {

class ASTVisitor {
private:
  exprIRConverter exprConverter;

public:
  [[nodiscard]] std::shared_ptr<tir::CompUnit>
  visit(const std::shared_ptr<parsetree::ast::ProgramDecl> program);

  // ...

  [[nodiscard]] std::shared_ptr<tir::Expr>
  visitExpr(const std::shared_ptr<parsetree::ast::Expr> expr);
};

} // namespace codegen
