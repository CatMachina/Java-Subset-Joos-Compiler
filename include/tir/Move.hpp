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
  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Move \n";
    printIndent(os, indent + 1);
    os << "target: { \n";
    target->print(os, indent + 1);
    printIndent(os, indent + 1);
    os << "}\n";
    printIndent(os, indent + 1);
    os << "src: { \n";
    src->print(os, indent + 2);
    printIndent(os, indent + 1);
    os << "}\n";
    printIndent(os, indent);
    os << ")\n";
    return os;
  }
};

} // namespace tir