#pragma once

#include "ast/ast.hpp"
#include "ast/astManager.hpp"
#include "environment.hpp"

namespace static_check {

class HierarchyCheck {
  std::shared_ptr<Package> rootPackage;

  bool isClass(std::shared_ptr<parsetree::ast::AstNode> decl) {
    return !!dynamic_pointer_cast<parsetree::ast::ClassDecl>(decl);
  }

  bool isInterface(std::shared_ptr<parsetree::ast::AstNode> decl) {
    return !!dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(decl);
  }

  // Check acyclic class hierarchy
  bool checkAcyclicHelper(
      std::shared_ptr<parsetree::ast::Decl> decl,
      std::unordered_set<std::shared_ptr<parsetree::ast::AstNode>> &visited,
      std::unordered_set<std::shared_ptr<parsetree::ast::AstNode>> &inStack) {
    inStack.insert(decl);
    visited.insert(decl);
    if (isClass(decl)) {
      std::shared_ptr<parsetree::ast::ClassDecl> classDecl =
          dynamic_pointer_cast<parsetree::ast::ClassDecl>(decl);
      for (auto &superClass : classDecl->getSuperClasses()) {
        if (!superClass)
          continue;
        auto astNode = superClass->getResolvedDecl()->getAstNode();
        // If in current stack, then there's a cycle
        if (inStack.count(astNode))
          return false;
        if (!visited.count(astNode) &&
            !checkAcyclicHelper(astNode, visited, inStack))
          return false;
      }

      for (auto &superInterface : classDecl->getInterfaces()) {
        if (!superInterface)
          continue;
        auto astNode = superInterface->getResolvedDecl()->getAstNode();
        // If in current stack, then there's a cycle
        if (inStack.count(astNode))
          return false;
        if (!visited.count(astNode) &&
            !checkAcyclicHelper(astNode, visited, inStack))
          ;
      }
    } else if (isInterface(decl)) {
      std::shared_ptr<parsetree::ast::InterfaceDecl> interfaceDecl =
          dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(decl);

      for (auto &superInterface : interfaceDecl->getInterfaces()) {
        if (!superInterface)
          continue;
        auto astNode = superInterface->getResolvedDecl()->getAstNode();
        // If in current stack, then there's a cycle
        if (inStack.count(astNode))
          return false;
        if (!visited.count(astNode) &&
            !checkAcyclicHelper(astNode, visited, inStack))
          return false;
      }
    }
    inStack.erase(decl);
    return true;
  }

  bool checkAcyclic(std::shared_ptr<Decl> decl) {
    std::unordered_set<std::shared_ptr<parsetree::ast::AstNode>> visited;
    std::unordered_set<std::shared_ptr<parsetree::ast::AstNode>> inStack;
    return checkAcyclicHelper(decl->getAstNode(), visited, inStack);
  }

  bool checkProperExtends(std::shared_ptr<Decl> decl) {
    std::shared_ptr<parsetree::ast::Decl> astNode = decl->getAstNode();
    if (isClass(astNode)) {
      // Do class checks
      std::shared_ptr<parsetree::ast::ClassDecl> classDecl =
          dynamic_pointer_cast<parsetree::ast::ClassDecl>(astNode);
      // Check super classes
      for (auto &superClass : classDecl->getSuperClasses()) {
        if (!superClass || !superClass->getResolvedDecl() ||
            !superClass->getResolvedDecl()->getAstNode())
          continue;

        // Check if extended class is a class
        if (!isClass(superClass->getResolvedDecl()->getAstNode()))
          return false;

        // Cast to class
        std::shared_ptr<parsetree::ast::ClassDecl> superClassDecl =
            dynamic_pointer_cast<parsetree::ast::ClassDecl>(
                superClass->getResolvedDecl()->getAstNode());
        // Check if class is final
        if (superClassDecl->getModifiers() &&
            superClassDecl->getModifiers()->isFinal())
          return false;
      }

      // Check interfaces
      std::unordered_set<std::shared_ptr<parsetree::ast::Decl>>
          uniqueInterfaces;
      for (auto &superInterface : classDecl->getInterfaces()) {
        if (!superInterface || !superInterface->getResolvedDecl() ||
            !superInterface->getResolvedDecl()->getAstNode())
          continue;

        // Check if interfaces is an interface
        if (!isInterface(superInterface->getResolvedDecl()->getAstNode()))
          return false;

        // Store and check for duplicate interfaces
        if (uniqueInterfaces.count(
                superInterface->getResolvedDecl()->getAstNode()))
          return false;
        uniqueInterfaces.insert(
            superInterface->getResolvedDecl()->getAstNode());
      }
    } else if (isInterface(astNode)) {
      // Do interfaces check
      std::shared_ptr<parsetree::ast::InterfaceDecl> interfaceDecl =
          dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(astNode);

      // Check super interfaces
      std::unordered_set<std::shared_ptr<parsetree::ast::Decl>>
          uniqueInterfaces;
      for (auto &superInterface : interfaceDecl->getInterfaces()) {
        if (!superInterface || !superInterface->getResolvedDecl() ||
            !superInterface->getResolvedDecl()->getAstNode())
          continue;
        // Check if interfaces is an interface
        if (!isInterface(superInterface->getResolvedDecl()->getAstNode()))
          return false;

        // Store and check for duplicate interfaces
        if (uniqueInterfaces.count(
                superInterface->getResolvedDecl()->getAstNode()))
          return false;
        uniqueInterfaces.insert(
            superInterface->getResolvedDecl()->getAstNode());
      }
    }
    return true;
  }

