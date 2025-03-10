#include "staticCheck/cfgBuilder.hpp"

namespace static_check {

std::shared_ptr<CFGNode>
CFGBuilder::buildIfStmt(std::shared_ptr<parsetree::ast::IfStmt> stmt,
                        std::shared_ptr<CFGNode> successor) {
  if (!stmt) {
    std::cout << "CFGBuilder::buildIfStmt: stmt is null" << std::endl;
    return nullptr;
  }
  std::cout << "CFGBuilder::buildIfStmt" << std::endl;

  std::shared_ptr<parsetree::ast::Expr> condition = stmt->getCondition();
  std::shared_ptr<parsetree::ast::Stmt> ifBody = stmt->getIfBody();
  std::shared_ptr<parsetree::ast::Stmt> elseBody = stmt->getElseBody();
  std::shared_ptr<CFGNode> branchNode =
      std::make_shared<CFGNode>(getNextId(), stmt, condition);
  cfg->addNode(branchNode);

  std::shared_ptr<CFGNode> ifNode = buildStmt(ifBody, successor);
  cfg->addEdge(branchNode, ifNode);
  // if-then
  if (!elseBody) {
    cfg->addEdge(branchNode, successor);
  }
  // if-then-else
  else {
    std::shared_ptr<CFGNode> elseNode = buildStmt(elseBody, successor);
    cfg->addEdge(branchNode, elseNode);
  }
  return branchNode;
}

std::shared_ptr<CFGNode>
CFGBuilder::buildWhileStmt(std::shared_ptr<parsetree::ast::WhileStmt> stmt,
                           std::shared_ptr<CFGNode> successor) {
  if (!stmt) {
    std::cout << "CFGBuilder::buildWhileStmt: stmt is null" << std::endl;
    return nullptr;
  }
  std::cout << "CFGBuilder::buildWhileStmt" << std::endl;

  std::shared_ptr<parsetree::ast::Expr> condition = stmt->getCondition();
  std::shared_ptr<parsetree::ast::Stmt> whileBody = stmt->getWhileBody();
  std::shared_ptr<CFGNode> branchNode =
      std::make_shared<CFGNode>(getNextId(), stmt, condition);
  cfg->addNode(branchNode);
  std::shared_ptr<CFGNode> bodyNode = buildStmt(whileBody, branchNode);
  cfg->addEdge(branchNode, bodyNode);  // true branch
  cfg->addEdge(branchNode, successor); // false branch
  return branchNode;
}

std::shared_ptr<CFGNode>
CFGBuilder::buildForStmt(std::shared_ptr<parsetree::ast::ForStmt> stmt,
                         std::shared_ptr<CFGNode> successor) {
  if (!stmt) {
    std::cout << "CFGBuilder::buildForStmt: stmt is null" << std::endl;
    return nullptr;
  }
  std::cout << "CFGBuilder::buildForStmt" << std::endl;

  std::shared_ptr<parsetree::ast::Stmt> forInit = stmt->getForInit();
  std::shared_ptr<parsetree::ast::Expr> condition = stmt->getCondition();
  std::shared_ptr<parsetree::ast::Stmt> forUpdate = stmt->getForUpdate();
  std::shared_ptr<parsetree::ast::Stmt> forBody = stmt->getForBody();

  std::shared_ptr<CFGNode> branchNode =
      std::make_shared<CFGNode>(getNextId(), stmt, condition);
  cfg->addNode(branchNode);

  // FIXME: probably not right
  std::shared_ptr<CFGNode> initNode = buildStmt(forInit, branchNode);
  std::shared_ptr<CFGNode> updateNode = buildStmt(forUpdate, branchNode);
  std::shared_ptr<CFGNode> bodyNode = buildStmt(forBody, updateNode);
  cfg->addEdge(branchNode, bodyNode);  // true branch
  cfg->addEdge(branchNode, successor); // false banch
  return initNode;
}

std::shared_ptr<CFGNode>
CFGBuilder::buildReturnStmt(std::shared_ptr<parsetree::ast::ReturnStmt> stmt) {
  if (!stmt) {
    std::cout << "CFGBuilder::buildReturnStmt: stmt is null" << std::endl;
    return nullptr;
  }
  std::cout << "CFGBuilder::buildReturnStmt" << std::endl;
  // no successor
  std::shared_ptr<CFGNode> node = std::make_shared<CFGNode>(getNextId(), stmt);
  cfg->addNode(node);
  return node;
}

std::shared_ptr<CFGNode>
CFGBuilder::buildStmt(std::shared_ptr<parsetree::ast::Stmt> stmt,
                      std::shared_ptr<CFGNode> successor) {
  if (!stmt) {
    std::cout << "CFGBuilder::buildStmt: stmt is null" << std::endl;
    return nullptr;
  }
  std::shared_ptr<CFGNode> node;
  if (auto block = std::dynamic_pointer_cast<parsetree::ast::Block>(stmt)) {
    node = buildBlock(block, successor);
  } else if (auto ifStmt =
                 std::dynamic_pointer_cast<parsetree::ast::IfStmt>(stmt)) {
    node = buildIfStmt(ifStmt, successor);
  } else if (auto whileStmt =
                 std::dynamic_pointer_cast<parsetree::ast::WhileStmt>(stmt)) {
    node = buildWhileStmt(whileStmt, successor);
  } else if (auto forStmt =
                 std::dynamic_pointer_cast<parsetree::ast::ForStmt>(stmt)) {
    node = buildForStmt(forStmt, successor);
  } else if (auto returnStmt =
                 std::dynamic_pointer_cast<parsetree::ast::ReturnStmt>(stmt)) {
    node = buildReturnStmt(returnStmt);
  } else {
    std::cout << "CFGBuilder::buildStmt" << std::endl;
    node = std::make_shared<CFGNode>(getNextId(), stmt);
    cfg->addNode(node);
    cfg->addEdge(node, successor);
  }
  return node;
}

std::shared_ptr<CFGNode>
CFGBuilder::buildBlock(std::shared_ptr<parsetree::ast::Block> block,
                       std::shared_ptr<CFGNode> successor) {
  if (!block) {
    throw std::runtime_error("CFGBuilder::buildBlock: block is null");
  }
  std::cout << "CFGBuilder::buildBlock" << std::endl;

  auto stmts = block->getStatements();
  for (auto it = stmts.rbegin(); it != stmts.rend(); ++it) {
    auto node = buildStmt(*it, successor);
    successor = node;
  }
  return successor;
}

void CFGBuilder::init() {
  id = 0;
  cfg = std::make_shared<CFG>();
}

std::shared_ptr<CFG>
CFGBuilder::buildCFG(std::shared_ptr<parsetree::ast::MethodDecl> method) {
  if (!method || !method->hasBody()) {
    return nullptr;
  }
  init();
  std::shared_ptr<CFGNode> entryNode = buildBlock(method->getMethodBody());
  cfg->setEntryNode(entryNode);
  return cfg;
}

} // namespace static_check
