#pragma once

#include "ast/ast.hpp"
#include "ast/astManager.hpp"
#include "environment.hpp"

namespace static_check {

struct InheritedMethodsResult {
  std::unordered_map<std::string, std::shared_ptr<parsetree::ast::MethodDecl>>
      abstractMethods;
  std::unordered_map<std::string, std::shared_ptr<parsetree::ast::MethodDecl>>
      methods;
  bool success;
};

class HierarchyCheck {
  std::shared_ptr<Package> rootPackage;

  bool isClass(std::shared_ptr<parsetree::ast::AstNode> decl) {
    return !!dynamic_pointer_cast<parsetree::ast::ClassDecl>(decl);
  }

  bool isInterface(std::shared_ptr<parsetree::ast::AstNode> decl) {
    return !!dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(decl);
  }

  // TODO: fix automatic java.lang.Object inheritance
  std::vector<std::shared_ptr<parsetree::ast::ClassDecl>>
  sanitizedSuperClasses(std::shared_ptr<parsetree::ast::ClassDecl> classDecl) {
    std::vector<std::shared_ptr<parsetree::ast::ClassDecl>> superClasses;
    for (auto &superClass : classDecl->getSuperClasses()) {
      if (superClass && superClass->getResolvedDecl() &&
          superClass->getResolvedDecl()->getAstNode()) {
        auto astNode = superClass->getResolvedDecl()->getAstNode();
        std::shared_ptr<parsetree::ast::ClassDecl> superClassDecl =
            dynamic_pointer_cast<parsetree::ast::ClassDecl>(astNode);
        if (astNode->getName() != "Object") {
          superClasses.push_back(superClassDecl);
        }
      }
    }
    return superClasses;
  }

  // Clear object of superclass of itself
  void resolveJavaLangObject() {
    auto javaPackageVariant = rootPackage->getChild("java");
    if (!std::holds_alternative<std::shared_ptr<Package>>(javaPackageVariant)) {
      std::cerr << "ERROR: Could not find java package\n";
      return;
    }
    auto javaPackage = std::get<std::shared_ptr<Package>>(javaPackageVariant);

    auto langPackageVariant = javaPackage->getChild("lang");
    if (!std::holds_alternative<std::shared_ptr<Package>>(langPackageVariant)) {
      std::cerr << "ERROR: Could not find java.lang package\n";
      return;
    }
    auto langPackage = std::get<std::shared_ptr<Package>>(langPackageVariant);

    auto objectDeclVariant = langPackage->getChild("Object");
    if (!std::holds_alternative<std::shared_ptr<Decl>>(objectDeclVariant)) {
      std::cerr << "ERROR: Could not find java.lang.Object\n";
      return;
    }

    auto objectDecl = std::get<std::shared_ptr<Decl>>(objectDeclVariant);
    if (!objectDecl) {
      std::cerr << "ERROR: Object decl is null\n";
      return;
    }

    auto astNode = objectDecl->getAstNode();
    if (!astNode) {
      std::cerr << "ERROR: Object decl has no AST node\n";
      return;
    }

    auto objectAstDecl =
        std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(astNode);
    if (!objectAstDecl) {
      std::cerr << "ERROR: Failed to cast Object decl to AST ClassDecl\n";
      return;
    }

    objectAstDecl->clearSuperClasses();
  }

