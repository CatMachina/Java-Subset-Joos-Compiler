#pragma once

#include "ast/astNode.hpp"

namespace parsetree::ast {

class ASTManager {
private:
  struct PackageClasses {
    std::string packageName;
    std::vector<std::string> classes;
  };

  std::vector<PackageClasses> predefinedPackages = {
      {"lang",
       {"Object", "Number", "String", "Integer", "Thread", "Cloneable"}},
      {"util", {"Vector"}},
      {"io", {"PrintStream", "Serializable"}}};

  std::vector<std::shared_ptr<parsetree::ast::ProgramDecl>> asts;

public:
  void addAST(std::shared_ptr<ProgramDecl> ast) { asts.push_back(ast); }
  const std::vector<std::shared_ptr<parsetree::ast::ProgramDecl>> &
  getASTs() const {
    return asts;
  }
};

} // namespace parsetree::ast
