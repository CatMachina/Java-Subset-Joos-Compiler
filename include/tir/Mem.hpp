#pragma once

#include <memory>
#include <string>

#include "tir/Expr.hpp"

namespace tir {

class Mem : public Expr {
  std::shared_ptr<Expr> address;

public:
  Mem(std::shared_ptr<Expr> address) : address{address} {}

  std::shared_ptr<Expr> &getAddress() { return address; }

  std::string label() const override { return "MEM"; }

  static std::shared_ptr<Expr> makeExpr(std::shared_ptr<Expr> address) {
    return std::make_shared<Mem>(address);
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Mem \n";
    address->print(os, indent + 1);
    printIndent(os, indent);
    os << ")\n";
    return os;
  }
};

} // namespace tir
