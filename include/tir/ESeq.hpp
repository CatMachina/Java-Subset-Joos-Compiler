#pragma once

#include <memory>
#include <string>

#include "tir/Expr.hpp"
#include "tir/Stmt.hpp"

namespace tir {

class ESeq : public Expr {
  std::shared_ptr<Stmt> stmt;
  std::shared_ptr<Expr> expr;

public:
  ESeq(std::shared_ptr<Stmt> stmt, std::shared_ptr<Expr> expr)
      : stmt(stmt), expr(expr) {}

  std::shared_ptr<Stmt> &getStmt() { return stmt; }
  std::shared_ptr<Expr> &getExpr() { return expr; }
  std::string label() const override { return "ESEQ"; }
  static std::shared_ptr<Expr> makeExpr(std::shared_ptr<Stmt> stmt,
                                        std::shared_ptr<Expr> expr) {
    return std::make_unique<ESeq>(stmt, expr);
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(ESeq\n";
    stmt->print(os, indent + 1);
    printIndent(os, indent + 1);
    os << "expr: {\n";
    expr->print(os, indent + 2);
    printIndent(os, indent + 1);
    os << "}\n";
    printIndent(os, indent);
    os << ")\n";
    return os;
  }
};

} // namespace tir
