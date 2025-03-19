#include "staticCheck/liveVariableAnalysis.hpp"
#include <queue>
#include <typeinfo>
#include <unordered_set>

namespace static_check {

bool isLocalVariable(std::shared_ptr<parsetree::ast::MemberName> astNode) {
  if (!astNode || !astNode->getResolvedDecl())
    return false;
  auto varDecl = std::dynamic_pointer_cast<parsetree::ast::VarDecl>(
      astNode->getResolvedDecl());
  if (!varDecl)
    return false;
  return !varDecl->isInParam() &&
         !std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(varDecl);
}

void precomputeExprStatement(
    std::shared_ptr<parsetree::ast::ExpressionStmt> exprStmt,
    std::shared_ptr<CFGNode> node) {
  // std::cout << "precomputeExprStatement\n";
  // exprStmt->print(std::cout);
  auto expr = exprStmt->getStatementExpr();
  if (!expr)
    return;
  auto exprNodes = expr->getExprNodes();
  // If assignment expression (with simple name), first node should be
  // identifier and last node should be =
  // TODO: Known issue, how to get variables used in field access or array
  // access, changes use & def
  int n = exprNodes.size();
  if (n == 0)
    return;
  auto var =
      std::dynamic_pointer_cast<parsetree::ast::MemberName>(exprNodes[0]);
  auto equals =
      std::dynamic_pointer_cast<parsetree::ast::Assignment>(exprNodes[n - 1]);

  // Get use/def vectors
  auto &use = node->getLiveVariableAnalysisInfo()->use();
  auto &def = node->getLiveVariableAnalysisInfo()->def();
  if (!var || !equals) {
    for (int i = 0; i < n; ++i) {
      if (auto memberName =
              std::dynamic_pointer_cast<parsetree::ast::MemberName>(
                  exprNodes[i])) {
        if (isLocalVariable(memberName))
          use.insert(memberName->getName());
      }
    }
  } else {
    // local variable expression statement
    for (int i = 1; i < n - 1; ++i) {
      if (auto memberName =
              std::dynamic_pointer_cast<parsetree::ast::MemberName>(
                  exprNodes[i])) {
        if (isLocalVariable(memberName))
          use.insert(memberName->getName());
      }
    }
    auto lvalue =
        std::dynamic_pointer_cast<parsetree::ast::MemberName>(exprNodes[0]);
    // std::cout << "isLocalVariable: " << lvalue->getName() << " " <<
    // isLocalVariable(lvalue) << "\n";
    if (isLocalVariable(lvalue)) {
      // std::cout << node->getId() << "\n";
      def.insert(lvalue->getName());
    }
  }
}

void precomputeExpr(std::shared_ptr<parsetree::ast::Expr> expr,
                    std::shared_ptr<CFGNode> node) {
  // std::cout << "precomputeExpr\n";
  if (!expr)
    return;
  auto exprNodes = expr->getExprNodes();
  auto &use = node->getLiveVariableAnalysisInfo()->use();
  for (auto exprNode : exprNodes) {
    auto lvalue =
        std::dynamic_pointer_cast<parsetree::ast::MemberName>(exprNode);
    // std::cout << "isLocalVariable: " << lvalue->getName() << " " <<
    // isLocalVariable(lvalue) << "\n";
    if (lvalue && isLocalVariable(lvalue)) {
      use.insert(lvalue->getName());
    }
  }
}

void precomputeVarDecl(std::shared_ptr<parsetree::ast::VarDecl> varDecl,
                       std::shared_ptr<CFGNode> node) {
  // std::cout << "precomputeVarDecl\n";
  if (varDecl->getInitializer()) {
    precomputeExpr(varDecl->getInitializer(), node);
  }
  // std::cout << varDecl->getName() << "\n";
  auto &def = node->getLiveVariableAnalysisInfo()->def();
  def.insert(varDecl->getName());
}

void precomputeUseDefHelper(
    std::shared_ptr<CFGNode> node,
    std::unordered_set<std::shared_ptr<CFGNode>> &visited) {
  if (node->getCondition()) {
    precomputeExpr(node->getCondition(), node);
  } else if (node->getStatement()) {
    if (auto returnStmt = std::dynamic_pointer_cast<parsetree::ast::ReturnStmt>(
            node->getStatement())) {
      precomputeExpr(returnStmt->getReturnExpr(), node);
    } else if (auto exprStmt =
                   std::dynamic_pointer_cast<parsetree::ast::ExpressionStmt>(
                       node->getStatement())) {
      precomputeExprStatement(exprStmt, node);
    } else if (auto declStmt =
                   std::dynamic_pointer_cast<parsetree::ast::DeclStmt>(
                       node->getStatement())) {
      if (declStmt->getDecl())
        precomputeVarDecl(declStmt->getDecl(), node);
    }
  }
  for (auto successor : node->getSuccessors()) {
    if (!visited.count(successor)) {
      visited.insert(successor);
      precomputeUseDefHelper(successor, visited);
    }
  }
}

void precomputeUseDef(std::shared_ptr<CFGNode> node) {
  std::unordered_set<std::shared_ptr<CFGNode>> visited;
  visited.insert(node);
  precomputeUseDefHelper(node, visited);
}

bool LiveVariableAnalysis::checkDeadAssignments(std::shared_ptr<CFG> cfg) {
  std::shared_ptr<CFGNode> entry = cfg->getEntryNode();
  // If no entry, then method is empty, no reachability analysis needed
  if (!entry)
    return true;
  // std::cout << "Filling\n";
  precomputeUseDef(cfg->getEntryNode());
  // std::cout << "Filled\n";
  std::queue<std::shared_ptr<CFGNode>> q;
  std::unordered_set<std::shared_ptr<CFGNode>> inQ;
  for (auto node : cfg->getNodes()) {
    if (node != cfg->getEndNode()) {
      q.push(node);
      inQ.insert(node);
    }
  }

  // std::cout << "Worklist\n";
  while (!q.empty()) {
    auto node = q.front();
    q.pop();
    inQ.erase(node);

    auto &in = node->getLiveVariableAnalysisInfo()->in();
    auto &out = node->getLiveVariableAnalysisInfo()->out();
    out.clear();
    for (auto successor : node->getSuccessors()) {
      const auto &succIn = successor->getLiveVariableAnalysisInfo()->in();
      out.insert(succIn.begin(), succIn.end());
    }

    // std::cout << "Computed out\n";
    std::unordered_set<std::string> oldIn = in;
    in.clear();
    std::unordered_set<std::string> outMinusDef = out;
    for (const std::string &var : node->getLiveVariableAnalysisInfo()->def()) {
      outMinusDef.erase(var);
    }
    in.insert(outMinusDef.begin(), outMinusDef.end());
    const auto &use = node->getLiveVariableAnalysisInfo()->use();
    in.insert(use.begin(), use.end());

    // std::cout << "Computed in\n";
    if (oldIn != in) {
      for (auto pred : node->getPredecessors()) {
        if (!inQ.count(pred)) {
          inQ.insert(pred);
          q.push(pred);
        }
      }
    }
    // std::cout << "Added preds\n";
  }

  // std::cout << "Analyzing\n";
  for (auto node : cfg->getNodes()) {
    auto declStmt = std::dynamic_pointer_cast<parsetree::ast::DeclStmt>(
        node->getStatement());
    if (node != cfg->getEndNode() && !declStmt) {
      const auto &def = node->getLiveVariableAnalysisInfo()->def();
      const auto &out = node->getLiveVariableAnalysisInfo()->out();
      // std::cout << node->getId() << "\n";
      // std::cout << "out:\n";
      for (const auto &var : out) {
        // std::cout << var << "\n";
      }
      // std::cout << "def:\n";
      for (const auto &var : def) {
        // std::cout << var << "\n";
        if (!out.count(var)) {
          // node->getStatement()->print(std::cout);
          return false;
        }
      }
    }
  }

  return true;
}

} // namespace static_check
