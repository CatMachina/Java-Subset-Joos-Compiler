#pragma once

#include "tir/Name.hpp"
#include "tir/Stmt.hpp"
#include <memory>

namespace tir {

// From /u/cs444/pub/tir/src/joosc/ir/ast/Jump.java
class Jump : public Stmt {
private:
  std::shared_ptr<Expr> target; // target usually takes a Name

public:
  Jump(std::shared_ptr<Expr> target) : target(target) {}

  std::shared_ptr<Expr> getTarget() { return target; }

  std::string label() const override { return "JUMP"; }

  void visitChildren(InsnMapsBuilder &v) override { v.visit(target); }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Jump\n";
    target->print(os, indent + 1);
    printIndent(os, indent);
    os << ")\n";
    return os;
  }

  std::vector<std::shared_ptr<Node>> getChildren() const override {
    std::vector<std::shared_ptr<Node>> children;
    children.push_back(target);
    return children;
  }
};

} // namespace tir