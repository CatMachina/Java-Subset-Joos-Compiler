#pragma once

#include <memory>
#include <string>

#include "ast/ast.hpp"
#include "tir/Expr.hpp"

namespace tir {

class Mem : public Expr {
  std::shared_ptr<Expr> address;
  std::shared_ptr<parsetree::ast::FieldDecl> field;

public:
  Mem(std::shared_ptr<Expr> address,
      std::shared_ptr<parsetree::ast::FieldDecl> field = nullptr)
      : address{address}, field{field} {}

  std::shared_ptr<Expr> &getAddress() { return address; }

  std::string label() const override { return "MEM"; }

  std::shared_ptr<parsetree::ast::FieldDecl> getField() const { return field; }

  void visitChildren(InsnMapsBuilder &v) override { v.visit(address); }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Mem\n";
    address->print(os, indent + 1);
    printIndent(os, indent);
    os << ")\n";
    return os;
  }

  std::vector<std::shared_ptr<Node>> getChildren() const override {
    std::vector<std::shared_ptr<Node>> children;
    children.push_back(address);
    return children;
  }
};

} // namespace tir
