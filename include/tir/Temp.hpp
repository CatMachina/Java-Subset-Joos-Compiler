#pragma once

#include <memory>
#include <string>

#include "ast/ast.hpp"
#include "tir/Expr.hpp"

namespace tir {

class Temp : public Expr {
  static size_t num_temps;
  std::string name;
  std::shared_ptr<parsetree::ast::AstNode> astNode;

public:
  bool isGlobal = false; // Is a static field

  Temp(std::string name, std::shared_ptr<parsetree::ast::AstNode> astNode = nullptr,
       bool isGlobal = false)
      : name{name}, astNode{astNode}, isGlobal{isGlobal} {}

  std::string &getName() { return name; }
  std::shared_ptr<parsetree::ast::AstNode> getAstNode() { return astNode; }

  static std::string generateName(std::string prefix = "") {
    num_temps++;
    return (prefix.empty() ? "temp" : prefix) + std::to_string(num_temps);
  }

  std::string label() { return "TEMP(" + name + ")"; }
};

} // namespace tir
