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
  std::unordered_set<std::shared_ptr<assembly::RegisterOp>> regOps;
  std::unordered_set<std::shared_ptr<assembly::RegisterOp>> liveOut;
  int loc = instructions.size() - 1;
  for (auto it = instructions.rbegin(); it != instructions.rend(); ++it) {
    std::shared_ptr<assembly::Instruction> instruction = *it;
    std::unordered_set<std::shared_ptr<assembly::RegisterOp>> use;
    std::unordered_set<std::shared_ptr<assembly::RegisterOp>> def;
    for (auto operand : instruction->getOperands()) {
      if (auto regOp =
              std::dynamic_pointer_cast<assembly::RegisterOp>(operand)) {
        if (regOp->isRead()) {
          use.insert(regOp);
        }
        if (regOp->isWrite()) {
          def.insert(regOp);
        }
        regOps.insert(regOp);
      }
    }
    std::unordered_set<std::shared_ptr<assembly::RegisterOp>> liveIn;
    for (auto regOp : liveOut) {
      if (def.find(regOp) == def.end()) {
        liveIn.insert(regOp);
      }
    }
    for (auto regOp : use) {
      liveIn.insert(regOp);
    }
    for (auto regOp : liveIn) {
      liveOutBeforeMap[regOp].push_back(loc);
    }
    liveOut = liveIn;
    loc--;
  }
  // Now store the live intervals for each variable
  for (auto regOp : regOps) {
    // Pushed locs in reverse instruction order, so back() is earliest and
    // front() is latest.
    int begin = liveOutBeforeMap[regOp].back();
    int end = liveOutBeforeMap[regOp].front();
    auto interval =
        std::make_shared<LiveInterval>(LiveInterval{regOp, begin, end, false});
    addLiveInterval(regOp, interval);
  }
}

void LinearScanAllocator::populateOffset() {
  int offset = 4;
  for (auto regOp : toSpill) {
    if (registerOffsets.find(regOp->getReg()) == registerOffsets.end()) {
      registerOffsets[regOp->getReg()] = offset;
      offset += 4;
    }
  }
}

void LinearScanAllocator::spillToStack(
    std::vector<std::shared_ptr<assembly::Instruction>> &instructions) {
  std::vector<std::shared_ptr<assembly::Instruction>> finalInstructions;
  populateOffset();
  for (auto instruction : instructions) {
    std::vector<std::shared_ptr<assembly::RegisterOp>> spillRegs;
    for (auto operand : instruction->getOperands()) {
      auto regOp = std::dynamic_pointer_cast<assembly::RegisterOp>(operand);
      if (!regOp)
        continue;
      if (toSpill.find(regOp) != toSpill.end()) {
        spillRegs.push_back(regOp);
      }
    }
    // No register to spill
    if (spillRegs.empty()) {
      finalInstructions.push_back(instruction);
      continue;
    }
    // Prepare for the new load and store instructions
    std::vector<std::shared_ptr<assembly::Instruction>> newInstructions;
    // Simpler case: just one register to spill
    if (spillRegs.size() == 1) {
      std::shared_ptr<assembly::RegisterOp> regOp = spillRegs[0];
      std::string reg = regOp->getReg();
      // Load for read
      if (regOp->isRead()) {
        newInstructions.push_back(
            std::make_shared<assembly::Comment>("Load from " + reg));
        newInstructions.push_back(std::make_shared<assembly::Mov>(
            std::make_shared<assembly::RegisterOp>(SPILL_REG),
            std::make_unique<assembly::MemAddrOp>(assembly::R32_EBP,
                                                  -1 * registerOffsets[reg])));
      }
      // Add back the original instruction
      instruction->replaceRegister(reg, SPILL_REG);
      newInstructions.push_back(instruction);
      // Store for write
      if (regOp->isWrite()) {
        newInstructions.push_back(
            std::make_shared<assembly::Comment>("Store to " + reg));
        newInstructions.push_back(std::make_shared<assembly::Mov>(
            std::make_unique<assembly::MemAddrOp>(assembly::R32_EBP,
                                                  -1 * registerOffsets[reg]),
            std::make_shared<assembly::RegisterOp>(SPILL_REG)));
      }
    }
    // More complicated case: two registers to spill
    else if (spillRegs.size() == 2) {
      std::shared_ptr<assembly::RegisterOp> regOp0 = spillRegs[0];
      std::string reg0 = regOp0->getReg();
      std::shared_ptr<assembly::RegisterOp> regOp1 = spillRegs[1];
      std::string reg1 = regOp1->getReg();
      bool isRead0 = spillRegs[0]->isRead();
      bool isWrite0 = spillRegs[0]->isWrite();
      bool isRead1 = spillRegs[1]->isRead();
      bool isWrite1 = spillRegs[1]->isWrite();
      if (isRead0 && isRead1) {
        // Load arg0
        newInstructions.push_back(
            std::make_shared<assembly::Comment>("Load from " + reg0));
        newInstructions.push_back(std::make_shared<assembly::Mov>(
            std::make_unique<assembly::MemAddrOp>(assembly::R32_EBP,
                                                  -1 * registerOffsets[reg0]),
            std::make_shared<assembly::RegisterOp>(SPILL_REG)));
        // Perform operation between arg0 and arg1 (from memory)
        newInstructions.push_back(
            std::make_shared<assembly::Comment>("Operate directly on memory"));
        instruction->replaceRegister(reg0, SPILL_REG);
        instruction->setOperand(
            1, std::make_unique<assembly::MemAddrOp>(
                   assembly::R32_EBP, -1 * registerOffsets[reg1]));
        if (isWrite0) {
          // Store arg0
          newInstructions.push_back(
              std::make_shared<assembly::Comment>("Store to " + reg0));
          newInstructions.push_back(std::make_shared<assembly::Mov>(
              std::make_unique<assembly::MemAddrOp>(assembly::R32_EBP,
                                                    -1 * registerOffsets[reg0]),
              std::make_shared<assembly::RegisterOp>(SPILL_REG)));
        }
      }
      // TODO: any other cases?
    }
    finalInstructions.insert(finalInstructions.end(), newInstructions.begin(),
                             newInstructions.end());
  }
  instructions = std::move(finalInstructions);
}

