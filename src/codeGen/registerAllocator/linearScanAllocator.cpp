#include "codeGen/registerAllocator/linearScanAllocator.hpp"

namespace codegen {

// Each variable (that is, temporary) is live at a subset of locations in the
// code—its live range; however, to ensure that each variable is assigned to a
// single register, the variable is treated as live everywhere from the first
// place it is live to the last. This set of locations is the variable's live
// interval.
void runLiveVariableAnalysis(
    std::vector<std::shared_ptr<assembly::Instruction>> &instructions) {
  std::unordered_set<assembly::RegisterOp> liveOut;
  for (auto it = instructions.rbegin(); it != instructions.rend(); ++it) {
    std::shared_ptr<assembly::Instruction> instruction = *it;
    std::unordered_set<assembly::RegisterOp> use;
    std::unordered_set<assembly::RegisterOp> def;
    for (auto operand : instruction->getOperands()) {
      if (operand->isRead()) {
        use.insert(operand);
      } else if (operand->isWrite()) {
        def.insert(operand);
      }
    }
    std::unordered_set<assembly::RegisterOp> liveIn;
    for (auto operand : liveOut) {
      if (!def.contains(operand)) {
        liveIn.insert(operand);
      }
    }
    for (auto operand : use) {
      liveIn.insert(use);
    }
    for (auto operand : liveIn) {
      lvaInfo->addToMap(operand, instruction);
    }
    liveOut = liveIn;
  }
}

} // namespace codegen
