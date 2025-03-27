#include "staticCheck/cfgBuilder.hpp"
#include "ast/astExprNode.hpp"
#include <stack>

namespace static_check {

using EvalType = std::variant<bool, u_int32_t, std::nullptr_t>;

// TODO: may need to handle char/string literals as well?
static EvalType
evaluateBoolConstantExpr(std::shared_ptr<parsetree::ast::Expr> expr) {
  auto exprNodes = expr->getExprNodes();
  std::stack<EvalType> st;
  for (auto exprNode : exprNodes) {
    if (auto binOp =
            std::dynamic_pointer_cast<parsetree::ast::BinOp>(exprNode)) {
      auto binOpType = binOp->getOp();
      auto rhs = st.top();
      st.pop();
      auto lhs = st.top();
      st.pop();
      auto lhsBool = std::get_if<bool>(&lhs);
      auto rhsBool = std::get_if<bool>(&rhs);
      auto lhsNum = std::get_if<u_int32_t>(&lhs);
      auto rhsNum = std::get_if<u_int32_t>(&rhs);
      switch (binOpType) {
      case parsetree::ast::BinOp::OpType::And:
      case parsetree::ast::BinOp::OpType::BitWiseAnd:
        st.push(EvalType(*lhsBool && *rhsBool));
        break;
      case parsetree::ast::BinOp::OpType::Or:
      case parsetree::ast::BinOp::OpType::BitWiseOr:
        st.push(EvalType(*lhsBool || *rhsBool));
        break;
      case parsetree::ast::BinOp::OpType::Equal:
        if (lhsBool) {
          st.push(EvalType(*lhsBool == *rhsBool));
        } else {
          st.push(EvalType(*lhsNum == *rhsNum));
        }
        break;
      case parsetree::ast::BinOp::OpType::NotEqual:
        if (lhsBool) {
          st.push(EvalType(*lhsBool != *rhsBool));
        } else {
          st.push(EvalType(*lhsNum != *rhsNum));
        }
        break;
      case parsetree::ast::BinOp::OpType::Plus:
        st.push(EvalType(*lhsNum + *rhsNum));
        break;
      case parsetree::ast::BinOp::OpType::Minus:
        st.push(EvalType(*lhsNum - *rhsNum));
        break;
      case parsetree::ast::BinOp::OpType::Multiply:
        st.push(EvalType(*lhsNum * *rhsNum));
        break;
      case parsetree::ast::BinOp::OpType::Subtract:
        st.push(EvalType(*lhsNum / *rhsNum));
        break;
      case parsetree::ast::BinOp::OpType::Modulo:
        st.push(EvalType(*lhsNum % *rhsNum));
        break;
      default:
        return EvalType(nullptr);
      }
    } else if (auto unOp =
                   std::dynamic_pointer_cast<parsetree::ast::UnOp>(exprNode)) {
      auto unOpType = unOp->getOp();
      auto operand = st.top();
      st.pop();
      auto operandBool = std::get_if<bool>(&operand);
      auto operandNum = std::get_if<u_int32_t>(&operand);
      if (unOpType == parsetree::ast::UnOp::OpType::Not) {
        st.push(EvalType(!(*operandBool)));
      } else if (unOpType == parsetree::ast::UnOp::OpType::Minus) {
        st.push(EvalType(-(*operandNum)));
      } else {
        return EvalType(nullptr);
      }
    } else if (auto literal =
                   std::dynamic_pointer_cast<parsetree::ast::Literal>(
                       exprNode)) {
      u_int32_t val = literal->getAsInt();
      if (literal->getBasicType()->isNumeric()) {
        st.push(EvalType(val));
      } else if (val == 1) {
        st.push(EvalType(true));
      } else if (val == 0) {
        st.push(EvalType(false));
      } else {
        return EvalType(nullptr);
      }
    } else {
      return EvalType(nullptr);
    }
  }
  return st.top();
}

std::shared_ptr<CFGNode>
CFGBuilder::buildIfStmt(std::shared_ptr<parsetree::ast::IfStmt> stmt,
                        std::shared_ptr<CFGNode> successor) {
  if (!stmt) {
    // std::cout << "CFGBuilder::buildIfStmt: stmt is null" << std::endl;
    return nullptr;
  }
  // std::cout << "CFGBuilder::buildIfStmt" << std::endl;

  std::shared_ptr<parsetree::ast::Expr> condition = stmt->getCondition();
  std::shared_ptr<parsetree::ast::Stmt> ifBody = stmt->getIfBody();
  std::shared_ptr<parsetree::ast::Stmt> elseBody = stmt->getElseBody();
  auto branchNode = std::make_shared<CFGNode>(getNextId(), stmt, condition);
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
    // std::cout << "CFGBuilder::buildWhileStmt: stmt is null" << std::endl;
    return nullptr;
  }
  // std::cout << "CFGBuilder::buildWhileStmt" << std::endl;

  std::shared_ptr<parsetree::ast::Expr> condition = stmt->getCondition();
  std::shared_ptr<parsetree::ast::Stmt> whileBody = stmt->getWhileBody();
  auto branchNode = std::make_shared<CFGNode>(getNextId(), stmt, condition);
  cfg->addNode(branchNode);
  std::shared_ptr<CFGNode> bodyNode = buildStmt(whileBody, branchNode);

