#pragma once

#include <memory>

#include "3rd_party/magic_enum.hpp"

#include "Expr.hpp"

namespace tir {

class BinOp : public Expr {
  std::shared_ptr<Expr> left;
  std::shared_ptr<Expr> right;

public:
  enum OpType { ADD, SUB, MUL, DIV, MOD, AND, OR, EQ, NEQ, LT, GT, LEQ, GEQ };
  OpType op;

  BinOp(OpType op, std::shared_ptr<Expr> left, std::shared_ptr<Expr> right)
      : op(op), left{left}, right{right} {}
  std::shared_ptr<Expr> &getLeft() { return left; }
  std::shared_ptr<Expr> &getRight() { return right; }
  std::string label() const override {
    return std::string(magic_enum::enum_name(op));
  }

  bool isConstant() const override {
    return left->isConstant() && right->isConstant();
  }

  static std::shared_ptr<Expr> makeNegate(std::shared_ptr<Expr> negated);

  void visitChildren(InsnMapsBuilder &v) {
    v.visit(left);
    v.visit(right);
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override;

  std::vector<std::shared_ptr<Node>> getChildren() const override {
    std::vector<std::shared_ptr<Node>> children;
    children.push_back(left);
    children.push_back(right);
    return children;
  }
};

} // namespace tir
