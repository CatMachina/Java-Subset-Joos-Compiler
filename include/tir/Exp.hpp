#pragma once

#include "tir/Expr.hpp"
#include "tir/Stmt.hpp"

namespace tir {

class Exp : public Stmt {
private:
  std::shared_ptr<Expr> expr;

public:
  explicit Exp(std::shared_ptr<Expr> expr) : expr(expr) {}

  std::shared_ptr<Expr> getExpr() const { return expr; }

  std::string label() const override { return "EXP"; }

  void visitChildren(InsnMapsBuilder &v) { v.visit(expr); }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Exp\n";
    expr->print(os, indent + 1);
    printIndent(os, indent);
    os << ")\n";
    return os;
  }

  std::vector<std::shared_ptr<Node>> getChildren() const override {
    std::vector<std::shared_ptr<Node>> children;
    children.push_back(expr);
    return children;
  }
};

} // namespace tir