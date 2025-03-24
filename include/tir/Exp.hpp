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
};

} // namespace tir