  bool checkMethodSignatures(
      std::shared_ptr<Decl> decl,
      std::unordered_map<std::string,
                         std::shared_ptr<parsetree::ast::MethodDecl>>
          methodMap) {
    std::shared_ptr<parsetree::ast::Decl> astNode = decl->getAstNode();
    bool valid = true;

    auto checkMethods = [&](auto decl) {
      for (auto &method : decl->getMethods()) {
        std::string signature = method->getSignature();
        std::string returnTypeStr =
            method->getReturnType() ? method->getReturnType()->toString() : "";

        if (methodMap.count(signature)) {
          std::string existingReturnTypeStr =
              methodMap[signature]->getReturnType()
                  ? methodMap[signature]->getReturnType()->toString()
                  : "";

          if (existingReturnTypeStr != returnTypeStr) {
            std::cout
                << "Error: Conflicting methods in " << decl->getName() << ": "
                << method->getName()
                << " have the same signature but different return types.\n";
            valid = false;
          }
        } else {
          methodMap[signature] = method;
        }
      }
    };

    if (auto classDecl =
            std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(astNode)) {
      checkMethods(classDecl);

      if (!classDecl->getSuperClasses().empty()) {
        for (auto &superClass : classDecl->getSuperClasses()) {
          if (superClass && superClass->getResolvedDecl()) {
            checkMethodSignatures(superClass->getResolvedDecl(), methodMap);
          }
        }
      }
    } else if (auto interfaceDecl =
                   std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                       astNode)) {
      checkMethods(interfaceDecl);

      if (!interfaceDecl->getInterfaces().empty()) {
        for (auto &superInterface : interfaceDecl->getInterfaces()) {
          if (superInterface && superInterface->getResolvedDecl()) {
            checkMethodSignatures(superInterface->getResolvedDecl(), methodMap);
          }
        }
      }
    }

    return valid;
  }

  bool traverseTree(std::shared_ptr<Package> node) {
    if (node == nullptr)
      return true;

    std::cout << "Traverse Start\n";
    for (auto &[ch, child] : node->children) {
      if (std::holds_alternative<std::shared_ptr<static_check::Decl>>(child)) {
        // Perform checks
        std::shared_ptr<Decl> decl = std::get<std::shared_ptr<Decl>>(child);
        bool ret = checkProperExtends(decl) && checkAcyclic(decl);
         && checkMethodSignatures(
                decl, std::unordered_map<
                          std::string,
                          std::shared_ptr<parsetree::ast::MethodDecl>>());
        if (!ret)
          return false;
      } else if (std::holds_alternative<std::shared_ptr<Package>>(child)) {
        std::shared_ptr<Package> childPackage =
            std::get<std::shared_ptr<Package>>(child);
        if (!traverseTree(childPackage))
          return false;
      }
    }
    std::cout << "Traverse End\n";
    return true;
  }

public:
  HierarchyCheck(std::shared_ptr<Package> rootPackage)
      : rootPackage{rootPackage} {}

  bool check() {
    // Traverse tree and validate each node
    return traverseTree(rootPackage);
  }
};

} // namespace static_check