  auto eval = evaluateBoolConstantExpr(condition);
  auto evalResult = std::get_if<bool>(&eval);
  if (evalResult == nullptr) {
    // std::cout << "Cannot constant fold" << std::endl;
    cfg->addEdge(branchNode, bodyNode);  // true branch
    cfg->addEdge(branchNode, successor); // false branch
    return branchNode;
  }

  if (*evalResult) {
    // std::cout << "True branch only" << std::endl;
    cfg->addEdge(branchNode, bodyNode); // true branch
  } else {
    // std::cout << "False branch only" << std::endl;
    cfg->addEdge(branchNode, successor); // false branch
  }

  return branchNode;
}

std::shared_ptr<CFGNode>
CFGBuilder::buildForStmt(std::shared_ptr<parsetree::ast::ForStmt> stmt,
                         std::shared_ptr<CFGNode> successor) {
  if (!stmt) {
    // std::cout << "CFGBuilder::buildForStmt: stmt is null" << std::endl;
    return nullptr;
  }
  // std::cout << "CFGBuilder::buildForStmt" << std::endl;

  std::shared_ptr<parsetree::ast::Stmt> forInit = stmt->getForInit();
  std::shared_ptr<parsetree::ast::Expr> condition = stmt->getCondition();
  std::shared_ptr<parsetree::ast::Stmt> forUpdate = stmt->getForUpdate();
  std::shared_ptr<parsetree::ast::Stmt> forBody = stmt->getForBody();

  auto branchNode = std::make_shared<CFGNode>(getNextId(), stmt, condition);
  cfg->addNode(branchNode);

  // FIXME: probably not right
  std::shared_ptr<CFGNode> initNode = buildStmt(forInit, branchNode);
  std::shared_ptr<CFGNode> updateNode = buildStmt(forUpdate, branchNode);
  std::shared_ptr<CFGNode> bodyNode = buildStmt(forBody, updateNode);

  auto eval = evaluateBoolConstantExpr(condition);
  auto evalResult = std::get_if<bool>(&eval);
  if (evalResult == nullptr) {
    // std::cout << "Cannot constant fold" << std::endl;
    cfg->addEdge(branchNode, bodyNode);  // true branch
    cfg->addEdge(branchNode, successor); // false branch
    return initNode;
  }

  if (*evalResult) {
    // std::cout << "True branch only" << std::endl;
    cfg->addEdge(branchNode, bodyNode); // true branch
  } else {
    // std::cout << "False branch only" << std::endl;
    cfg->addEdge(branchNode, successor); // false branch
  }

  return initNode;
}

std::shared_ptr<CFGNode>
CFGBuilder::buildReturnStmt(std::shared_ptr<parsetree::ast::ReturnStmt> stmt) {
  if (!stmt) {
    // std::cout << "CFGBuilder::buildReturnStmt: stmt is null" << std::endl;
    return nullptr;
  }
  // std::cout << "CFGBuilder::buildReturnStmt" << std::endl;
  // no successor
  auto node = std::make_shared<CFGNode>(getNextId(), stmt);
  cfg->addNode(node);
  return node;
}

std::shared_ptr<CFGNode>
CFGBuilder::buildOtherStmt(std::shared_ptr<parsetree::ast::Stmt> stmt,
                           std::shared_ptr<CFGNode> successor) {
  if (!stmt) {
    throw std::runtime_error("CFGBuilder::buildOtherStmt: stmt is null");
  }
  // std::cout << "CFGBuilder::buildOtherStmt" << std::endl;
  auto node = std::make_shared<CFGNode>(getNextId(), stmt);
  cfg->addNode(node);
  cfg->addEdge(node, successor);
  return node;
}

std::shared_ptr<CFGNode>
CFGBuilder::buildStmt(std::shared_ptr<parsetree::ast::Stmt> stmt,
                      std::shared_ptr<CFGNode> successor) {
  if (!stmt) {
    // std::cout << "CFGBuilder::buildStmt: stmt is null" << std::endl;
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
    node = buildOtherStmt(stmt, successor);
  }
  return node;
}

std::shared_ptr<CFGNode>
CFGBuilder::buildBlock(std::shared_ptr<parsetree::ast::Block> block,
                       std::shared_ptr<CFGNode> successor) {
  if (!block) {
    throw std::runtime_error("CFGBuilder::buildBlock: block is null");
  }
  // std::cout << "CFGBuilder::buildBlock" << std::endl;

  auto stmts = block->getStatements();
  if (stmts.empty()) {
    return buildOtherStmt(block, successor);
  }
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
  // Create CFG with sentinel node for easier analysis
  auto methodBody = method->getMethodBody();
  auto stmts = methodBody->getStatements();
  auto sentinelStmt = std::make_shared<parsetree::ast::Block>();
  stmts.push_back(sentinelStmt);
  auto newMethodBody = std::make_shared<parsetree::ast::Block>(stmts);

  std::shared_ptr<CFGNode> entryNode = buildBlock(newMethodBody);
  cfg->setEntryNode(entryNode);

  // Set sentinel node
  for (auto &node : cfg->getNodes()) {
    if (node->getStatement() == sentinelStmt) {
      cfg->setEndNode(node);
      break;
    }
  }
  return cfg;
}

} // namespace static_check