int LinearScanAllocator::allocateFor(
    std::vector<std::shared_ptr<assembly::Instruction>> &instructions) {
  registerOffsets.clear();
  toSpill.clear();
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
      activeIntervals(compareEnd);
  // Loop through the first list
  for (auto interval : intervals) {
    // When the live interval begins with a mov instruction, we look for the
    // opportunity to do move coalescing, by assigning the destination of the
    // instruction the same register as the source in the case where the
    // source is a variable whose live interval ends at this instruction.
    if (auto mov = std::dynamic_pointer_cast<assembly::Mov>(
            instructions[interval->begin])) {
      std::shared_ptr<assembly::Operand> src = mov->getOperands()[1];
      if (auto srcRegister =
              std::dynamic_pointer_cast<assembly::RegisterOp>(src)) {
        std::shared_ptr<LiveInterval> srcInterval =
            getLiveInterval(srcRegister);
        if (instructions[srcInterval->end] == mov) {
          if (auto destRegister =
                  std::dynamic_pointer_cast<assembly::RegisterOp>(
                      mov->getOperands()[0]))
            destRegister->setReg(srcInterval->getReg());
        }
      }
    }
    std::unordered_set<std::shared_ptr<LiveInterval>> toRemove;
    // Any other, active interval whose last instructions has been passed
    for (auto activeInterval : activeIntervals) {
      if (activeInterval->end < interval->begin) {
        // Its assigned register is made available
        setFreeRegister(activeInterval->getReg());
        // Remove from the active list
        toRemove.insert(activeInterval);
      }
    }
    // Update active intervals
    for (auto interval : toRemove) {
      activeIntervals.erase(interval);
    }
    // The considered interval is then allocated a free register
    // And is added to the active list
    if (hasFreeRegister()) {
      interval->setReg(getFreeRegister());
      interval->isAllocated = true;
      activeIntervals.insert(interval);
    }
    // If no register is available to be allocated to a variable, one of the
    // variable is chosen to be spilled
    if (!interval->isAllocated) {
      // Heuristic: spill the active interval with the last end time
      std::shared_ptr<LiveInterval> lastEndInterval = *activeIntervals.rbegin();
      std::string freeRegister = lastEndInterval->getReg();
      setFreeRegister(freeRegister);
      activeIntervals.erase(lastEndInterval);
      toSpill.insert(lastEndInterval->regOp);
      // Allocate register to the current interval;
      interval->setReg(freeRegister);
    }
  }
  return toSpill.size();
}

} // namespace codegen
