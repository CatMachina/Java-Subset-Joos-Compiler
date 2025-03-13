#pragma once

#include "ast/ast.hpp"

namespace static_check {

class ForwardChecker {
public:
  void checkField(const std::shared_ptr<parsetree::ast::FieldDecl> node) {
    if (!node->hasInit())
      return;
    auto exprNodes = node->getInitializer()->getExprNodes();
    std::cout << "for field " << node->getName()
              << " we have expr nodes: " << std::endl;
    node->getInitializer()->print(std::cout);

    std::shared_ptr<parsetree::ast::MemberName> base = nullptr;
    bool hasAssignment = false;
    for (auto expr : exprNodes) {
      if (std::dynamic_pointer_cast<parsetree::ast::Assignment>(expr)) {
        hasAssignment = true;
        break;
      }
    }
    if (!hasAssignment) {
      for (auto expr : exprNodes) {
        if (std::dynamic_pointer_cast<parsetree::ast::MethodName>(expr)) {
          ;
        } else if (auto member =
                       std::dynamic_pointer_cast<parsetree::ast::MemberName>(
                           expr)) {
          if (!member->isDeclResolved())
            throw std::runtime_error("member name not decl resolved for " +
                                     member->getName());

          auto memberDecl = member->getResolvedDecl();
          auto currentLoc = node->getLoc();
          auto declLoc = memberDecl->getLoc();
          auto fieldDecl =
              std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(memberDecl);
          if (fieldDecl && fieldDecl->getModifiers()->isStatic())
            continue;
          std::cout << "for field " << member->getName()
                    << " we have current location:";
          std::cout << currentLoc << std::endl;
          std::cout << " and decl location at " << declLoc << std::endl;
          if (!base && currentLoc < declLoc)
            throw std::runtime_error("forward reference of field " +
                                     member->getName());

          base = member;
        }
      }
    }
  }

  void checkAST(const std::shared_ptr<parsetree::ast::AstNode> node) {
    if (!node)
      throw std::runtime_error("Node is null when resolving AST");
    if (auto field =
            std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(node)) {
      checkField(field);
    }
    for (const auto &child : node->getChildren()) {
      if (!child)
        continue;
      checkAST(child);
    }
  }

  void check(std::shared_ptr<parsetree::ast::ASTManager> astManager) {
    for (auto ast : astManager->getASTs()) {
      checkAST(ast);
    }
  }
};

} // namespace static_check