#pragma once

#include "tir/Expr.hpp"
#include <memory>
#include <string>

class Const : public Expr {
  int64_t value;

public:
  Const(int64_t value) : value(value) {}
  int getValue() { return value; }
  std::string label() { return "CONST (" + std::to_string(value) + ")"; }
  bool isConstant() { return true; }

  static std::shared_ptr<Expr> makeExpr(int64_t value) {
    return std::make_shared<Const>(value);
  }

  static std::shared_ptr<Expr> makeWords(int num_words = 1) {
    return makeExpr(4 * num_words);
  }
};
