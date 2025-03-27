#pragma once

#include <memory>
#include <string>

#include "tir/Expr.hpp"

namespace tir {

class Name : public Expr {
  std::string name;

public:
  bool isGlobal = false;
  Name(std::string name, bool isGlobal = false)
      : name{name}, isGlobal{isGlobal} {}
  std::string &getName() { return name; }
  std::string label() const override { return "NAME(" + name + ")"; }

  static std::shared_ptr<Expr> makeExpr(std::string name,
                                        bool isGlobal = false) {
    return std::make_shared<Name>(name, isGlobal);
  }

  static std::shared_ptr<Expr> makeMalloc() { return makeExpr("__malloc"); }
  static std::shared_ptr<Expr> makeException() {
    return makeExpr("__exception");
  }
  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    return os << "(Name " << name << ")\n";
  }
};

} // namespace tir
