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

void GreedyReordering::reorder() {
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

// Unconditional jumps to the very next basic block should be deleted.
// An unconditional jump to the correct label should be added to the end of each
// basic block that does not transfer control to the following one, including
// cases where the “false” case of a CJUMP does not jump to the following block.
// Delete the third argument of all CJUMPS and remove any unused labels.

void GreedyReordering::fixJumps() {
  for (auto bb : reorderResult) {
    std::vector<std::shared_ptr<tir::Stmt>> stmts = bb->getStmts();
    for (int i = 0; i < stmts.size(); ++i) {
      if (auto cjump = std::dynamic_pointer_cast<tir::CJump>(stmt)) {
        // CJUMP statements where the “true” case jumps to the next statement
        // should have their condition inverted so the “false” case becomes the
        // fall-through.
        std::string trueLabel = cjump->getTrueLabel();
        bool needInvert = false;
        if (i < stmts.size() - 1) {
          auto label = std::dynamic_pointer_cast<tir::Label>(stmts[i + 1]);
          needInvert = (label && label->getName() == trueLabel);
        }
        if (needInvert) {
          std::shared_ptr<tir::Expr> condition = cjump->getCondition(stmt);
          std::shared_ptr<tir::Expr> inverted =
              tir::BinOp::makeNegate(condition);
          cjump->setCondition(inverted);
          // delete the third argument
          cjump->deleteFalseLabel();
        }
      } else if (auto jump = std::dynamic_pointer_cast<tir::Jump>(stmt)) {
        // Unconditional jumps to the very next basic block should be deleted
      }
    }
  }
}

} // namespace codegen