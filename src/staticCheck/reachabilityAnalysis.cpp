#include "staticCheck/reachabilityAnalysis.hpp"
#include <queue>

namespace static_check {

bool isReturnStatement(std::shared_ptr<parsetree::ast::Stmt> stmt) {
  return stmt != nullptr &&
         !!std::dynamic_pointer_cast<parsetree::ast::ReturnStmt>(stmt);
}

bool ReachabilityAnalysis::checkUnreachableStatements(
    std::shared_ptr<CFG> cfg) {
  std::shared_ptr<CFGNode> entry = cfg->getEntryNode();
  // If no entry, then method is empty, no reachability analysis needed
  if (!entry)
    return true;

  std::queue<std::shared_ptr<CFGNode>> q;
  q.push(entry);
  entry->getReachabilityAnalysisInfo()->in(true);

  std::unordered_set<int> visited;

  // Doing forward reachability analysis, can just BFS
  while (!q.empty()) {
    auto node = q.front();
    if (isReturnStatement(node->getStatement())) {
      node->getReachabilityAnalysisInfo()->out(false);
    } else {
      node->getReachabilityAnalysisInfo()->out(
          node->getReachabilityAnalysisInfo()->in());
    }
    q.pop();
    // Skip if it doesn't change reachability of predecessors
    if (!node->getReachabilityAnalysisInfo()->out())
      continue;

    std::cout << "Neighbours\n";
    // Otherwise, compute reachability for successors
    for (auto successor : node->getSuccessors()) {
      if (!successor->getReachabilityAnalysisInfo()->in()) {
        successor->getReachabilityAnalysisInfo()->in(true);
        q.push(successor);
      }
    }
  }

  // Check in[n] for all nodes n for unreachable statements
  for (const auto &node : cfg->getNodes()) {
    if (node != cfg->getEndNode() &&
        !node->getReachabilityAnalysisInfo()->in()) {
      // When checking, sentinel end node is not included
      return false;
    }
  }

  return true;
}

bool ReachabilityAnalysis::checkFiniteLengthReturn(
    std::shared_ptr<CFG> cfg,
    std::shared_ptr<parsetree::ast::MethodDecl> method) {
  // Check if finite-length execution results in explicit return by checking
  // sentinel node
  if (cfg->getEndNode() &&
      cfg->getEndNode()->getReachabilityAnalysisInfo()->in() &&
      method->getReturnType() && !method->getReturnType()->isNull()) {
    return false;
  }
  return true;
}

} // namespace static_check
