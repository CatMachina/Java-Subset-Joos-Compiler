#include "staticCheck/reachabilityAnalysis.hpp"
#include <queue>

namespace static_check {
  
  bool isReturnStatement(std::shared_ptr<parsetree::ast::Stmt> stmt) {
    return stmt != nullptr && !!std::dynamic_pointer_cast<parsetree::ast::ReturnStmt>(stmt);
  }

  bool ReachabilityAnalysis::run(std::shared_ptr<CFG> cfg)
  {
    std::shared_ptr<CFGNode> entry = cfg->getEntryNode();
    // If no entry, then method is empty, no reachability analysis needed
    if(!entry) return true;

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
        node->getReachabilityAnalysisInfo()->out(node->getReachabilityAnalysisInfo()->in());
      }
      q.pop();
      // Skip if it doesn't change reachability of predecessors
      if (!node->getReachabilityAnalysisInfo()->out()) continue;

      std::cout << "Neighbours\n";
      // Otherwise, compute reachability for successors
      for (auto successor : node->getSuccessors()) {
        if(!successor->getReachabilityAnalysisInfo()->in()) {
          successor->getReachabilityAnalysisInfo()->in(true);
          q.push(successor);
        }
      }
    }

    // Check in[n] for all nodes n
    for (const auto &node: cfg->getNodes()) {
      if (!node->getReachabilityAnalysisInfo()->in()) {
        return false;
      }
    }

    return true;
  }

} // namespace static_check
