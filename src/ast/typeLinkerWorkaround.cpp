#include "ast/typeLinkerWorkaround.hpp"

namespace parsetree::ast::workaround {

std::unordered_set<std::shared_ptr<parsetree::ast::MethodDecl>>
ClassDeclWrapper::getAllMethods() {
  std::unordered_set<std::shared_ptr<MethodDecl>> methods;
  for (const auto &methodDecl : classDecl->getMethods()) {
    methods.insert(methodDecl);
  }

  for (const auto &interface : classDecl->getInterfaces()) {
    workaround->typeLinker->resolveType(interface);
    auto interfaceDecl = std::dynamic_pointer_cast<InterfaceDecl>(
        interface->getResolvedDeclAst());
    if (!interfaceDecl) {
      throw std::runtime_error("Interface Decl not resolved");
    }
    if (workaround->interfaceMethods.find(interfaceDecl) !=
        workaround->interfaceMethods.end()) {
      for (const auto &method : workaround->interfaceMethods[interfaceDecl]) {
        methods.insert(method);
      }
    } else {
      auto wrapper = InterfaceDeclWrapper(interfaceDecl, workaround);
      workaround->interfaceMethods[interfaceDecl] = wrapper.getAllMethods();
      for (const auto &method : workaround->interfaceMethods[interfaceDecl]) {
        methods.insert(method);
      }
    }
  }

  for (const auto &superClass : classDecl->getSuperClasses()) {
    workaround->typeLinker->resolveType(superClass);
    auto superClassDecl =
        std::dynamic_pointer_cast<ClassDecl>(superClass->getResolvedDeclAst());
    if (!superClassDecl) {
      throw std::runtime_error("Super Class Decl not resolved");
    }
    if (workaround->classMethods.find(superClassDecl) !=
        workaround->classMethods.end()) {
      for (const auto &method : workaround->classMethods[superClassDecl]) {
        methods.insert(method);
      }
    } else {
      auto wrapper = ClassDeclWrapper(superClassDecl, workaround);
      workaround->classMethods[superClassDecl] = wrapper.getAllMethods();
      for (const auto &method : workaround->classMethods[superClassDecl]) {
        methods.insert(method);
      }
    }
  }

  return methods;
}

std::unordered_set<std::shared_ptr<parsetree::ast::MethodDecl>>
InterfaceDeclWrapper::getAllMethods() {
  std::unordered_set<std::shared_ptr<MethodDecl>> methods;
  for (const auto &methodDecl : interfaceDecl->getMethods()) {
    methods.insert(methodDecl);
  }

  for (const auto &interface : interfaceDecl->getInterfaces()) {
    workaround->typeLinker->resolveType(interface);
    auto interfaceDecl2 = std::dynamic_pointer_cast<InterfaceDecl>(
        interface->getResolvedDeclAst());
    if (!interfaceDecl2) {
      throw std::runtime_error("Interface Decl not resolved");
    }
    if (workaround->interfaceMethods.find(interfaceDecl2) !=
        workaround->interfaceMethods.end()) {
      for (const auto &method : workaround->interfaceMethods[interfaceDecl2]) {
        methods.insert(method);
      }
    } else {
      auto wrapper = InterfaceDeclWrapper(interfaceDecl2, workaround);
      workaround->interfaceMethods[interfaceDecl2] = wrapper.getAllMethods();
      for (const auto &method : workaround->interfaceMethods[interfaceDecl2]) {
        methods.insert(method);
      }
    }
  }

  return methods;
}
} // namespace parsetree::ast::workaround
