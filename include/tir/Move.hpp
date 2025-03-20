#pragma once

#include "tir/Expr.hpp"
#include "tir/Stmt.hpp"

namespace tir {

class Move : public Stmt {
private:
  std::shared_ptr<Expr> target;
  std::shared_ptr<Expr> src;

public:
  Move(std::shared_ptr<Expr> target, std::shared_ptr<Expr> src)
      : target(target), src(src) {}

  std::shared_ptr<Expr> getTarget() const { return target; }
  std::shared_ptr<Expr> getSource() const { return src; }

  std::string label() const override { return "MOVE"; }
};

} // namespace tir