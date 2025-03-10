#include "staticCheck/cfgBuilder.hpp"

namespace static_check {

std::shared_ptr<CFGNode>
CFGBuilder::visitIfStmt(std::shared_ptr<parsetree::ast::IfStmt> stmt,
                        std::shared_ptr<CFGNode> successor) {
  if (!stmt) {
    std::cout << "CFGBuilder::visitIfStmt: stmt is null" << std::endl;
    return nullptr;
  }
  std::shared_ptr<parsetree::ast::Expr> condition = stmt->getCondition();
  std::shared_ptr<parsetree::ast::Stmt> ifBody = stmt->getIfBody();
  std::shared_ptr<parsetree::ast::Stmt> elseBody = stmt->getElseBody();
  std::shared_ptr<CFGNode> branchNode =
      std::make_shared<CFGNode>(getNextId(), stmt, condition);
  cfg->addNode(branchNode);

  std::shared_ptr<CFGNode> thenNode = visitStmt(ifBody);
  std::shared_ptr<CFGNode> elseNode = visitStmt(elseBody);
  // TODO: refactor
  // if-then-else
  if (elseNode) {
    cfg->addEdge(branchNode, thenNode);
    cfg->addEdge(branchNode, elseNode);
    cfg->addEdge(thenNode, successor);
    cfg->addEdge(elseNode, successor);
  }
  // if-then
  else {
    cfg->addEdge(branchNode, thenNode);
    cfg->addEdge(branchNode, successor);
    cfg->addEdge(thenNode, successor);
  }
  return branchNode;
}

std::shared_ptr<CFGNode>
CFGBuilder::visitWhileStmt(std::shared_ptr<parsetree::ast::WhileStmt> stmt,
                           std::shared_ptr<CFGNode> successor) {
  if (!stmt) {
    std::cout << "CFGBuilder::visitWhileStmt: stmt is null" << std::endl;
    return nullptr;
  }
  std::shared_ptr<parsetree::ast::Expr> condition = stmt->getCondition();
  std::shared_ptr<parsetree::ast::Stmt> whileBody = stmt->getWhileBody();
  std::shared_ptr<CFGNode> branchNode =
      std::make_shared<CFGNode>(getNextId(), stmt, condition);
  cfg->addNode(branchNode);
  std::shared_ptr<CFGNode> bodyNode = visitStmt(whileBody);
  // true branch
  cfg->addEdge(branchNode, bodyNode);
  cfg->addEdge(bodyNode, branchNode);
  // false branch
  cfg->addEdge(branchNode, successor);
  return branchNode;
}

std::shared_ptr<CFGNode>
CFGBuilder::visitForStmt(std::shared_ptr<parsetree::ast::ForStmt> stmt,
                         std::shared_ptr<CFGNode> successor) {
  if (!stmt) {
    std::cout << "CFGBuilder::visitForStmt: stmt is null" << std::endl;
    return nullptr;
  }
  std::shared_ptr<parsetree::ast::Stmt> forInit = stmt->getForInit();
  std::shared_ptr<parsetree::ast::Expr> condition = stmt->getCondition();
  std::shared_ptr<parsetree::ast::Stmt> forUpdate = stmt->getForUpdate();
  std::shared_ptr<parsetree::ast::Stmt> forBody = stmt->getForBody();

  std::shared_ptr<CFGNode> branchNode =
      std::make_shared<CFGNode>(getNextId(), stmt, condition);
  cfg->addNode(branchNode);
  std::shared_ptr<CFGNode> initNode = visitStmt(forInit);
  std::shared_ptr<CFGNode> updateNode = visitStmt(forUpdate);
  std::shared_ptr<CFGNode> bodyNode = visitStmt(forBody);

  // FIXME: probably not right
  cfg->addEdge(initNode, branchNode);
  // true branch
  cfg->addEdge(branchNode, bodyNode);
  cfg->addEdge(bodyNode, updateNode);
  cfg->addEdge(updateNode, branchNode);
  // false banch
  cfg->addEdge(branchNode, successor);
  return initNode;
}

std::shared_ptr<CFGNode>
CFGBuilder::visitReturnStmt(std::shared_ptr<parsetree::ast::ReturnStmt> stmt) {
  if (!stmt) {
    std::cout << "CFGBuilder::visitReturnStmt: stmt is null" << std::endl;
    return nullptr;
  }
  std::shared_ptr<CFGNode> node = std::make_shared<CFGNode>(getNextId(), stmt);
  // no successor
  return node;
}

std::shared_ptr<CFGNode>
CFGBuilder::visitStmt(std::shared_ptr<parsetree::ast::Stmt> stmt,
                      std::shared_ptr<CFGNode> successor) {
  if (!stmt) {
    std::cout << "CFGBuilder::visitStmt: stmt is null" << std::endl;
    return nullptr;
  }
  std::shared_ptr<CFGNode> node;
  if (auto block = std::dynamic_pointer_cast<parsetree::ast::Block>(stmt)) {
    node = visitBlock(block, successor);
  } else if (auto ifStmt =
                 std::dynamic_pointer_cast<parsetree::ast::IfStmt>(stmt)) {
    node = visitIfStmt(ifStmt, successor);
  } else if (auto whileStmt =
                 std::dynamic_pointer_cast<parsetree::ast::WhileStmt>(stmt)) {
    node = visitWhileStmt(whileStmt, successor);
  } else if (auto forStmt =
                 std::dynamic_pointer_cast<parsetree::ast::ForStmt>(stmt)) {
    node = visitForStmt(forStmt, successor);
  } else if (auto returnStmt =
                 std::dynamic_pointer_cast<parsetree::ast::ReturnStmt>(stmt)) {
    node = visitReturnStmt(returnStmt);
  } else {
    node = std::make_shared<CFGNode>(getNextId(), stmt);
    cfg->addNode(node);
    cfg->addEdge(node, successor);
  }
  return node;
}

std::shared_ptr<CFGNode>
CFGBuilder::visitBlock(std::shared_ptr<parsetree::ast::Block> block,
                       std::shared_ptr<CFGNode> successor) {
  if (!block) {
    throw std::runtime_error("CFGBuilder::visitBlock: block is null");
  }
  auto stmts = block->getStatements();
  for (auto it = stmts.rbegin(); it != stmts.rend(); ++it) {
    auto node = visitStmt(*it, successor);
    successor = node;
  }
  return successor;
}

std::shared_ptr<CFG>
CFGBuilder::buildCFG(std::shared_ptr<parsetree::ast::Block> methodBody) {
  std::shared_ptr<CFGNode> entryNode = visitBlock(methodBody);
  cfg = std::make_shared<CFG>(entryNode);
  return cfg;
}

} // namespace static_check
