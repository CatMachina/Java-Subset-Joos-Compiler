#pragma once

#include "ast/ast.hpp"

namespace static_check {

class ForwardChecker {
public:
  void checkField(const std::shared_ptr<parsetree::ast::FieldDecl> node) {
    if (!node->hasInit())
      return;
    auto exprNodes = node->getInitializer()->getExprNodes();
    // std::cout << "for field " << node->getName()
    //           << " we have expr nodes: " << std::endl;
    // node->getInitializer()->print(std::cout);

    for (auto exprNode : exprNodes) {
      if (std::dynamic_pointer_cast<parsetree::ast::MethodName>(exprNode)) {
        continue;
      }
      if (auto member =
              std::dynamic_pointer_cast<parsetree::ast::MemberName>(exprNode)) {
        if (!member->isDeclResolved())
          throw std::runtime_error("member name not decl resolved for " +
                                   member->getName());

        if (member->isAccessedByThis()) {
          continue;
        }
        if (member->isinitializedInExpr()) {
          continue;
        }
        if (member->isNotAsBase()) {
          continue;
        }
        if (auto fieldDecl =
                std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(
                    member->getResolvedDecl())) {
          if (node->getModifiers()->isStatic() &&
              fieldDecl->getModifiers()->isStatic()) {
            continue;
          }
        }

        bool isInitializedLater =
            node->getLoc() <= member->getResolvedDecl()->getLoc();

        // std::cout << "isInitializedLater: " << isInitializedLater <<
        // std::endl; std::cout << "current loc: " << node->getLoc() <<
        // std::endl; std::cout << "decl loc: " <<
        // member->getResolvedDecl()->getLoc()
        //           << std::endl;

        if (isInitializedLater) {
          throw std::runtime_error("forward reference error of " +
                                   member->getName() + " in " +
                                   node->getName());
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