#include "codeGen/registerAllocator/linearScanAllocator.hpp"

namespace codegen {

// Helpers

static bool compareBegin(const std::shared_ptr<LiveInterval> &a,
                         const std::shared_ptr<LiveInterval> &b) {
  return a->begin < b->begin;
}

static bool compareEnd(const std::shared_ptr<LiveInterval> &a,
                       const std::shared_ptr<LiveInterval> &b) {
  return a->end < b->end;
}

// Each variable (that is, temporary) is live at a subset of locations in the
// code—its live range; however, to ensure that each variable is assigned to a
// single register, the variable is treated as live everywhere from the first
// place it is live to the last. This set of locations is the variable's live
// interval.
void LinearScanAllocator::runLiveVariableAnalysis(
    const std::vector<std::shared_ptr<assembly::Instruction>> &instructions) {
  std::unordered_set<std::shared_ptr<assembly::RegisterOp>> operands;
  std::unordered_set<std::shared_ptr<assembly::RegisterOp>> liveOut;
  int loc = instructions.size() - 1;
  for (auto it = instructions.rbegin(); it != instructions.rend(); ++it) {
    std::shared_ptr<assembly::Instruction> instruction = *it;
    std::unordered_set<std::shared_ptr<assembly::RegisterOp>> use;
    std::unordered_set<std::shared_ptr<assembly::RegisterOp>> def;
    for (auto operand : instruction->getOperands()) {
      if (auto registerOp =
              std::dynamic_pointer_cast<assembly::RegisterOp>(operand)) {
        if (registerOp->isRead()) {
          use.insert(registerOp);
        }
        if (registerOp->isWrite()) {
          def.insert(registerOp);
        }
        operands.insert(registerOp);
      }
    }
    std::unordered_set<std::shared_ptr<assembly::RegisterOp>> liveIn;
    for (auto operand : liveOut) {
      if (!def.contains(operand)) {
        liveIn.insert(operand);
      }
    }
    for (auto operand : use) {
      liveIn.insert(operand);
    }
    for (auto operand : liveIn) {
      liveOutBeforeMap[operand].push_back(loc);
    }
    liveOut = liveIn;
    loc--;
  }
  // Now store the live intervals for each variable
  for (auto operand : operands) {
    int begin = liveOutBeforeMap[operand].back();
    int end = liveOutBeforeMap[operand].front();
    auto interval = std::make_shared<LiveInterval>(
        LiveInterval{operand, begin, end, false});
    addLiveInterval(operand, interval);
  }
}

int LinearScanAllocator::allocateFor(
    std::vector<std::shared_ptr<assembly::Instruction>> &instructions) {
  // Begins with a live variable analysis
  runLiveVariableAnalysis(instructions);
  // Sort the live intervals in order of their first instruction
  std::vector<std::shared_ptr<LiveInterval>> intervals;
  for (auto pair : liveIntervalMap) {
    intervals.push_back(pair.second);
  }
  std::sort(intervals.begin(), intervals.end(), compareBegin);
  // List of "active" live intervals; initially empty
  std::set<std::shared_ptr<LiveInterval>, decltype(compareEnd) *>
      activeIntervals;
  int numSpilled = 0;
  // Loop through the first list
  for (auto interval : intervals) {
    // When the live interval begins with a mov instruction, we look for the
    // opportunity to do move coalescing, by assigning the destination of the
    // instruction the same register as the source in the case where the source
    // is a variable whose live interval ends at this instruction.
    if (auto mov = std::dynamic_pointer_cast<assembly::Mov>(
            instructions[interval->begin])) {
      std::shared_ptr<assembly::Operand> src = mov->getOperands()[1];
      auto srcRegister = std::dynamic_pointer_cast<assembly::RegisterOp>(src);
      if (srcRegister) {
        std::shared_ptr<LiveInterval> srcInterval =
            getLiveInterval(srcRegister);
        if (instructions[srcInterval->end] == mov) {
          interval->setReg(srcInterval->getReg());
        }
      }
    }
    // Any other, active interval whose last instructions has been passed
    std::vector<std::shared_ptr<LiveInterval>> toAdd;
    std::vector<std::shared_ptr<LiveInterval>> toRemove;
    for (auto activeInterval : activeIntervals) {
      if (activeInterval->end < interval->begin) {
        // Its assigned register is made available
        setFreeRegister(activeInterval->getReg());
        // Remove from the active list;
        toRemove.push_back(activeInterval);
      }
      // The considered interval is then allocated a free register
      interval->setReg(getFreeRegister());
      // And is added to the active list
      toAdd.push_back(interval);
    }
    // If no register is available to be allocated to a variable, one of the
    // variable is chosen to be spilled
    if (!interval->isAllocated) {
      // Heuristic: spill the active interval with the last end time
      std::shared_ptr<LiveInterval> lastEndInterval = *activeIntervals.rbegin();
      std::string freeRegister = lastEndInterval->getReg();
      setFreeRegister(freeRegister);
      toRemove.push_back(lastEndInterval);
      // TODO: implement
      spillToStack(lastEndInterval->registerOp);
      numSpilled++;
      // Allocate register to the current interval;
      interval->setReg(freeRegister);
      toAdd.push_back(interval);
    }
  }
  return numSpilled;
}

} // namespace codegen
