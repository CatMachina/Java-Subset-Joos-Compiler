#include "codeGen/greedyReordering.hpp"

namespace codegen {

std::vector<std::shared_ptr<BasicBlock>>
GreedyReordering::constructMaximalTrace(std::shared_ptr<BasicBlock> start) {
  std::vector<std::shared_ptr<BasicBlock>> trace = {start};
  while (true) {
    bool foundUnmarked = false;
    for (auto succ : trace.back()->getSuccessors()) {
      if (unmarked.find(succ) != unmarked.end()) {
        foundUnmarked = true;
      }
      trace.push_back(succ);
      unmarked.erase(succ);
    }
    if (!foundUnmarked) {
      break;
    }
  }
  return trace;
}

void GreedyReordering::run() {
  // Initialize all basic blocks as unmarked
  for (int i = 0; i < cfg->getNumNodes(); ++i) {
    unmarked.insert(i);
  }
  // Repeat until all basic blocks have been marked
  while (!unmarked.empty()) {
    // Choose an unmarked basic block
    std::shared_ptr<BasicBlock> bb = *unmarked.begin();
    unmarked.erase(bb);
    // Construct a maximal unmarked trace
    std::vector<std::shared_ptr<BasicBlock>> trace = constructMaximalTrace(bb);
    // Append blocks in that trace to output code
    reorderResult.insert(reorderResult.end(), trace.begin(), trace.end());
  }
}

} // namespace codegen