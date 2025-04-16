#include "codeGen/registerAllocator/linearScanAllocator.hpp"

namespace codegen {

// Helpers

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
    std::cout << "=== Instruction " << instruction->toString() << std::endl;
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
    std::cout << "Use: ";
    for (auto reg : use) {
      std::cout << reg << ", ";
    }
    std::cout << "\nDef: ";
    for (auto reg : def) {
      std::cout << reg << ", ";
    }
    std::cout << std::endl;
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
    std::cout << "Regs in Live In: " << std::endl;
    for (auto reg : liveIn) {
      std::cout << reg << ": ";
      liveOutBeforeMap[reg].push_back(loc);
      for (auto loc : liveOutBeforeMap[reg]) {
        std::cout << loc << ", ";
      }
      std::cout << std::endl;
    }
    liveOut = liveIn;
    loc--;
    std::cout << "Live In (= Live Out Before): " << std::endl;
    for (auto reg : liveOut) {
      std::cout << reg << ", ";
    }
    std::cout << std::endl;
  }
  // Compute the live intervals for each variable
  for (auto reg : regs) {
    // Pushed locs in reverse instruction order, so back() is earliest and
    // front() is latest.
    if (liveOutBeforeMap[reg].empty()) {
      std::cout << "liveOutBefore is empty? " << reg << std::endl;
      continue;
    }
    int begin = liveOutBeforeMap[reg].back();
    int end = liveOutBeforeMap[reg].front();
    auto interval =
        std::make_shared<LiveInterval>(LiveInterval{reg, begin, end});
    std::cout << interval->toString() << std::endl;
    addLiveInterval(reg, interval);
  }
  std::cout << "Linear Scan: Done Live Variable Analysis" << std::endl;
}

void LinearScanAllocator::populateOffset() {
  int offset = 4;
  for (auto reg : toSpill) {
    if (registerOffsets.find(reg) == registerOffsets.end()) {
      registerOffsets[reg] = offset;
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
      if (toSpill.find(regOp->getReg()) != toSpill.end()) {
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
      std::string reg = spillRegs[0]->getReg();
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
      std::string reg0 = spillRegs[0]->getReg();
      std::string reg1 = spillRegs[1]->getReg();
      bool isRead0 = true;
      bool isWrite0 = false;
      bool isRead1 = true;
      bool isWrite1 = true;
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
    std::cout << "=== " << interval->toString() << std::endl;
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
      // In the case where the source is a variable whose live interval ends at
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
        markAsFree(activeInterval->reg);
        // Remove from the active list
        toRemove.insert(activeInterval);
      }
    }
    // Update active intervals
    for (auto interval : toRemove) {
      deactivate(interval);
    }
    // The considered interval is then allocated a free register
    if (hasFreeRegister()) {
      interval->reg = allocateFreeRegister();
      // And is added to the active list
      activate(interval);
    }
    // If no register is available to be allocated to a variable, one of the
    // variable is chosen to be spilled
    if (!interval->isAllocated()) {
      // Heuristic: spill the active interval with the last end time
      std::shared_ptr<LiveInterval> lastEndInterval = *activeIntervals.rbegin();
      std::cout << "lastEndInterval: " << lastEndInterval->toString()
                << std::endl;
      std::string reg = lastEndInterval->reg;
      markAsFree(reg);
      deactivate(lastEndInterval);
      toSpill.insert(lastEndInterval->reg);
      // Allocate register to the current interval
      interval->reg = reg;
    }
  }
  std::cout << "Spill registers: ";
  for (auto reg : toSpill) {
    std::cout << reg << ", ";
  }
  std::cout << std::endl;
  return toSpill.size();
}

void LinearScanAllocator::testLiveVariableAnalysis() {
  using namespace assembly;

  auto reg = [](const std::string &name) {
    return std::make_shared<RegisterOp>(name); // always create a fresh instance
  };

  auto imm = [](int val) { return std::make_shared<ImmediateOp>(val); };

  auto mem = [&](const std::string &base, int offset = 0) {
    return std::make_shared<MemAddrOp>(base, "", offset, 1);
  };

  std::vector<std::shared_ptr<Instruction>> instructions = {
      std::make_shared<Mov>(reg("b"), reg("a")),
      std::make_shared<Add>(reg("b"), imm(2)),
      std::make_shared<IMul>(reg("b")),
      std::make_shared<Mov>(reg("c"), reg("b")),
      std::make_shared<Lea>(reg("b"), mem("c", 1)),
      std::make_shared<Mov>(reg("d"), reg("a")),
      std::make_shared<IMul>(reg("b")),
      std::make_shared<Mov>(reg("eax"), reg("d")),
      std::make_shared<Ret>()};

  runLiveVariableAnalysis(instructions);

  std::unordered_map<std::string, std::pair<int, int>> expectedIntervals = {
      {"a", {0, 5}}, {"b", {1, 6}}, {"c", {4, 4}}, {"d", {6, 7}}};

  for (const auto &[reg, expected] : expectedIntervals) {
    auto it = liveIntervalMap.find(reg);
    if (it == liveIntervalMap.end()) {
      std::cerr << "Missing interval for virtual register: " << reg << "\n";
      assert(false);
    }
    const auto &interval = it->second;
    if (interval->begin != expected.first || interval->end != expected.second) {
      std::cerr << "Incorrect interval for virtual register " << reg
                << ": expected [" << expected.first << ", " << expected.second
                << "], got [" << interval->begin << ", " << interval->end
                << "]\n";
      assert(false);
    }
  }

  std::cout << "Live variable interval test passed!\n";
}

void LinearScanAllocator::testAllocateFor() {
  using namespace assembly;

  auto reg = [](const std::string &name) {
    return std::make_shared<RegisterOp>(name); // always create a fresh instance
  };

  auto imm = [](int val) { return std::make_shared<ImmediateOp>(val); };

  auto mem = [&](const std::string &base, int offset = 0) {
    return std::make_shared<MemAddrOp>(base, "", offset, 1);
  };

  std::vector<std::shared_ptr<Instruction>> instructions = {
      std::make_shared<Mov>(reg("b"), reg("a")),
      std::make_shared<Add>(reg("b"), imm(2)),
      std::make_shared<IMul>(reg("b")),
      std::make_shared<Mov>(reg("c"), reg("b")),
      std::make_shared<Lea>(reg("b"), mem("c", 1)),
      std::make_shared<Mov>(reg("d"), reg("a")),
      std::make_shared<IMul>(reg("b")),
      std::make_shared<Mov>(reg("eax"), reg("d")),
      std::make_shared<Ret>()};

  int spilled = allocateFor(instructions);
  assert(spilled == 0); // No spilling expected with 3 virtuals (a, b, c, d)

  std::cout << "Linear scan allocation test done\n";
}

} // namespace codegen
