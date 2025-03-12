#include "staticCheck/cfgBuilder.hpp"
#include "ast/astExprNode.hpp"
#include <stack>

namespace static_check {

static bool canConstantFold(std::shared_ptr<parsetree::ast::Expr> expr) {
  if (!expr) {
    throw std::runtime_error("canConstantFold: erpr is null");
  }
  for (auto node : expr->getExprNodes()) {
    auto literal = std::dynamic_pointer_cast<parsetree::ast::Literal>(node);
    auto unOp = std::dynamic_pointer_cast<parsetree::ast::UnOp>(node);
    auto binOp = std::dynamic_pointer_cast<parsetree::ast::BinOp>(node);
    bool isBooleanUnOp =
        (unOp && unOp->getOp() == parsetree::ast::UnOp::OpType::Not);
    bool isBooleanBinOp = false;
    if (binOp) {
      switch (binOp->getOp()) {
      case parsetree::ast::BinOp::OpType::And:
      case parsetree::ast::BinOp::OpType::Or:
      case parsetree::ast::BinOp::OpType::BitWiseAnd:
      case parsetree::ast::BinOp::OpType::BitWiseOr:
      case parsetree::ast::BinOp::OpType::Equal:
      case parsetree::ast::BinOp::OpType::NotEqual:
      case parsetree::ast::BinOp::OpType::GreaterThan:
      case parsetree::ast::BinOp::OpType::GreaterThanOrEqual:
      case parsetree::ast::BinOp::OpType::LessThan:
      case parsetree::ast::BinOp::OpType::LessThanOrEqual:
        isBooleanBinOp = true;
        break;
      }
    }
    if (!literal && !isBooleanUnOp && !isBooleanBinOp)
      return false;
  }
  return true;
}

// TODO: may need to handle char/string literals as well?
static std::variant<bool, u_int32_t>
evaluateBoolConstantExpr(std::shared_ptr<parsetree::ast::Expr> expr) {
  auto exprNodes = expr->getExprNodes();
  std::stack<std::variant<bool, u_int32_t>> st;
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
        st.push(std::variant<bool, u_int32_t>(*lhsBool && *rhsBool));
        break;
      case parsetree::ast::BinOp::OpType::Or:
      case parsetree::ast::BinOp::OpType::BitWiseOr:
        st.push(std::variant<bool, u_int32_t>(*lhsBool || *rhsBool));
        break;
      case parsetree::ast::BinOp::OpType::Equal:
        if (lhsBool) {
          st.push(std::variant<bool, u_int32_t>(*lhsBool == *rhsBool));
        } else {
          st.push(std::variant<bool, u_int32_t>(*lhsNum == *rhsNum));
        }
        break;
      case parsetree::ast::BinOp::OpType::NotEqual:
        if (lhsBool) {
          st.push(std::variant<bool, u_int32_t>(*lhsBool != *rhsBool));
        } else {
          st.push(std::variant<bool, u_int32_t>(*lhsNum != *rhsNum));
        }
        break;
      case parsetree::ast::BinOp::OpType::Plus:
        st.push(std::variant<bool, u_int32_t>(*lhsNum + *rhsNum));
        break;
      case parsetree::ast::BinOp::OpType::Minus:
        st.push(std::variant<bool, u_int32_t>(*lhsNum - *rhsNum));
        break;
      case parsetree::ast::BinOp::OpType::Multiply:
        st.push(std::variant<bool, u_int32_t>(*lhsNum * *rhsNum));
        break;
      case parsetree::ast::BinOp::OpType::Subtract:
        st.push(std::variant<bool, u_int32_t>(*lhsNum / *rhsNum));
        break;
      case parsetree::ast::BinOp::OpType::Modulo:
        st.push(std::variant<bool, u_int32_t>(*lhsNum % *rhsNum));
        break;
      default:
        throw std::runtime_error(
            "evaluateBoolConstantExpr: not a boolean/numeric binary op");
        ;
      }
    } else if (auto unOp =
                   std::dynamic_pointer_cast<parsetree::ast::UnOp>(exprNode)) {
      auto unOpType = unOp->getOp();
      auto operand = st.top();
      st.pop();
      auto operandBool = std::get_if<bool>(&operand);
      auto operandNum = std::get_if<u_int32_t>(&operand);
      if (unOpType == parsetree::ast::UnOp::OpType::Not) {
        st.push(std::variant<bool, u_int32_t>(!(*operandBool)));
      } else if (unOpType == parsetree::ast::UnOp::OpType::Minus) {
        st.push(std::variant<bool, u_int32_t>(-(*operandNum)));
      } else {
        throw std::runtime_error(
            "evaluateBoolConstantExpr: not a boolean/numeric unary op");
      }
    } else if (auto literal =
                   std::dynamic_pointer_cast<parsetree::ast::Literal>(
                       exprNode)) {
      u_int32_t val = literal->getAsInt();
      if (literal->getBasicType()->isNumeric()) {
        st.push(std::variant<bool, u_int32_t>(val));
      } else if (val == 1) {
        st.push(std::variant<bool, u_int32_t>(true));
      } else if (val == 0) {
        st.push(std::variant<bool, u_int32_t>(false));
      } else {
        throw std::runtime_error(
            "evaluateBoolConstantExpr: not a boolean/numeric literal");
      }
    } else {
      throw std::runtime_error(
          "evaluateBoolConstantExpr: not a boolean/numeric literal or op");
    }
  }
  return st.top();
}

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

  if (!canConstantFold(condition)) {
    cfg->addEdge(branchNode, bodyNode);  // true branch
    cfg->addEdge(branchNode, successor); // false branch
    return branchNode;
  }

  auto eval = evaluateBoolConstantExpr(condition);
  auto evalResult = std::get_if<bool>(&eval);
  if (evalResult == nullptr) {
    throw std::runtime_error(
        "CFGBuilder::buildWhileStmt: eval result is not a boolean");
  }

  if (*evalResult) {
    cfg->addEdge(branchNode, bodyNode); // true branch
  } else {
    cfg->addEdge(branchNode, successor); // false branch
  }

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

  if (!canConstantFold(condition)) {
    cfg->addEdge(branchNode, bodyNode);  // true branch
    cfg->addEdge(branchNode, successor); // false banch
    return initNode;
  }

  auto eval = evaluateBoolConstantExpr(condition);
  auto evalResult = std::get_if<bool>(&eval);
  if (evalResult == nullptr) {
    throw std::runtime_error(
        "CFGBuilder::buildForStmt: eval result is not a boolean");
  }

  if (*evalResult) {
    cfg->addEdge(branchNode, bodyNode); // true branch
  } else {
    cfg->addEdge(branchNode, successor); // false branch
  }

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
