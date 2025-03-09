#pragma once

#include "cfg.hpp"

namespace static_check {

class CFGBuilder {
public:
  std::shared_ptr<CFG>
  buildCFG(std::shared_ptr<parsetree::ast::Block> methodBody);

private:
  std::shared_ptr<CFG> cfg;
  std::shared_ptr<parsetree::ast::MethodDecl> method;

  int getNextId();

  std::shared_ptr<CFGNode>
  visitIfStmt(std::shared_ptr<parsetree::ast::IfStmt> stmt,
              std::shared_ptr<CFGNode> successor = nullptr);

  std::shared_ptr<CFGNode>
  visitWhileStmt(std::shared_ptr<parsetree::ast::WhileStmt> stmt,
                 std::shared_ptr<CFGNode> successor = nullptr);

  std::shared_ptr<CFGNode>
  visitForStmt(std::shared_ptr<parsetree::ast::ForStmt> stmt,
               std::shared_ptr<CFGNode> successor = nullptr);

  std::shared_ptr<CFGNode>
  visitReturnStmt(std::shared_ptr<parsetree::ast::ReturnStmt> stmt);

  std::shared_ptr<CFGNode>
  visitStmt(std::shared_ptr<parsetree::ast::Stmt> stmt,
            std::shared_ptr<CFGNode> successor = nullptr);

  std::shared_ptr<CFGNode>
  visitBlock(std::shared_ptr<parsetree::ast::Block> block,
             std::shared_ptr<CFGNode> successor = nullptr);
};

} // namespace static_check
