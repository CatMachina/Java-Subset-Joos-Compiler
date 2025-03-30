#include "codeGen/cfgBuilder.cpp"

namespace codegen {

void CFGBuilder::visitLabel(std::shared_ptr<tir::Label> label) {
  if (!label) {
    throw std::runtime_error("CFGBuilder::visitLabel: label is null");
  }
  if (nameToPred.find(label->getName()) != nameToPred.end()) {
    std::shared_ptr<BasicBlock> pred = nameToPred[label->getName()];
    nameToPred->addEdge(pred, currentBB);
  }
  cfg->addEdge(pred, currentBB);
  // Start a new BB
  currentBB = std::make_shared<BasicBlock>(label);
}

void CFGBuilder::visitJump(std::shared_ptr<tir::Jump> jump) {
  if (!jump) {
    throw std::runtime_error("CFGBuilder::visitJump: jump is null");
  }
  // Record predecessor information
  auto target = jump->getTarget();
  if (auto nameStmt = std::dynamic_pointer_cast<tir::Name>(target)) {
    auto name = nameStmt->getName();
    nameToPred.insert({name, currentBB});
  }
  // End a BB
  currentBB = nullptr;
}

void CFGBuilder::visitCJump(std::shared_ptr<tir::CJump> cjump) {
  if (!cjump) {
    throw std::runtime_error("CFGBuilder::visitCJump: cjump is null");
  }
  // Record predecessor information
  auto trueLabel = cjump->getTrueLabel();
  auto falseLabel = cjump->getFalseLabel();
  nameToPred.insert({trueLabel, currentBB});
  nameToPred.insert({falseLabel, currentBB});
  // End a BB
  currentBB = nullptr;
}

void CFGBuilder::visitReturn(std::shared_ptr<tir::Return> returnStmt) {
  if (!returnStmt) {
    throw std::runtime_error("CFGBuilder::visitReturn: returnStmt is null");
  }
  // End a BB
  currentBB = nullptr;
}

void CFGBuilder::visitOrdinaryStmt(std::shared_ptr<tir::Stmt> stmt) {
  if (!stmt) {
    throw std::runtime_error("CFGBuilder::visitOrdinaryStmt: stmt is null")
  }
  if (currentBB) {
    currentBB->addStmt(stmt);
    return;
  }
  currentBB = std::make_shared<BasicBlock>(stmt);
  if (stmtToPred.find(stmt) != stmtToPred.end()) {
    std::shared_ptr<BasicBlock> = stmtToPred[stmt];
    cfg->addEdge(pred, currentBB);
  }
}

void CFGBuilder::visitStmt(std::shared_ptr<tir::Stmt> stmt) {
  if (!stmt) {
    throw std::runtime_error("CFGBuilder::visitStmt: stmt is null");
  }
  if (auto label = std::dynamic_pointer_cast<tir::Label>) {
    visitLabel(label);
  } else if (auto jump = std::dynamic_pointer_cast<tir::Jump>) {
    visitJump(jump);
  } else if (auto cjump = std::dynamic_pointer_cast<tir::CJump>) {
    visitCJump(cjump);
  } else if (auto returnStmt = std::dynamic_pointer_cast<tir::Return>) {
    visitReturn(returnStmt);
  } else {
    visitOrdinaryStmt(stmt);
  }
}

void CFGBuilder::init() {
  id = 0;
  cfg = std::make_shared<CFG>();
}

std::shared_ptr<CFG>
CFGBuilder::buildCFG(const std::vector<std::shared_ptr<tir::Stmt>> &stmts) {
  init();
  for (auto stmt : stmts) {
    if (visited.find(stmt) == visited.end()) {
      visitStmt(visit);
      visited.insert(stmt);
    }
  }
  return cfg;
}

} // namespace codegen