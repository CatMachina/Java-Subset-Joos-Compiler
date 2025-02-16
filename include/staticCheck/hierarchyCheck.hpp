#pragma once

#include "ast/ast.hpp"
#include "ast/astManager.hpp"
#include "environment.hpp"

namespace static_check {

class HierarchyCheck {
  std::shared_ptr<Package> rootPackage;
  // std::unique_ptr<parsetree::ast::ASTManager> astManager;

  // Get the first package of the qualified name
  std::string getFirstToken(const std::string &qualifiedName) {
    size_t pos = qualifiedName.find('.');
    return (pos == std::string::npos) ? qualifiedName
                                      : qualifiedName.substr(0, pos);
  }

  // Get class definition from root
  // std::shared_ptr<Decl> getClassDef(std::string qualifiedName) {
  //   return getClassDefHelper(rootPackage, qualifiedName);
  // }

  // std::shared_ptr<Decl> getClassDefHelper(std::shared_ptr<Package> root,
  // std::string qualifiedName)
  // {
  //   std::string topPackage = getFirstToken(qualifiedName);
  //   if (topPackage.length() == qualifiedName.length()) {
  //     // Just the class name remaining

  //   }
  //   if (!root->children.count(topPackage)) {
  //     return nullptr;
  //   }
  //   root =
  // }

  // Get interface definition from root

  // Create a set of functions signatures with comparators

  // Get all parent functions

  // Check acyclic class hierarchy
  // bool acyclicCheck() {

  // }

  bool checkProperExtends(std::shared_ptr<Decl> decl) {
    std::shared_ptr<parsetree::ast::Decl> astNode = decl->getAstNode();
    if (dynamic_pointer_cast<parsetree::ast::ClassDecl>(astNode)) {
      // Do class checks
      std::shared_ptr<parsetree::ast::ClassDecl> classDecl =
          dynamic_pointer_cast<parsetree::ast::ClassDecl>(astNode);

      // Check super classes
      std::vector<std::shared_ptr<parsetree::ast::ReferenceType>> superClasses =
          classDecl->getSuperClasses();
      for (std::shared_ptr<parsetree::ast::ReferenceType> superClass :
           superClasses) {
        // Check if extended class is a class
        if (!dynamic_pointer_cast<parsetree::ast::ClassDecl>(
                superClass->getResolvedDecl()))
          return false;

        // Cast to class
        std::shared_ptr<parsetree::ast::ClassDecl> superClassDecl =
            dynamic_pointer_cast<parsetree::ast::ClassDecl>(
                superClass->getResolvedDecl()->getAstNode());
        // Check if class is final
        if (superClassDecl->getModifiers()->isFinal())
          return false;
      }

      // Check interfaces
      std::unordered_set<std::shared_ptr<parsetree::ast::Decl>> uniqueInterfaces;
      std::vector<std::shared_ptr<parsetree::ast::ReferenceType>> interfaces =
          classDecl->getInterfaces();
      for (std::shared_ptr<parsetree::ast::ReferenceType> interface :
           interfaces) {
        // Check if interfaces is an interface
        if (!dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                interface->getResolvedDecl()))
          return false;
        
        // Store and check for duplicate interfaces
        if (uniqueInterfaces.count(interface->getResolvedDecl()->getAstNode()))
          return false;
        uniqueInterfaces.insert(interface->getResolvedDecl()->getAstNode());
      }
    } else if (dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(astNode)){
      // Do interfaces check
      std::shared_ptr<parsetree::ast::InterfaceDecl> interface =
          dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(astNode);

      // Check super interfaces
      std::vector<std::shared_ptr<parsetree::ast::ReferenceType>> superInterfaces =
          interface->getInterfaces();
      std::unordered_set<std::shared_ptr<parsetree::ast::Decl>> uniqueInterfaces;
      for (std::shared_ptr<parsetree::ast::ReferenceType> superInterface :
           superInterfaces)
      {
        // Check if extended interfaces are interfaces
        if (!dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                superInterface->getResolvedDecl()))
          return false;

        // Store and check for duplicate interfaces
        if (uniqueInterfaces.count(superInterface->getResolvedDecl()->getAstNode()))
          return false;
        uniqueInterfaces.insert(superInterface->getResolvedDecl()->getAstNode());
      }
    }
    return true;
  }

  bool traverseTree(std::shared_ptr<Package> node) {
    if (node == nullptr)
      return true;

    bool ret = true;
    std::cout << "Traverse Start\n";
    for (auto &[ch, child] : node->children) {
      std::cout << ch << "\n";
      if (std::holds_alternative<std::shared_ptr<Decl>>(child)) {
        // Perform checks
        std::shared_ptr<Decl> decl = std::get<std::shared_ptr<Decl>>(child);
        ret = ret && checkProperExtends(decl);
      } else if (std::holds_alternative<std::shared_ptr<Package>>(child)) {
        std::shared_ptr<Package> childPackage =
            std::get<std::shared_ptr<Package>>(child);
        ret = ret && traverseTree(childPackage);
      }
    }
    std::cout << "Traverse End\n";
    return ret;
  }

public:
  HierarchyCheck(std::shared_ptr<Package> rootPackage
                 //, std::unique_ptr<parsetree::ast::ASTManager> astManager
                 )
      : rootPackage{rootPackage} // , astManager(std::move(astManager))
  {}

  bool check() {
    // Adhoc integrated checks (tree-level checks)
    // acyclicCheck
    std::cout << "Start check\n";
    // Traverse tree and validate each node (class-level checks)
    return traverseTree(rootPackage);
  }
};

} // namespace static_check
