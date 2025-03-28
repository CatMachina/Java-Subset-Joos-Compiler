#pragma once

#include "tir/Expr.hpp"
#include <memory>
#include <string>

namespace tir {

class Const : public Expr {
  int64_t value;

public:
  Const(int64_t value) : value(value) {}
  int getValue() { return value; }
  std::string label() const override {
    return "CONST (" + std::to_string(value) + ")";
  }
  bool isConstant() const override { return true; }

  static std::shared_ptr<Expr> makeExpr(int64_t value) {
    return std::make_shared<Const>(value);
  }

  static std::shared_ptr<Expr> makeWords(int num_words = 1) {
    return makeExpr(4 * num_words);
  }

  void visitChildren(InsnMapsBuilder &v) { v.visit(nullptr); }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    return os << "(Const " << value << ")\n";
  }

  std::vector<std::shared_ptr<Node>> getChildren() const override { return {}; }
};

} // namespace tir
