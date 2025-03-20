#pragma once

#include <memory>
#include <string>

#include "tir/Expr.hpp"

class Name : public Expr {
  std::string name;

public:
  bool isGlobal = false;
  Name(std::string name, bool isGlobal = false)
      : name{name}, isGlobal{isGlobal} {}
  std::string &getName() { return name; }
  std::string label() { return "NAME(" + name + ")"; }

  static std::shared_ptr<ExpressionIR> makeExpr(std::string name,
                                                bool isGlobal = false) {
    return std::make_shared<Name>(name, isGlobal);
  }

  static std::shared_ptr<ExpressionIR> makeMalloc() {
    return makeExpr("__malloc");
  }
  static std::shared_ptr<ExpressionIR> makeException() {
    return makeExpr("__exception");
  }
};
