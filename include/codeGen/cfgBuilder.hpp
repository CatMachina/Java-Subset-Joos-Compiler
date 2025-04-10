#pragma once

#include "codeGen/cfg.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace codegen {

class CFGBuilder {
public:
  std::shared_ptr<CFG>
  buildCFG(const std::vector<std::shared_ptr<tir::Stmt>> &stmts);

private:
  int nodeCounter;
  std::shared_ptr<BasicBlock> currentBB;
  std::unordered_set<std::shared_ptr<tir::Stmt>> visited;
  std::unordered_map<std::string, std::shared_ptr<BasicBlock>> nameToPred;
  std::unordered_map<std::shared_ptr<tir::Stmt>, std::shared_ptr<BasicBlock>>
      stmtToPred;

  int getNextId() { return id++; }
  void init();

  void visitLabel(std::shared_ptr<tir::Label> label);
  void visitJump(std::shared_ptr<tir::Jump> jump);
  void visitCJump(std::shared_ptr<tir::CJump> cjump);
  void visitReturn(std::shared_ptr<tir::Return> returnStmt);
  void visitOrdinaryStmt(std::shared_ptr<tir::Stmt> stmt);
  void visitStmt(std::shared_ptr<tir::Stmt> stmt);
};

} // namespace codegen
