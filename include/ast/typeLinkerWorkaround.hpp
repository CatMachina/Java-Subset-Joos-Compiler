#pragma once

#include "ast/ast.hpp"
#include "staticCheck/typeLinker.hpp"
#include <unordered_map>
#include <unordered_set>

namespace parsetree::ast::workaround {

class Workaround;

class ClassDeclWrapper {
public:
  std::shared_ptr<parsetree::ast::ClassDecl> classDecl;
  Workaround *workaround;
  ClassDeclWrapper(std::shared_ptr<parsetree::ast::ClassDecl> classDecl,
                   Workaround *workaround)
      : classDecl(classDecl), workaround(workaround) {}

  std::unordered_set<std::shared_ptr<parsetree::ast::MethodDecl>>
  getAllMethods();
};

class InterfaceDeclWrapper {
public:
  std::shared_ptr<parsetree::ast::InterfaceDecl> interfaceDecl;
  Workaround *workaround;
  InterfaceDeclWrapper(
      std::shared_ptr<parsetree::ast::InterfaceDecl> interfaceDecl,
      Workaround *workaround)
      : interfaceDecl(interfaceDecl), workaround(workaround) {}

  std::unordered_set<std::shared_ptr<parsetree::ast::MethodDecl>>
  getAllMethods();
};

class Workaround {
public:
  std::unordered_map<
      std::shared_ptr<parsetree::ast::ClassDecl>,
      std::unordered_set<std::shared_ptr<parsetree::ast::MethodDecl>>>
      classMethods;
  std::unordered_map<
      std::shared_ptr<parsetree::ast::InterfaceDecl>,
      std::unordered_set<std::shared_ptr<parsetree::ast::MethodDecl>>>
      interfaceMethods;

  std::shared_ptr<static_check::TypeLinker> typeLinker;

  Workaround(std::shared_ptr<static_check::TypeLinker> linker)
      : typeLinker(linker) {}

  void visitAST(std::shared_ptr<parsetree::ast::AstNode> node) {
    if (auto classDecl =
            std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(node)) {
      if (classMethods.find(classDecl) == classMethods.end()) {
        auto wrapper = ClassDeclWrapper(classDecl, this);
        classMethods[classDecl] = wrapper.getAllMethods();
        std::cout << "workaround class: " << classDecl->getFullName()
                  << std::endl;
        for (const auto &method : classMethods[classDecl]) {
          std::cout << "  method: " << method->getFullName() << std::endl;
        }
      }
    } else if (auto interfaceDecl =
                   std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                       node)) {
      if (interfaceMethods.find(interfaceDecl) == interfaceMethods.end()) {
        auto wrapper = InterfaceDeclWrapper(interfaceDecl, this);
        interfaceMethods[interfaceDecl] = wrapper.getAllMethods();
        std::cout << "workaround interface: " << interfaceDecl->getFullName()
                  << std::endl;
        for (const auto &method : interfaceMethods[interfaceDecl]) {
          std::cout << "  method: " << method->getFullName() << std::endl;
        }
      }
    }

    for (const auto &child : node->getChildren()) {
      if (!child)
        continue;
      visitAST(child);
    }
  }

  void visit(std::shared_ptr<parsetree::ast::ASTManager> astManager) {
    for (auto ast : astManager->getASTs()) {
      visitAST(ast);
    }
  }

  std::unordered_set<std::shared_ptr<parsetree::ast::MethodDecl>>
  getClassMethods(std::shared_ptr<parsetree::ast::ClassDecl> classDecl) {
    return classMethods[classDecl];
  }

  std::unordered_set<std::shared_ptr<parsetree::ast::MethodDecl>>
  getInterfaceMethods(
      std::shared_ptr<parsetree::ast::InterfaceDecl> interfaceDecl) {
    return interfaceMethods[interfaceDecl];
  }
};

} // namespace parsetree::ast::workaround