  std::shared_ptr<parsetree::ast::ClassDecl>
  resolveJavaLangObjectInterfaces(std::shared_ptr<Package> rootPackage) {
    auto javaPackageVariant = rootPackage->getChild("java");
    if (!std::holds_alternative<std::shared_ptr<Package>>(javaPackageVariant)) {
      std::cerr << "ERROR: Could not find java package\n";
      return nullptr;
    }
    auto javaPackage = std::get<std::shared_ptr<Package>>(javaPackageVariant);

    auto langPackageVariant = javaPackage->getChild("lang");
    if (!std::holds_alternative<std::shared_ptr<Package>>(langPackageVariant)) {
      std::cerr << "ERROR: Could not find java.lang package\n";
      return nullptr;
    }
    auto langPackage = std::get<std::shared_ptr<Package>>(langPackageVariant);

    auto objectDeclVariant = langPackage->getChild("Object");
    if (!std::holds_alternative<std::shared_ptr<Decl>>(objectDeclVariant)) {
      std::cerr << "ERROR: Could not find java.lang.Object\n";
      return nullptr;
    }

    auto objectDecl = std::get<std::shared_ptr<Decl>>(objectDeclVariant);
    if (!objectDecl) {
      std::cerr << "ERROR: Object decl is null\n";
      return nullptr;
    }

    auto astNode = objectDecl->getAstNode();
    if (!astNode) {
      std::cerr << "ERROR: Object decl has no AST node\n";
      return nullptr;
    }

    auto objectAstDecl =
        std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(astNode);
    if (!objectAstDecl) {
      std::cerr << "ERROR: Failed to cast Object decl to AST ClassDecl\n";
      return nullptr;
    }

    return objectAstDecl;
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

  std::string
  getSafeReturnType(std::shared_ptr<parsetree::ast::MethodDecl> method) {
    if (!method->getReturnType())
      return "Void";
    return method->getReturnType()->toString();
  }

  bool getInheritedMethods(
      std::shared_ptr<parsetree::ast::Decl> astNode,
      std::unordered_map<std::string,
                         std::shared_ptr<parsetree::ast::MethodDecl>>
          &abstractMethodMap,
      std::unordered_map<
          std::string, std::shared_ptr<parsetree::ast::MethodDecl>> &methodMap,
      std::unordered_set<std::string> &implements) {
    if (auto classDecl =
            std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(astNode)) {
      for (auto &superInterface : classDecl->getInterfaces()) {
        if (!superInterface || !superInterface->getResolvedDecl() ||
            !superInterface->getResolvedDecl()->getAstNode())
          continue;
        auto superInterfaceDecl =
            std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                superInterface->getResolvedDecl()->getAstNode());
        if (superInterfaceDecl &&
            !getInheritedMethods(superInterfaceDecl, abstractMethodMap,
                                 methodMap, implements))
          return false;
      }
      for (auto &superClass : sanitizedSuperClasses(classDecl)) {
        if (!getInheritedMethods(superClass, abstractMethodMap, methodMap,
                                 implements))
          return false;
      }
      // Resolve current classes's methods
      for (auto &method : classDecl->getMethods()) {
        if (!method || method->isConstructor())
          continue;
        std::string signature = method->getSignature();
        // Some inherited functions have same signature but different return
        // types
        if (abstractMethodMap.count(signature) &&
            getSafeReturnType(abstractMethodMap[signature]) !=
                getSafeReturnType(method))
          return false;
        if (methodMap.count(signature) &&
            getSafeReturnType(methodMap[signature]) !=
                getSafeReturnType(method))
          return false;
        // Handle abstract method;
        if (method->getModifiers() && method->getModifiers()->isAbstract()) {
          // Add to map if not exists yet
          if (!abstractMethodMap.count(signature))
            abstractMethodMap[signature] = method;
          // New abstract declaration means that it needs to be implemented by
          // child node
          implements.erase(signature);
        } else if (!method->isConstructor()) {
          // Add to map if not exists yet
          if (!methodMap.count(signature))
            methodMap[signature] = method;
          // If abstract signature exists, this method implements it
          if (abstractMethodMap.count(signature)) {
            implements.insert(signature);
          }
        }
      }
    } else if (auto interfaceDecl =
                   std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                       astNode)) {
      // Get inherited abstract methods
      for (auto &superInterface : interfaceDecl->getInterfaces()) {
        if (!superInterface || !superInterface->getResolvedDecl() ||
            !superInterface->getResolvedDecl()->getAstNode())
          continue;
        auto superInterfaceDecl =
            std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                superInterface->getResolvedDecl()->getAstNode());
        if (superInterfaceDecl &&
            !getInheritedMethods(superInterfaceDecl, abstractMethodMap,
                                 methodMap, implements))
          return false;
      }
      // Resolve current interface's abstract methods
      for (auto &method : interfaceDecl->getMethods()) {
        if (!method)
          continue;
        std::string signature = method->getSignature();
        // Some inherited functions have same signature but different return
        // types
        if (abstractMethodMap.count(signature) &&
            getSafeReturnType(abstractMethodMap[signature]) !=
                getSafeReturnType(method))
          return false;
        if (methodMap.count(signature) &&
            getSafeReturnType(methodMap[signature]) !=
                getSafeReturnType(method))
          return false;
        // If same signature and return, don't insert; otherwise not present, so
        // insert
        if (!abstractMethodMap.count(signature)) {
          abstractMethodMap[signature] = method;
        }
        // New abstract declaration means that it needs to be implemented by
        // child node
        implements.erase(signature);
      }
    }
    return true;
  }

  void getInheritedFields(
      std::shared_ptr<parsetree::ast::Decl> astNode,
      std::unordered_map<std::string,
                         std::shared_ptr<parsetree::ast::FieldDecl>> &fieldMap,
      bool isCurrentClass) {
    auto classDecl =
        std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(astNode);
    // An interface cannot have fields
    if (!classDecl)
      return;
    for (auto &superClass : sanitizedSuperClasses(classDecl)) {
      getInheritedFields(superClass, fieldMap, false);
    }
    if (isCurrentClass) {
      return;
    }
    for (auto &field : classDecl->getFields()) {
      if (!field)
        continue;
      if (!fieldMap.contains(field->getName()))
        fieldMap[field->getName()] = field;
    }
  }

  bool checkInheritence(std::shared_ptr<Decl> decl) {
    std::shared_ptr<parsetree::ast::Decl> astNode = decl->getAstNode();
    std::unordered_map<std::string, std::shared_ptr<parsetree::ast::MethodDecl>>
        abstractMethodMap;
    std::unordered_map<std::string, std::shared_ptr<parsetree::ast::MethodDecl>>
        methodMap;
    std::unordered_set<std::string> implements;
    // Type conflict of same signature, different return type methods in
    // superclasses
    if (!getInheritedMethods(astNode, abstractMethodMap, methodMap,
                             implements)) {
      return false;
    }

    // Check for abstract or unimplemented abstract methods
    if (auto classDecl =
            std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(astNode)) {
      // If abstract, then don't continue checking
      if (classDecl->getModifiers()->isAbstract())
        return true;

      // Check for abstract methods
      for (auto &method : classDecl->getMethods()) {
        if (method->getModifiers()->isAbstract()) {
          std::cerr
              << "Error: Class " << classDecl->getName()
              << " contains abstract methods but is not declared abstract.\n";
          return false;
        }
      }
      for (auto &[signature, methodDecl] : abstractMethodMap) {
        if (!implements.count(signature)) {
          std::cerr << "Error: Class " << classDecl->getName()
                    << " inherits an unimplemented abstract method but is not "
                       "declared abstract.\n";
          return false;
        }
      }
    }
    return true;
  }

  bool checkStaticMethodOverride(std::shared_ptr<Decl> decl) {
    std::shared_ptr<parsetree::ast::Decl> astNode = decl->getAstNode();

    if (auto classDecl =
            std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(astNode)) {
      for (auto &method : classDecl->getMethods()) {
        std::string signature = method->getSignature();
        bool isMethodStatic =
            method->getModifiers() && method->getModifiers()->isStatic();

        for (auto &superClass : classDecl->getSuperClasses()) {
          if (!superClass)
            continue;
          if (auto superDecl = superClass->getResolvedDecl()->getAstNode()) {
            auto superClassDecl =
                std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(superDecl);
            if (!superClassDecl)
              continue;

            // std::cout << superClassDecl->getMethods().size() << std::endl;

            for (auto &superMethod : superClassDecl->getMethods()) {
              // std::cout << superMethod->getSignature() << " " << signature
              //           << std::endl;
              if (superMethod->getSignature() == signature) {
                bool isSuperMethodStatic =
                    superMethod->getModifiers() &&
                    superMethod->getModifiers()->isStatic();
                if (isSuperMethodStatic && !isMethodStatic) {
                  std::cerr << "Error: Non-static method " << method->getName()
                            << " in class " << classDecl->getName()
                            << " cannot override static method from superclass "
                            << superClassDecl->getName() << "\n";
                  return false;
                }
              }
            }
          }
        }
      }
    } else if (auto interfaceDecl =
                   std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                       astNode)) {
      for (auto &method : interfaceDecl->getMethods()) {
        std::string signature = method->getSignature();
        bool isMethodStatic =
            method->getModifiers() && method->getModifiers()->isStatic();

        for (auto &superInterface : interfaceDecl->getInterfaces()) {
          if (!superInterface)
            continue;
          if (auto superDecl =
                  superInterface->getResolvedDecl()->getAstNode()) {
            auto superInterfaceDecl =
                std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                    superDecl);
            if (!superInterfaceDecl)
              continue;

            for (auto &superMethod : superInterfaceDecl->getMethods()) {
              if (superMethod->getSignature() == signature) {
                bool isSuperMethodStatic =
                    superMethod->getModifiers() &&
                    superMethod->getModifiers()->isStatic();
                if (isSuperMethodStatic && !isMethodStatic) {
                  std::cerr
                      << "Error: Non-static method " << method->getName()
                      << " in interface " << interfaceDecl->getName()
                      << " cannot override static method from superinterface "
                      << superInterfaceDecl->getName() << "\n";
                  return false;
                }
              }
            }
          }
        }
      }
    }
    return true;
  }

  bool checkNonStaticMethodOverride(std::shared_ptr<Decl> decl) {
    std::shared_ptr<parsetree::ast::Decl> astNode = decl->getAstNode();
    if (!astNode)
      return true;

    if (auto classDecl =
            std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(astNode)) {
      for (auto &method : classDecl->getMethods()) {
        std::string signature = method->getSignature();
        bool isMethodStatic =
            method->getModifiers() && method->getModifiers()->isStatic();

        for (auto &superClass : classDecl->getSuperClasses()) {
          if (!superClass)
            continue;
          if (auto superDecl = superClass->getResolvedDecl()->getAstNode()) {
            auto superClassDecl =
                std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(superDecl);
            if (!superClassDecl)
              continue;

            for (auto &superMethod : superClassDecl->getMethods()) {
              if (superMethod->getSignature() == signature) {
                bool isSuperMethodStatic =
                    superMethod->getModifiers() &&
                    superMethod->getModifiers()->isStatic();
                if (!isSuperMethodStatic && isMethodStatic) {
                  std::cerr
                      << "Error: Static method " << method->getName()
                      << " in class " << classDecl->getName()
                      << " cannot override non-static method from superclass "
                      << superClassDecl->getName() << "\n";
                  return false;
                }
              }
            }
          }
        }
      }
    } else if (auto interfaceDecl =
                   std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                       astNode)) {
      for (auto &method : interfaceDecl->getMethods()) {
        std::string signature = method->getSignature();
        bool isMethodStatic =
            method->getModifiers() && method->getModifiers()->isStatic();

        for (auto &superInterface : interfaceDecl->getInterfaces()) {
          if (!superInterface)
            continue;
          if (auto superDecl =
                  superInterface->getResolvedDecl()->getAstNode()) {
            auto superInterfaceDecl =
                std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                    superDecl);
            if (!superInterfaceDecl)
              continue;

            for (auto &superMethod : superInterfaceDecl->getMethods()) {
              if (superMethod->getSignature() == signature) {
                bool isSuperMethodStatic =
                    superMethod->getModifiers() &&
                    superMethod->getModifiers()->isStatic();
                if (!isSuperMethodStatic && isMethodStatic) {
                  std::cerr << "Error: Static method " << method->getName()
                            << " in interface " << interfaceDecl->getName()
                            << " cannot override non-static method from "
                               "superinterface "
                            << superInterfaceDecl->getName() << "\n";
                  return false;
                }
              }
            }
          }
        }
      }
    }
    return true;
  }

  bool checkMethodReturnTypeOverride(std::shared_ptr<Decl> decl,
                                     std::shared_ptr<Package> rootPackage) {
    std::shared_ptr<parsetree::ast::Decl> astNode = decl->getAstNode();
    if (!astNode)
      return true;

    if (auto classDecl =
            std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(astNode)) {
      for (auto &method : classDecl->getMethods()) {
        std::string signature = method->getSignature();
        std::string returnType =
            method->getReturnType() ? method->getReturnType()->toString() : "";

        // std::cout << "DEBUG: Checking method: " << signature << " in class "
        //           << classDecl->getName() << "\n";

        for (auto &superClass : classDecl->getSuperClasses()) {
          if (!superClass)
            continue;
          if (auto superDecl = superClass->getResolvedDecl()->getAstNode()) {
            auto superClassDecl =
                std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(superDecl);
            if (!superClassDecl)
              continue;

            for (auto &superMethod : superClassDecl->getMethods()) {
              if (!superMethod)
                continue;
              if (superMethod->getSignature() == signature) {
                std::string superReturnType =
                    superMethod->getReturnType()
                        ? superMethod->getReturnType()->toString()
                        : "";
                if (superReturnType != returnType) {
                  std::cerr << "Error: Method " << method->getName()
                            << " in class " << classDecl->getName()
                            << " overrides a method with a different return "
                               "type from superclass "
                            << superClassDecl->getName() << "\n";
                  return false;
                }
              }
            }
          }
        }
      }
    } else if (auto interfaceDecl =
                   std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                       astNode)) {

      std::vector<std::shared_ptr<parsetree::ast::ReferenceType>>
          superInterfaces = interfaceDecl->getInterfaces();

      for (auto &method : interfaceDecl->getMethods()) {
        std::string signature = method->getSignature();
        // std::cout << "DEBUG: Checking method: " << signature << " in
        // interface "
        //           << interfaceDecl->getName() << "\n";

        auto objectDecl = resolveJavaLangObjectInterfaces(rootPackage);
        if (objectDecl) {
          for (auto &objectMethod : objectDecl->getMethods()) {
            if (!objectMethod)
              continue;
            if (objectMethod->getSignature() == signature) {
              bool isReturnTypeDiff =
                  getSafeReturnType(objectMethod) != getSafeReturnType(method);
              if (isReturnTypeDiff) {
                std::cerr
                    << "Error: Method " << method->getName() << " in interface "
                    << interfaceDecl->getName()
                    << " cannot override final method from java.lang.Object\n";
                return false;
              }
            }
          }
        }

        for (auto &superInterface : superInterfaces) {
          if (!superInterface || !superInterface->getResolvedDecl()) {
            // std::cout << "DEBUG: Skipping unresolved superinterface\n";
            continue;
          }

          if (auto superDecl =
                  superInterface->getResolvedDecl()->getAstNode()) {
            auto superInterfaceDecl =
                std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                    superDecl);
            if (!superInterfaceDecl) {
              // std::cout << "DEBUG: Skipping superinterface that is not an "
              //              "interface\n";
              continue;
            }

            for (auto &superMethod : superInterfaceDecl->getMethods()) {
              if (!superMethod)
                continue;
              // std::cout << "DEBUG: Comparing against superinterface method: "
              //           << superMethod->getSignature() << "\n";

              if (superMethod->getSignature() == signature) {
                bool isSuperMethodFinal =
                    superMethod->getModifiers() &&
                    superMethod->getModifiers()->isFinal();
                if (isSuperMethodFinal) {
                  std::cerr
                      << "Error: Method " << method->getName()
                      << " in interface " << interfaceDecl->getName()
                      << " cannot override final method from superinterface "
                      << superInterfaceDecl->getName() << "\n";
                  return false;
                }
              }
            }
          }
        }
      }
    }
    return true;
  }

  bool checkProtectedMethodOverride(std::shared_ptr<Decl> decl,
                                    std::shared_ptr<Package> rootPackage) {
    std::shared_ptr<parsetree::ast::Decl> astNode = decl->getAstNode();
    if (!astNode)
      return true;

    if (auto classDecl =
            std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(astNode)) {

      auto inheritedMethods = getAllInheritedMethods(classDecl);

      std::unordered_map<std::string,
                         std::shared_ptr<parsetree::ast::MethodDecl>>
          allMethods;
      for (auto &method : classDecl->getMethods()) {
        allMethods[method->getSignature()] = method;
      }
      for (auto &[signature, method] : inheritedMethods.abstractMethods) {
        if (!allMethods.count(signature))
          allMethods[signature] = method;
      }
      for (auto &[signature, method] : inheritedMethods.methods) {
        if (!allMethods.count(signature))
          allMethods[signature] = method;
      }

      for (auto &[signature, method] : allMethods) {
        bool isMethodProtected =
            method->getModifiers() && method->getModifiers()->isProtected();
        bool isMethodPublic =
            method->getModifiers() && method->getModifiers()->isPublic();

        bool isInheritedProtected = false;

        for (auto &superClass : classDecl->getSuperClasses()) {
          if (!superClass)
            continue;
          if (auto superDecl = superClass->getResolvedDecl()->getAstNode()) {
            auto superClassDecl =
                std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(superDecl);
            if (!superClassDecl)
              continue;

            for (auto &superMethod : superClassDecl->getMethods()) {
              if (superMethod->getSignature() == signature) {
                bool isSuperMethodPublic =
                    superMethod->getModifiers() &&
                    superMethod->getModifiers()->isPublic();

                if (superMethod->getModifiers() &&
                    superMethod->getModifiers()->isProtected()) {
                  isInheritedProtected = true;
                }

                if (isSuperMethodPublic && isMethodProtected) {
                  std::cerr << "Error: Protected method " << method->getName()
                            << " in class " << classDecl->getName()
                            << " cannot override public method from superclass "
                            << superClassDecl->getName() << "\n";
                  return false;
                }
              }
            }
          }
        }

        for (auto &superInterface : classDecl->getInterfaces()) {
          if (!superInterface)
            continue;
          if (auto superDecl =
                  superInterface->getResolvedDecl()->getAstNode()) {
            auto superInterfaceDecl =
                std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                    superDecl);
            if (!superInterfaceDecl)
              continue;

            for (auto &superMethod : superInterfaceDecl->getMethods()) {
              if (superMethod->getSignature() == signature) {
                bool isSuperMethodPublic =
                    superMethod->getModifiers() &&
                    superMethod->getModifiers()->isPublic();

                if (isSuperMethodPublic && isInheritedProtected) {
                  std::cerr << "Error: Protected method " << method->getName()
                            << " in class " << classDecl->getName()
                            << " cannot override public method from interface "
                            << superInterfaceDecl->getName() << "\n";
                  return false;
                }

                if (isSuperMethodPublic && isMethodProtected) {
                  std::cerr << "Error: Protected method " << method->getName()
                            << " in class " << classDecl->getName()
                            << " cannot override public method from interface "
                            << superInterfaceDecl->getName() << "\n";
                  return false;
                }
              }
            }
          }
        }

        auto objectDecl = resolveJavaLangObjectInterfaces(rootPackage);
        if (objectDecl) {
          for (auto &objectMethod : objectDecl->getMethods()) {
            if (!objectMethod)
              continue;
            if (objectMethod->getSignature() == signature) {
              bool isObjectMethodProtected =
                  objectMethod->getModifiers() &&
                  objectMethod->getModifiers()->isProtected();

              for (auto &superInterface : classDecl->getInterfaces()) {
                if (!superInterface)
                  continue;
                if (auto superDecl =
                        superInterface->getResolvedDecl()->getAstNode()) {
                  auto superInterfaceDecl =
                      std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                          superDecl);
                  if (!superInterfaceDecl)
                    continue;
                  for (auto &superMethod : superInterfaceDecl->getMethods()) {
                    if (superMethod->getSignature() == signature &&
                        superMethod->getModifiers()->isPublic()) {
                      std::cerr
                          << "Error: Protected method " << method->getName()
                          << " in class " << classDecl->getName()
                          << " cannot override public method from interface "
                          << superInterfaceDecl->getName() << "\n";
                      return false;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    return true;
  }

  bool checkFinalMethodOverride(std::shared_ptr<Decl> decl,
                                std::shared_ptr<Package> rootPackage) {
    std::shared_ptr<parsetree::ast::Decl> astNode = decl->getAstNode();
    if (!astNode)
      return true;

    if (auto classDecl =
            std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(astNode)) {
      std::vector<std::shared_ptr<parsetree::ast::ReferenceType>> superClasses =
          classDecl->getSuperClasses();

      for (auto &method : classDecl->getMethods()) {
        std::string signature = method->getSignature();
        // std::cout << "DEBUG: Checking method: " << signature << " in class "
        //           << classDecl->getName() << "\n";

        for (auto &superClass : superClasses) {
          if (!superClass || !superClass->getResolvedDecl())
            continue;
          if (auto superDecl = superClass->getResolvedDecl()->getAstNode()) {
            auto superClassDecl =
                std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(superDecl);
            if (!superClassDecl)
              continue;

            for (auto &superMethod : superClassDecl->getMethods()) {
              if (!superMethod)
                continue;
              // std::cout << "DEBUG: Comparing against superclass method: "
              //           << superMethod->getSignature() << "\n";

              if (superMethod->getSignature() == signature) {

                bool isSuperMethodFinal =
                    superMethod->getModifiers() &&
                    superMethod->getModifiers()->isFinal();
                if (isSuperMethodFinal) {
                  std::cerr << "Error: Method " << method->getName()
                            << " in class " << classDecl->getName()
                            << " cannot override final method from superclass "
                            << superClassDecl->getName() << "\n";
                  return false;
                }
              }
            }
          }
        }
      }
    } else if (auto interfaceDecl =
                   std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                       astNode)) {
      std::vector<std::shared_ptr<parsetree::ast::ReferenceType>>
          superInterfaces = interfaceDecl->getInterfaces();

      for (auto &method : interfaceDecl->getMethods()) {
        std::string signature = method->getSignature();
        // std::cout << "DEBUG: Checking method: " << signature << " in
        // interface "
        //           << interfaceDecl->getName() << "\n";

        auto objectDecl = resolveJavaLangObjectInterfaces(rootPackage);
        if (objectDecl) {
          for (auto &objectMethod : objectDecl->getMethods()) {
            if (!objectMethod)
              continue;
            if (objectMethod->getSignature() == signature) {
              bool isObjectMethodFinal =
                  objectMethod->getModifiers() &&
                  objectMethod->getModifiers()->isFinal();
              if (isObjectMethodFinal) {
                std::cerr
                    << "Error: Method " << method->getName() << " in interface "
                    << interfaceDecl->getName()
                    << " cannot override final method from java.lang.Object\n";
                return false;
              }
            }
          }
        }

        for (auto &superInterface : superInterfaces) {
          if (!superInterface || !superInterface->getResolvedDecl()) {
            // std::cout << "DEBUG: Skipping unresolved superinterface\n";
            continue;
          }

          if (auto superDecl =
                  superInterface->getResolvedDecl()->getAstNode()) {
            auto superInterfaceDecl =
                std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                    superDecl);
            if (!superInterfaceDecl) {
              // std::cout << "DEBUG: Skipping superinterface that is not an "
              //              "interface\n";
              continue;
            }

            for (auto &superMethod : superInterfaceDecl->getMethods()) {
              if (!superMethod)
                continue;
              // std::cout << "DEBUG: Comparing against superinterface method: "
              //           << superMethod->getSignature() << "\n";

              if (superMethod->getSignature() == signature) {
                bool isSuperMethodFinal =
                    superMethod->getModifiers() &&
                    superMethod->getModifiers()->isFinal();
                if (isSuperMethodFinal) {
                  std::cerr
                      << "Error: Method " << method->getName()
                      << " in interface " << interfaceDecl->getName()
                      << " cannot override final method from superinterface "
                      << superInterfaceDecl->getName() << "\n";
                  return false;
                }
              }
            }
          }
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
        // if (ret)
        //   std::cout << "Passed extends\n";
        ret = ret && checkAcyclic(decl);
        // if (ret)
        //   std::cout << "Passed acyclic\n";
        ret = ret && checkDuplicateSignatures(decl);
        // if (ret)
        //   std::cout << "Passed duplicate\n";
        ret = ret && checkInheritence(decl);
        // if (ret)
        //   std::cout << "Passed inheritence\n";
        ret = ret && checkStaticMethodOverride(decl);
        // if (ret)
        //   std::cout << "Passed static override\n";
        ret = ret && checkNonStaticMethodOverride(decl);
        // if (ret)
        //   std::cout << "Passed nonstatic override\n";
        ret = ret && checkMethodReturnTypeOverride(decl, rootPackage);
        // if (ret)
        //   std::cout << "Passed return type override\n";
        ret = ret && checkProtectedMethodOverride(decl, rootPackage);
        // if (ret)
        //   std::cout << "Passed protected override\n";
        ret = ret && checkFinalMethodOverride(decl, rootPackage);
        // if (ret)
        //   std::cout << "Passed final override\n";
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
    resolveJavaLangObject();
    // Traverse tree and validate each node
    return traverseTree(rootPackage);
  }

  // Wrapper
  std::unordered_map<std::string, std::shared_ptr<parsetree::ast::FieldDecl>>
  getInheritedFields(std::shared_ptr<parsetree::ast::Decl> astNode) {
    std::unordered_map<std::string, std::shared_ptr<parsetree::ast::FieldDecl>>
        fields;
    getInheritedFields(astNode, fields, true);
    return fields;
  }

  InheritedMethodsResult
  getAllInheritedMethods(std::shared_ptr<parsetree::ast::Decl> astNode) {
    InheritedMethodsResult result;

    if (!astNode) {
      result.success = false;
      return result;
    }

    std::unordered_set<std::string> implements;
    bool success = getInheritedMethods(astNode, result.abstractMethods,
                                       result.methods, implements);
    result.success = success;
    return result;
  }
};

} // namespace static_check
