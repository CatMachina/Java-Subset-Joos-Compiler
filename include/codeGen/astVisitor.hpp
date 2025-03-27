#pragma once

#include "ast/ast.hpp"
#include "ast/astManager.hpp"
#include "codeGen/exprIRConverter.hpp"
#include "tir/TIR.hpp"

// For testing Expr IR tree building only.

namespace codegen {

class ASTVisitor {
private:
  std::shared_ptr<ExprIRConverter> exprConverter = nullptr;
  std::shared_ptr<parsetree::ast::ASTManager> astManager = nullptr;
  std::shared_ptr<CodeGenLabels> codeGenLabels = nullptr;

public:
  ASTVisitor(std::shared_ptr<parsetree::ast::ASTManager> astManager,
             std::shared_ptr<CodeGenLabels> codeGenLabels) {
    this->astManager = astManager;
    this->codeGenLabels = codeGenLabels;
    this->exprConverter =
        std::make_shared<ExprIRConverter>(astManager, codeGenLabels);
  }

  void visitAST(const std::shared_ptr<parsetree::ast::AstNode> node) {
    if (!node)
      throw std::runtime_error("Node is null when resolving AST");
    if (auto classDecl =
            std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(node)) {
      exprConverter->setCurrentClass(classDecl);
    } else if (auto expr =
                   std::dynamic_pointer_cast<parsetree::ast::Expr>(node)) {
      std::shared_ptr<tir::Expr> tirExpr =
          exprConverter->evaluateList(expr->getExprNodes());
      tirExpr->print(std::cout);
    }
    for (const auto &child : node->getChildren()) {
      if (!child)
        continue;
      visitAST(child);
    }
  }

  void visit() {
    for (auto ast : astManager->getASTs()) {
      visitAST(ast);
    }
  };
};

} // namespace codegen
