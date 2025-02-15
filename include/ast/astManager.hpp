#pragma once

#include "ast/astNode.hpp"

namespace parsetree::ast {

class ASTManager {
private:
  std::vector<std::shared_ptr<parsetree::ast::ProgramDecl>> asts;

public:
  void addAST(std::shared_ptr<ProgramDecl> ast) { asts.push_back(ast); }
  const std::vector<std::shared_ptr<parsetree::ast::ProgramDecl>> &
  getASTs() const {
    return asts;
  }
  void finalize() {
    // java program imports java.lang.*
    auto javaPkg = std::make_shared<parsetree::ast::UnresolvedType>();
    javaPkg->addIdentifier("java");
    javaPkg->addIdentifier("lang");
    // current workaround: add a dummy AST
    auto dummyBody = std::make_shared<parsetree::ast::ClassDecl>("Object");
    auto dummyAST = std::make_shared<parsetree::ast::ProgramDecl>(
        javaPkg, std::vector<std::shared_ptr<parsetree::ast::ImportDecl>>(),
        dummyBody);
    asts.push_back(dummyAST);

    dummyBody = std::make_shared<parsetree::ast::ClassDecl>("Number");
    dummyAST = std::make_shared<parsetree::ast::ProgramDecl>(
        javaPkg, std::vector<std::shared_ptr<parsetree::ast::ImportDecl>>(),
        dummyBody);
    asts.push_back(dummyAST);

    dummyBody = std::make_shared<parsetree::ast::ClassDecl>("String");
    dummyAST = std::make_shared<parsetree::ast::ProgramDecl>(
        javaPkg, std::vector<std::shared_ptr<parsetree::ast::ImportDecl>>(),
        dummyBody);
    asts.push_back(dummyAST);

    dummyBody = std::make_shared<parsetree::ast::ClassDecl>("Integer");
    dummyAST = std::make_shared<parsetree::ast::ProgramDecl>(
        javaPkg, std::vector<std::shared_ptr<parsetree::ast::ImportDecl>>(),
        dummyBody);
    asts.push_back(dummyAST);

    dummyBody = std::make_shared<parsetree::ast::ClassDecl>("Thread");
    dummyAST = std::make_shared<parsetree::ast::ProgramDecl>(
        javaPkg, std::vector<std::shared_ptr<parsetree::ast::ImportDecl>>(),
        dummyBody);
    asts.push_back(dummyAST);

    auto utilPkg = std::make_shared<parsetree::ast::UnresolvedType>();
    utilPkg->addIdentifier("java");
    utilPkg->addIdentifier("util");
    dummyBody = std::make_shared<parsetree::ast::ClassDecl>("Vector");
    dummyAST = std::make_shared<parsetree::ast::ProgramDecl>(
        utilPkg, std::vector<std::shared_ptr<parsetree::ast::ImportDecl>>(),
        dummyBody);
    asts.push_back(dummyAST);

    auto ioPkg = std::make_shared<parsetree::ast::UnresolvedType>();
    ioPkg->addIdentifier("java");
    ioPkg->addIdentifier("io");
    dummyBody = std::make_shared<parsetree::ast::ClassDecl>("PrintStream");
    dummyAST = std::make_shared<parsetree::ast::ProgramDecl>(
        ioPkg, std::vector<std::shared_ptr<parsetree::ast::ImportDecl>>(),
        dummyBody);
    asts.push_back(dummyAST);

    dummyBody = std::make_shared<parsetree::ast::ClassDecl>("Serializable");
    dummyAST = std::make_shared<parsetree::ast::ProgramDecl>(
        ioPkg, std::vector<std::shared_ptr<parsetree::ast::ImportDecl>>(),
        dummyBody);
    asts.push_back(dummyAST);
  }
};

} // namespace parsetree::ast
