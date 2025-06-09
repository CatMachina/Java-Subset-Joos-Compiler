#include "codeGen/registerAllocator/linearScanAllocator.hpp"

namespace codegen {

//////////////////// Helpers ////////////////////

static void printInstructions(
    const std::vector<std::shared_ptr<assembly::Instruction>> instructions) {
  for (auto instruction : instructions) {
    std::cout << instruction->toString() << std::endl;
  }
}

//////////////////// Live Variable Analysis ////////////////////

// Each variable (that is, temporary) is live at a subset of locations in the
// code—its live range; however, to ensure that each variable is assigned to a
// single register, the variable is treated as live everywhere from the first
// place it is live to the last. This set of locations is the variable's live
// interval.
void LinearScanAllocator::runLiveVariableAnalysis(
    const std::vector<std::shared_ptr<assembly::Instruction>> &instructions) {
  std::unordered_set<std::string> regs, liveOut;
  int loc = instructions.size() - 1;
  for (auto it = instructions.rbegin(); it != instructions.rend(); ++it) {
    std::shared_ptr<assembly::Instruction> instruction = *it;
    // std::cout << "=== Instruction " << instruction->toString() << std::endl;
    std::unordered_set<std::string> use, def, liveIn;
    // Populate use and def
    for (auto reg : instruction->getReadRegisters()) {
      use.insert(reg);
    }
    for (auto reg : instruction->getWriteRegisters()) {
      def.insert(reg);
    }
    for (auto reg : instruction->getAllUsedRegisters()) {
      if (liveOutBeforeMap.find(reg) == liveOutBeforeMap.end()) {
        liveOutBeforeMap[reg] = std::vector<int>();
      }
      regs.insert(reg);
    }
    // std::cout << "Use: ";
    for (auto reg : use) {
      // std::cout << reg << ", ";
    }
    // std::cout << "\nDef: ";
    for (auto reg : def) {
      // std::cout << reg << ", ";
    }
    // std::cout << std::endl;
    // Exclude def
    for (auto reg : liveOut) {
      if (def.find(reg) == def.end()) {
        liveIn.insert(reg);
      }
    }
    // Union with use
    for (auto reg : use) {
      liveIn.insert(reg);
    }
    // std::cout << "Regs in Live In: " << std::endl;
    for (auto reg : liveIn) {
      // std::cout << reg << ": ";
      liveOutBeforeMap[reg].push_back(loc);
      for (auto loc : liveOutBeforeMap[reg]) {
        // std::cout << loc << ", ";
      }
      // std::cout << std::endl;
    }
    liveOut = liveIn;
    loc--;
    // std::cout << "Live In (= Live Out Before): " << std::endl;
    for (auto reg : liveOut) {
      // std::cout << reg << ", ";
    }
    // std::cout << std::endl;
  }
  // Compute the live intervals for each variable
  std::shared_ptr<LiveInterval> interval;
  for (auto reg : regs) {
    // Pushed locs in reverse instruction order, so back() is earliest and
    // front() is latest.
    if (liveOutBeforeMap[reg].empty()) {
      // std::cout << "liveOutBefore is empty? " << reg << std::endl;
      continue;
    }
    int begin = liveOutBeforeMap[reg].back();
    int end = liveOutBeforeMap[reg].front();
    interval = std::make_shared<LiveInterval>(LiveInterval{reg, begin, end});
    addLiveInterval(reg, interval);
  }
  // std::cout << "Linear Scan: Done Live Variable Analysis" << std::endl;
}

//////////////////// Spilling to Stack ////////////////////

void LinearScanAllocator::populateOffsets(
    std::vector<std::shared_ptr<assembly::Instruction>> &instructions) {
  int offset = 4;
  for (auto &instruction : instructions) {
    for (auto &reg : instruction->getAllUsedVirtualRegisters()) {
      if (registerOffsets.find(reg) == registerOffsets.end()) {
        registerOffsets[reg] = offset;
        offset += 4;
      }
    }
  }
}

void LinearScanAllocator::spillToStack(
    std::vector<std::shared_ptr<assembly::Instruction>> &instructions) {
  populateOffsets(instructions);
  std::vector<std::shared_ptr<assembly::Instruction>> newInstructions;
  for (auto &instruction : instructions) {
    replaceVirtualRegisters(instruction, newInstructions, true);
  }
  instructions = newInstructions;
}

//////////////////// Register Allocation ////////////////////

void LinearScanAllocator::replaceRegisters(
    std::vector<std::shared_ptr<assembly::Instruction>> &instructions) {
  std::vector<std::shared_ptr<assembly::Instruction>> newInstructions;
  for (auto instruction : instructions) {
    for (auto reg : instruction->getAllUsedVirtualRegisters()) {
      if (virtualToGPR.find(reg) != virtualToGPR.end() &&
          assembly::isGPR(virtualToGPR[reg])) {
        instruction->replaceRegister(reg, virtualToGPR[reg]);
      }
    }
    auto mov = std::dynamic_pointer_cast<assembly::Mov>(instruction);
    if (!mov) {
      newInstructions.push_back(instruction);
      continue;
    }
    auto regOp0 =
        std::dynamic_pointer_cast<assembly::RegisterOp>(mov->getOperands()[0]);
    auto regOp1 =
        std::dynamic_pointer_cast<assembly::RegisterOp>(mov->getOperands()[1]);
    bool canRemove = regOp0 && regOp1 && assembly::isGPR(regOp0->getReg()) &&
                     assembly::isGPR(regOp1->getReg()) &&
                     regOp0->getReg() == regOp1->getReg();
    if (!canRemove) {
      newInstructions.push_back(instruction);
    }
  }
  instructions = newInstructions;
}

int LinearScanAllocator::allocateFor(
    std::vector<std::shared_ptr<assembly::Instruction>> &instructions) {
  init();
  // Begins with a live variable analysis
  runLiveVariableAnalysis(instructions);
  // Populate the intervals list
  for (auto pair : liveIntervalMap) {
    intervals.push_back(pair.second);
  }
  // Sort in order of their first instructions
  auto compareBegin = [](const std::shared_ptr<LiveInterval> &a,
                         const std::shared_ptr<LiveInterval> &b) {
    return a->begin < b->begin;
  };
  std::sort(intervals.begin(), intervals.end(), compareBegin);
  // Loop through the first list
  for (auto interval : intervals) {
    // std::cout << "=== " << interval->toString() << std::endl;
    // For IMul and IDiv
    if (assembly::isGPR(interval->reg)) {
      markAsInUse(interval->reg);
      activate(interval);
      continue;
    }
    // When the live interval begins with a mov instruction
    // TODO: disable for now
    /*
    if (auto mov = std::dynamic_pointer_cast<assembly::Mov>(
            instructions[interval->begin])) {
      // In the case where the source is a variable whose live interval ends
    at
      // this instruction.
      std::shared_ptr<assembly::Operand> dest = mov->getOperands()[0];
      std::shared_ptr<assembly::Operand> src = mov->getOperands()[1];
      auto srcRegOp = std::dynamic_pointer_cast<assembly::RegisterOp>(src);
      auto destRegOp = std::dynamic_pointer_cast<assembly::RegisterOp>(dest);
      if (srcRegOp && destRegOp) {
        std::shared_ptr<LiveInterval> srcInterval =
            getLiveInterval(srcRegOp->getReg());
        if (instructions[srcInterval->end] == mov) {
          // Perfrorm move coalescing, by assigning the destination of the
          // instruction the same register as the source
          destRegOp->setReg(srcInterval->reg);
        }
      }
    }
    */
    std::unordered_set<std::shared_ptr<LiveInterval>> toRemove;
    // Any other, active interval whose last instructions has been passed
    for (auto activeInterval : activeIntervals) {
      if (activeInterval->end < interval->begin) {
        // Its assigned register is made available
        std::string toFree = assembly::isGPR(activeInterval->reg)
                                 ? activeInterval->reg
                                 : virtualToGPR[activeInterval->reg];
        if (availableRegisters.find(toFree) != availableRegisters.end()) {
          markAsFree(toFree);
          // Remove from the active list
          toRemove.insert(activeInterval);
        }
      }
    }
    // Update active intervals
    for (auto interval : toRemove) {
      deactivate(interval);
    }
    // The considered interval is then allocated a free register
    if (hasFreeRegister()) {
      virtualToGPR.insert({interval->reg, allocateFreeRegister()});
      // And is added to the active list
      activate(interval);
    }
    // If no register is available to be allocated to a variable, one of the
    // variable is chosen to be spilled
    if (virtualToGPR.find(interval->reg) == virtualToGPR.end()) {
      // Heuristic: spill the active interval with the last end time
      std::shared_ptr<LiveInterval> lastEndInterval = *activeIntervals.rbegin();
      // std::cout << "lastEndInterval: " << lastEndInterval->toString()
      // << std::endl;
      std::string gpr = virtualToGPR[lastEndInterval->reg];
      virtualToGPR.erase(lastEndInterval->reg);
      deactivate(lastEndInterval);
      // Allocate register to the current interval
      virtualToGPR.insert({interval->reg, gpr});
      activate(interval);
    }
  }
  std::cout << "\nVirtual to GPR:" << std::endl;
  for (auto pair : virtualToGPR) {
    std::cout << pair.first << ": " << pair.second << std::endl;
  }
  std::cout << "\nBefore replacing registers:" << std::endl;
  printInstructions(instructions);
  replaceRegisters(instructions);
  std::cout << "\nAfter replacing registers:" << std::endl;
  printInstructions(instructions);
  spillToStack(instructions);
  std::cout << "\nAfter spilling to stack:" << std::endl;
  printInstructions(instructions);
  return registerOffsets.size();
}

} // namespace codegen
