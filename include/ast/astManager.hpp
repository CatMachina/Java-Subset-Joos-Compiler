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
      {"util", {"Vector", "List"}},
      {"io", {"PrintStream", "Serializable"}}};

  std::vector<std::shared_ptr<parsetree::ast::ProgramDecl>> asts;

public:
  void addAST(std::shared_ptr<ProgramDecl> ast) { asts.push_back(ast); }
  const std::vector<std::shared_ptr<parsetree::ast::ProgramDecl>> &
  getASTs() const {
    return asts;
  }
  void finalize() {
    for (const auto &pkg : predefinedPackages) {
      auto packageType = std::make_shared<parsetree::ast::UnresolvedType>();
      packageType->addIdentifier("java");
      packageType->addIdentifier(pkg.packageName);

      for (const auto &className : pkg.classes) {
        auto dummyBody = std::make_shared<parsetree::ast::ClassDecl>(className);
        auto dummyAST = std::make_shared<parsetree::ast::ProgramDecl>(
            packageType,
            std::vector<std::shared_ptr<parsetree::ast::ImportDecl>>(),
            dummyBody);
        asts.push_back(dummyAST);
      }
    }
  }
};

} // namespace parsetree::ast
