#include "staticCheck/liveVariableAnalysis.hpp"
#include <queue>
#include <unordered_set>

namespace static_check {

  void fillExprStatement(std::shared_ptr<parsetree::ast::ExpressionStmt> exprStmt, std::shared_ptr<CFGNode> node) {
    std::cout << "fillExprStatement\n";
    exprStmt->print(std::cout);
    auto expr = exprStmt->getStatementExpr();
    if (!expr) return;
    auto exprNodes = expr->getExprNodes();
    // If assignment expression (with simple name), first node should be identifier and last node should be =
    // TODO: Known issue, how to get variables used in field access or array access, changes use & def
    // TODO: Known issue, what about different variables with same name? Actually might be ok
    int n = exprNodes.size();
    if (n == 0) return;
    auto var = std::dynamic_pointer_cast < parsetree::ast::MemberName>(exprNodes[0]); 
    auto equals = std::dynamic_pointer_cast < parsetree::ast::Assignment>(exprNodes[n-1]);
    if (!var || !equals) return;
    
    // Get use/def vectors
    auto &use = node->getLiveVariableAnalysisInfo()->use();
    auto &def = node->getLiveVariableAnalysisInfo()->def();
    
    for (int i = 1; i < n-1; ++i) {
      if (auto memberName = std::dynamic_pointer_cast<parsetree::ast::MemberName>(exprNodes[i]))
      {
        use.insert(memberName->getName());
      }
    }
    auto lvalue = std::dynamic_pointer_cast<parsetree::ast::MemberName>(exprNodes[0]);
    def.insert(lvalue->getName());
  }
  
  void fillExpr(std::shared_ptr<parsetree::ast::Expr> expr, std::shared_ptr<CFGNode> node) {
    std::cout << "fillExpr\n";
    if (!expr) return;
    auto exprNodes = expr->getExprNodes();
    auto &use = node->getLiveVariableAnalysisInfo()->use();
    for (auto exprNode : exprNodes) {
      auto lvalue = std::dynamic_pointer_cast<parsetree::ast::MemberName>(exprNode);
      if (lvalue)
      {
        use.insert(lvalue->getName());
      }
    }
  }

  void fillAll(std::shared_ptr<CFG> cfg) {
    for (auto &node : cfg->getNodes()) {
      if (node->getCondition()) {
        fillExpr(node->getCondition(), node);
      } else if(node->getStatement()) {
        if(auto returnStmt = std::dynamic_pointer_cast<parsetree::ast::ReturnStmt>(node->getStatement())) {
          fillExpr(returnStmt->getReturnExpr(), node);
        }
        else if (auto exprStmt = std::dynamic_pointer_cast<parsetree::ast::ExpressionStmt>(node->getStatement()))
        {
          fillExprStatement(exprStmt, node);
        }
      }
    }
  }

  bool LiveVariableAnalysis::checkDeadAssignments(std::shared_ptr<CFG> cfg)
  {
    std::shared_ptr<CFGNode> entry = cfg->getEntryNode();
    // If no entry, then method is empty, no reachability analysis needed
    if (!entry)
    return true;
    std::cout << "Filling\n";
    fillAll(cfg);
    std::cout << "Filled\n";
    std::queue<std::shared_ptr<CFGNode>> q;
    std::unordered_set<std::shared_ptr<CFGNode>> inQ;
    for (auto node : cfg->getNodes()) {
      if (node != cfg->getEndNode()) {
        q.push(node);
        inQ.insert(node);
      }
    }
    
    std::cout << "Worklist\n";
    while(!q.empty()) {
      auto node = q.front(); q.pop();
      inQ.erase(node);
      
      auto &in = node->getLiveVariableAnalysisInfo()->in();
      auto &out = node->getLiveVariableAnalysisInfo()->out();
      out.clear();
      for (auto successor : node->getSuccessors()) {
        const auto &succIn = successor->getLiveVariableAnalysisInfo()->in();
        out.insert(succIn.begin(), succIn.end());
      }
      
      std::cout << "Computed out\n";
      std::unordered_set<std::string> oldIn = in; in.clear();
      std::unordered_set<std::string> outMinusDef = out;
      for (const std::string &var : node->getLiveVariableAnalysisInfo()->def()) {
        outMinusDef.erase(var);
      }
      in.insert(outMinusDef.begin(), outMinusDef.end());
      const auto &use = node->getLiveVariableAnalysisInfo()->use();
      in.insert(use.begin(), use.end());
      
      std::cout << "Computed in\n";
      if(oldIn != in) {
        for (auto pred : node->getPredecessors()) {
          if(!inQ.count(pred)) {
            inQ.insert(pred);
            q.push(pred);
          }
        }
      }
      std::cout << "Added preds\n";
    }
    
    std::cout << "Analyzing\n";
    for (auto node : cfg->getNodes())
    {
      if (node != cfg->getEndNode())
      {
        const auto &def = node->getLiveVariableAnalysisInfo()->def();
        const auto &out = node->getLiveVariableAnalysisInfo()->out();
        std::cout << "out: \n";
        for (const auto &var : out) {
          std::cout << var << "\n";
        }
        std::cout << "def: \n";
        for (const auto &var : def) {
          std::cout << var << "\n";
          if (!out.count(var)) {
            return false;
          }
        }
      }
    }

    return true;
  }

} // namespace static_check
