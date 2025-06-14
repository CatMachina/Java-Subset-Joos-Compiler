#pragma once

#include "tir/Expr.hpp"
#include "tir/Stmt.hpp"
#include <memory>
#include <string>

namespace tir {

// From /u/cs444/pub/tir/src/joosc/ir/ast/CJump.java
class CJump : public Stmt {
private:
  std::shared_ptr<Expr> condition; // This
  std::string trueLabel, falseLabel;

public:
  CJump(std::shared_ptr<Expr> condition, std::string trueLabel)
      : condition(condition), trueLabel(trueLabel) {}
  CJump(std::shared_ptr<Expr> condition, std::string trueLabel,
        std::string falseLabel)
      : condition(condition), trueLabel(trueLabel), falseLabel(falseLabel) {}

  std::shared_ptr<Expr> getCondition() { return condition; }
  void setCondition(std::shared_ptr<Expr> condition) {
    this->condition = condition;
  }

  std::string getTrueLabel() { return trueLabel; }
  std::string getFalseLabel() { return falseLabel; }
  void deleteFalseLabel() { falseLabel = ""; }
  bool hasFalseLabel() { return !falseLabel.empty(); }

  void visitChildren(InsnMapsBuilder &v) override { v.visit(condition); }

  std::string label() const override { return "CJUMP"; }
  std::ostream &print(std::ostream &os, int indent = 0) const override;

  std::vector<std::shared_ptr<Node>> getChildren() const override {
    std::vector<std::shared_ptr<Node>> children;
    children.push_back(condition);
    return children;
  }
};

} // namespace tir