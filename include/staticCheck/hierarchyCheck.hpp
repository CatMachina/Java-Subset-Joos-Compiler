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

  bool checkDuplicateSignatures(std::shared_ptr<Decl> decl) {
    std::shared_ptr<parsetree::ast::Decl> astNode = decl->getAstNode();
    if (auto classDecl =
            std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(astNode)) {

      // Check to see if class contains duplicate methods or constructors
      std::vector<std::unordered_set<std::string>> sameClassSignatures(
          2, std::unordered_set<std::string>());
      for (auto &method : classDecl->getMethods()) {
        // Continue if method is nullptr
        if (!method)
          continue;
        // Differentiate check/store depending on if constructor or not
        int index = method->isConstructor() ? 1 : 0;
        std::string signature = method->getSignature();
        if (sameClassSignatures[index].count(signature))
          return false;
        sameClassSignatures[index].insert(signature);
      }
    } else if (auto interfaceDecl =
                   std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                       astNode)) {

      // Check to see if interface contains duplicate methods
      std::unordered_set<std::string> sameClassMethodSignatures;
      for (auto &method : interfaceDecl->getMethods()) {
        // Continue if method is nullptr
        if (!method)
          continue;
        // Check/store method signatures
        std::string signature = method->getSignature();
        if (sameClassMethodSignatures.count(signature))
          return false;
        sameClassMethodSignatures.insert(signature);
      }
    }

    return true;
  }

  bool getInheritedMethods(
      std::shared_ptr<parsetree::ast::Decl> astNode,
      std::unordered_map<std::string,
                         std::shared_ptr<parsetree::ast::MethodDecl>>
          &abstractMethodMap,
      std::unordered_map<std::string,
                         std::shared_ptr<parsetree::ast::MethodDecl>>
          &methodMap) {
    if (auto classDecl =
            std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(astNode)) {
      for (auto &superInterface : classDecl->getInterfaces()) {
        if(!superInterface || !superInterface->getResolvedDecl() || !superInterface->getResolvedDecl()->getAstNode())
          continue;
        auto superInterfaceDecl =
            std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(superInterface->getResolvedDecl()->getAstNode());
        if(superInterfaceDecl && !getInheritedMethods(superInterfaceDecl, abstractMethodMap, methodMap))
          return false;
      }
      for (auto &superClass : classDecl->getSuperClasses()) {
        if (!superClass || !superClass->getResolvedDecl() || !superClass->getResolvedDecl()->getAstNode())
          continue;
        auto superClassDecl =
            std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(superClass->getResolvedDecl()->getAstNode());
        if(superClass && !getInheritedMethods(superClassDecl, abstractMethodMap, methodMap))
          return false;
      }
      // Resolve current interface's abstract methods
      for (auto &method : classDecl->getMethods())
      {
        if(!method)
          continue;
        std::string signature = method->getSignature();
        // Some inherited functions have same signature but different return types
        if (abstractMethodMap.count(signature) && abstractMethodMap[signature]->getReturnType()->toString() != method->getReturnType()->toString())
          return false;
        if (methodMap.count(signature) && methodMap[signature]->getReturnType()->toString() != method->getReturnType()->toString())
          return false;
        // If same signature and return, don't insert; otherwise not present, so insert
        if (method->getModifiers() && method->getModifiers()->isAbstract() && !abstractMethodMap.count(signature))
        {
          abstractMethodMap[signature] = method;
        }
        else if (!methodMap.count(signature) && !method->isConstructor()) {
          methodMap[signature] = method;
        }
      }
    }
    else if (auto interfaceDecl =
                 std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(astNode))
    {
      // Get inherited abstract methods
      for (auto &superInterface : interfaceDecl->getInterfaces())
      {
        if (!superInterface || !superInterface->getResolvedDecl() || !superInterface->getResolvedDecl()->getAstNode())
          continue;
        auto superInterfaceDecl =
            std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(superInterface);
        if(superInterface && !getInheritedMethods(superInterfaceDecl, abstractMethodMap, methodMap))
          return false;
      }
      // Resolve current interface's abstract methods
      for (auto &method : interfaceDecl->getMethods())
      {
        if(!method)
          continue;
        std::string signature = method->getSignature();
        // Some inherited functions have same signature but different return types
        if (abstractMethodMap.count(signature) && abstractMethodMap[signature]->getReturnType()->toString() != method->getReturnType()->toString()) 
          return false;
        if (methodMap.count(signature) && methodMap[signature]->getReturnType()->toString() != method->getReturnType()->toString())
          return false;
        // If same signature and return, don't insert; otherwise not present, so insert
        if (!abstractMethodMap.count(signature))
        {
          abstractMethodMap[signature] = method;
        }
      }
    }
    return true;
  }

  

  bool checkInheritence(std::shared_ptr<Decl> decl) {
    std::shared_ptr<parsetree::ast::Decl> astNode = decl->getAstNode();
    std::unordered_map<std::string, std::shared_ptr<parsetree::ast::MethodDecl>>
        abstractMethodMap;
    std::unordered_map<std::string, std::shared_ptr<parsetree::ast::MethodDecl>>
        methodMap;
    // Type conflict of same signature, different return type methods in superclasses
    if(!getInheritedMethods(astNode, abstractMethodMap, methodMap)) {
      return false;
    }

    // Check for abstract or unimplemented abstract methods
    if (auto classDecl =
            std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(astNode))
    {
      // If abstract, then don't continue checking
      if(classDecl->getModifiers()->isAbstract())
        return true;

      // Check for abstract methods
      for(auto &method : classDecl->getMethods()) {
        if(method->getModifiers()->isAbstract()) {
          std::cerr
              << "Error: Class " << classDecl->getName()
              << " contains abstract methods but is not declared abstract.\n";
          return false;
        }
      }

      for (auto &[signature, methodDecl] : abstractMethodMap)
      {
        if(!methodMap.count(signature)) {
          std::cerr
              << "Error: Class " << classDecl->getName()
              << " inherits an unimplemented abstract method but is not declared abstract.\n";
          return false;
        }
      }
    }
    return true;
  }

  bool traverseTree(std::shared_ptr<Package> node) {
    if (node == nullptr)
      return true;

    for (auto &[ch, child] : node->children) {
      if (std::holds_alternative<std::shared_ptr<static_check::Decl>>(child)) {
        // Perform checks
        std::shared_ptr<Decl> decl = std::get<std::shared_ptr<Decl>>(child);
        // Order matters!
        bool ret = checkProperExtends(decl);
        ret = ret && checkAcyclic(decl);
        ret = ret && checkDuplicateSignatures(decl);
        ret = ret && checkInheritence(decl);
        if (!ret)
          return false;
      } else if (std::holds_alternative<std::shared_ptr<Package>>(child)) {
        std::shared_ptr<Package> childPackage =
            std::get<std::shared_ptr<Package>>(child);
        if (!traverseTree(childPackage))
          return false;
      }
    }
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
