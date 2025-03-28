#pragma once

#include <memory>
#include <string>

#include "ast/ast.hpp"
#include "tir/Expr.hpp"

namespace tir {

class Temp : public Expr {
  static int numTemps;
  std::string name;
  std::shared_ptr<parsetree::ast::AstNode> astNode;

public:
  bool isGlobal = false; // Is a static field

  Temp(std::string name,
       std::shared_ptr<parsetree::ast::AstNode> astNode = nullptr,
       bool isGlobal = false)
      : name{name}, astNode{astNode}, isGlobal{isGlobal} {}

  std::string &getName() { return name; }
  std::shared_ptr<parsetree::ast::AstNode> getAstNode() { return astNode; }

  static std::string generateName(std::string prefix = "") {
    numTemps++;
    return (prefix.empty() ? "temp" : prefix) + std::to_string(numTemps);
  }

  std::string label() const override { return "TEMP(" + name + ")"; }

  void visitChildren(InsnMapsBuilder &v) { v.visit(nullptr); }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    return os << "(Temp " << name << ")\n";
  }

  std::vector<std::shared_ptr<Node>> getChildren() const override { return {}; }
};

} // namespace tir
