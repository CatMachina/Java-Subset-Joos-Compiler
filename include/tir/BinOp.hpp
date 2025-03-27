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

  static std::shared_ptr<Expr> makeExpr(OpType op, std::shared_ptr<Expr> left,
                                        std::shared_ptr<Expr> right) {
    return std::make_shared<BinOp>(op, left, right);
  }
  static std::shared_ptr<Expr> makeNegate(std::shared_ptr<Expr> negated);

  std::ostream &print(std::ostream &os, int indent = 0) const override;
};

} // namespace tir
