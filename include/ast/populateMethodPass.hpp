#pragma once
#include "ast/astManager.hpp"
#include "staticCheck/typeLinker.hpp"

namespace parsetree::ast {

class PopulateMethodPass {
  std::shared_ptr<ASTManager> astManager;
  std::shared_ptr<static_check::TypeLinker> typeLinker;

public:
  PopulateMethodPass(std::shared_ptr<ASTManager> astManager,
                     std::shared_ptr<static_check::TypeLinker> typeLinker)
      : astManager(astManager), typeLinker(typeLinker) {}

  void populate() {
    for (auto classOrInterfaceDecl : astManager->allDecls) {
      // Class
      if (auto classDecl =
              std::dynamic_pointer_cast<ClassDecl>(classOrInterfaceDecl)) {
        // std::cout << "populating methods for " << classDecl->getFullName()
        //           << std::endl;

        for (auto &method : classDecl->getMethods()) {
          classDecl->getAllMethods().insert(method);
        }

        std::vector<static_check::Decl> copy;
        for (auto superClass : classDecl->getSuperClasses()) {
          if (!superClass)
            continue;
          auto temp = superClass->getResolvedDecl();
          if (!temp.getAstNode()) {
            temp = *(typeLinker->resolveTypeAgain(superClass));
          }
          // auto temp = typeLinker->resolveTypeAgain(superClass);
          if (!temp.getAstNode()) {
            superClass->print(std::cout);
            throw std::runtime_error("super class not found for " +
                                     classDecl->getFullName());
          }
          copy.push_back(temp);
        }
        // for (auto interface : classDecl->getInterfaces()) {
        //     if (!interface) continue;
        //     auto temp = typeLinker->resolveTypeAgain(interface);
        //     if (!temp)
        //         throw std::runtime_error("interface not found");
        //     copy.push_back(temp);
        // }

        // classDecl->getAllMethods();
        for (auto decl : copy) {
          auto declAst = decl.getAstNode();
          if (!declAst)
            throw std::runtime_error("Decl no AST Node");
          if (auto classDecl2 = std::dynamic_pointer_cast<ClassDecl>(declAst)) {
            // std::cout << "adding methods from super class "
            //           << classDecl2->getFullName() << " to "
            //           << classDecl->getFullName() << std::endl;
            auto superMethods = classDecl2->getMethods();
            classDecl->getAllMethods().insert(superMethods.begin(),
                                              superMethods.end());
          }
          // else if (auto interfaceDecl =
          // std::dynamic_pointer_cast<InterfaceDecl>(declAst)) {
          //     std::cout << "adding methods from interface " <<
          //     interfaceDecl->getFullName() << " to " <<
          //     classDecl->getFullName() << std::endl; auto interfaceMethods =
          //     interfaceDecl->getMethods();
          //     classDecl->getAllMethods().insert(interfaceMethods.begin(),
          //     interfaceMethods.end());

          // }
        }

        if (classDecl->getAllMethods().size() == 0) {
          throw std::runtime_error("Class " + classDecl->getFullName() +
                                   " has no methods");
        } else {
        //   std::cout << "Class " << classDecl->getFullName() << " has "
        //             << classDecl->getAllMethods().size() << " methods"
        //             << std::endl;
        }
      }

      //     // Interface
      //     else if (auto interfaceDecl =
      //     std::dynamic_pointer_cast<InterfaceDecl>(classOrInterfaceDecl)) {

      //         if (!interfaceDecl) continue;
      //     std::vector<std::shared_ptr<static_check::Decl>> copy;
      //     for (auto interface : interfaceDecl->getInterfaces()) {
      //         auto temp = typeLinker->resolveTypeAgain(interface);
      //         if (!temp)
      //             throw std::runtime_error("interface not found");
      //         copy.push_back(temp);
      //     }
      //     // interfaceDecl->getAllMethods();

      //     for (auto decl : copy) {
      //         auto declAst = decl->getAstNode();
      //         if (!declAst)
      //             throw std::runtime_error("Decl no AST Node");
      //         if (auto interfaceDecl2 =
      //         std::dynamic_pointer_cast<InterfaceDecl>(declAst)) {
      //             std::cout << "adding methods from interface " <<
      //             interfaceDecl2->getFullName() << " to " <<
      //             interfaceDecl->getFullName() << std::endl; auto
      //             interfaceMethods = interfaceDecl2->getMethods();
      //             interfaceDecl->getAllMethods().insert(interfaceMethods.begin(),
      //             interfaceMethods.end());

      //         }
      //     }

      //     if (interfaceDecl->getAllMethods().empty()) {
      //         throw std::runtime_error("Interface " +
      //         interfaceDecl->getFullName() + " has no methods");
      //     }

      //     }
    }
  }
};

} // namespace parsetree::ast
