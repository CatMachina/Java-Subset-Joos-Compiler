#pragma once

#include "cfg.hpp"

namespace static_check {

class CFGBuilder {
public:
  CFGBuilder() : id{0} {}

  void init();

  std::shared_ptr<CFG>
  buildCFG(std::shared_ptr<parsetree::ast::MethodDecl> method);

private:
  int id;
  std::shared_ptr<CFG> cfg;

  int getNextId() { return id++; };

  std::shared_ptr<CFGNode>
  buildIfStmt(std::shared_ptr<parsetree::ast::IfStmt> stmt,
              std::shared_ptr<CFGNode> successor = nullptr);

  std::shared_ptr<CFGNode>
  buildWhileStmt(std::shared_ptr<parsetree::ast::WhileStmt> stmt,
                 std::shared_ptr<CFGNode> successor = nullptr);

  std::shared_ptr<CFGNode>
  buildForStmt(std::shared_ptr<parsetree::ast::ForStmt> stmt,
               std::shared_ptr<CFGNode> successor = nullptr);

  std::shared_ptr<CFGNode>
  buildReturnStmt(std::shared_ptr<parsetree::ast::ReturnStmt> stmt);

  std::shared_ptr<CFGNode>
  buildOtherStmt(std::shared_ptr<parsetree::ast::Stmt> stmt,
                 std::shared_ptr<CFGNode> successor = nullptr);

  std::shared_ptr<CFGNode>
  buildStmt(std::shared_ptr<parsetree::ast::Stmt> stmt,
            std::shared_ptr<CFGNode> successor = nullptr);

  std::shared_ptr<CFGNode>
  buildBlock(std::shared_ptr<parsetree::ast::Block> block,
             std::shared_ptr<CFGNode> successor = nullptr);
};

} // namespace static_check
