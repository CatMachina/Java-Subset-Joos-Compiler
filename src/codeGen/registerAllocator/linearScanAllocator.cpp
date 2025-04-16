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
  std::shared_ptr<LiveInterval> interval;
  for (auto reg : regs) {
    // Pushed locs in reverse instruction order, so back() is earliest and
    // front() is latest.
    if (liveOutBeforeMap[reg].empty()) {
      std::cout << "liveOutBefore is empty? " << reg << std::endl;
      continue;
    }
    int begin = liveOutBeforeMap[reg].back();
    int end = liveOutBeforeMap[reg].front();
    interval = std::make_shared<LiveInterval>(LiveInterval{reg, begin, end});
    addLiveInterval(reg, interval);
  }
  std::cout << "Linear Scan: Done Live Variable Analysis" << std::endl;
}

//////////////////// Spilling to Stack ////////////////////

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
    std::vector<std::string> spillRegs;
    for (auto reg : instruction->getAllUsedVirtualRegisters()) {
      if (toSpill.find(reg) != toSpill.end()) {
        spillRegs.push_back(reg);
      }
    }
    // No register to spill
    if (spillRegs.empty()) {
      finalInstructions.push_back(instruction);
      continue;
    }
    // Prepare for the new load and store instructions
    std::vector<std::shared_ptr<assembly::Instruction>> newInstructions;
    std::unordered_set<std::string> reads =
        instruction->getReadVirtualRegisters();
    std::unordered_set<std::string> writes =
        instruction->getWriteVirtualRegisters();
    // Simpler case: just one register to spill
    if (spillRegs.size() == 1) {
      std::string reg = spillRegs[0];
      bool isRead = reads.contains(reg);
      bool isWrite = writes.contains(reg);
      // Load for read
      if (isRead) {
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
      if (isWrite) {
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
      std::string reg0 = spillRegs[0];
      std::string reg1 = spillRegs[1];
      bool isRead0 = reads.contains(reg0);
      bool isWrite0 = writes.contains(reg0);
      bool isRead1 = reads.contains(reg1);
      if (isRead0 && isRead1) {
        // Load arg0
        newInstructions.push_back(
            std::make_shared<assembly::Comment>("Load from " + reg0));
        newInstructions.push_back(std::make_shared<assembly::Mov>(
            std::make_shared<assembly::RegisterOp>(SPILL_REG),
            std::make_unique<assembly::MemAddrOp>(assembly::R32_EBP,
                                                  -1 * registerOffsets[reg0])));
        // Perform operation between arg0 and arg1 (from memory)
        newInstructions.push_back(
            std::make_shared<assembly::Comment>("Operate directly on memory"));
        instruction->replaceRegister(reg0, SPILL_REG);
        instruction->setOperand(
            1, std::make_unique<assembly::MemAddrOp>(
                   assembly::R32_EBP, -1 * registerOffsets[reg1]));
        newInstructions.push_back(instruction);
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
  instructions = finalInstructions;
}

//////////////////// Register Allocation ////////////////////

void LinearScanAllocator::replaceRegisters(
    std::vector<std::shared_ptr<assembly::Instruction>> &instructions) {
  for (auto instruction : instructions) {
    for (auto reg : instruction->getAllUsedVirtualRegisters()) {
      if (virtualToGPR.find(reg) != virtualToGPR.end()) {
        instruction->replaceRegister(reg, virtualToGPR[reg]);
      }
    }
  }
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
        std::string toFree = assembly::isGPR(activeInterval->reg)
                                 ? activeInterval->reg
                                 : virtualToGPR[activeInterval->reg];
        markAsFree(toFree);
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
      virtualToGPR.insert({interval->reg, allocateFreeRegister()});
      // And is added to the active list
      activate(interval);
    }
    // If no register is available to be allocated to a variable, one of the
    // variable is chosen to be spilled
    if (virtualToGPR.find(interval->reg) == virtualToGPR.end()) {
      // Heuristic: spill the active interval with the last end time
      std::shared_ptr<LiveInterval> lastEndInterval = *activeIntervals.rbegin();
      std::cout << "lastEndInterval: " << lastEndInterval->toString()
                << std::endl;
      toSpill.insert(lastEndInterval->reg);
      std::string gpr = virtualToGPR[lastEndInterval->reg];
      virtualToGPR.erase(lastEndInterval->reg);
      deactivate(lastEndInterval);
      // Allocate register to the current interval
      virtualToGPR.insert({interval->reg, gpr});
      activeIntervals.insert(interval);
    }
  }
  std::cout << "\nVirtual to GPR:" << std::endl;
  for (auto pair : virtualToGPR) {
    std::cout << pair.first << ": " << pair.second << std::endl;
  }
  std::cout << "\nSpill registers:" << std::endl;
  for (auto reg : toSpill) {
    std::cout << reg << std::endl;
  }
  replaceRegisters(instructions);
  std::cout << "\nAfter replacing registers:" << std::endl;
  printInstructions(instructions);
  spillToStack(instructions);
  std::cout << "\nAfter spilling to stack:" << std::endl;
  printInstructions(instructions);
  return toSpill.size();
}

//////////////////// Tests ////////////////////

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

void LinearScanAllocator::testAllocateWithSpilling() {
  using namespace assembly;

  auto reg = [](const std::string &name) {
    return std::make_shared<RegisterOp>(name); // fresh virtual register
  };

  auto imm = [](int val) { return std::make_shared<ImmediateOp>(val); };

  auto mem = [&](const std::string &base, int offset = 0) {
    return std::make_shared<MemAddrOp>(base, "", offset, 1);
  };

  std::vector<std::shared_ptr<Instruction>> instructions = {
      std::make_shared<Mov>(reg("r1"), imm(1)), // r1 live
      std::make_shared<Mov>(reg("r2"), imm(2)), // r1, r2 live
      std::make_shared<Mov>(reg("r3"), imm(3)), // r1, r2, r3 live
      std::make_shared<Mov>(reg("r4"), imm(4)), // r1-r4 live
      std::make_shared<Mov>(reg("r5"),
                            imm(5)), // r1-r5 live -> should trigger spilling
      std::make_shared<Add>(reg("r1"), reg("r2")),
      std::make_shared<Add>(reg("r3"), reg("r4")),
      std::make_shared<Add>(reg("r1"), reg("r5")), // use spilled r5
      std::make_shared<Mov>(reg("eax"), reg("r1")),
      std::make_shared<Ret>()};

  int spilled = allocateFor(instructions);
  assert(spilled > 0); // Expect spilling due to r1-r5 pressure

  std::cout << "Linear scan spilling test done (spilled = " << spilled << ")\n";
}

void LinearScanAllocator::testDoubleSpillReads() {
  using namespace assembly;

  auto reg = [](const std::string &name) {
    return std::make_shared<RegisterOp>(name);
  };

  auto imm = [](int val) { return std::make_shared<ImmediateOp>(val); };

  std::vector<std::shared_ptr<Instruction>> instructions = {
      std::make_shared<Mov>(reg("r1"), imm(10)), // r1 live
      std::make_shared<Mov>(reg("r2"), imm(20)), // r2 live
      std::make_shared<Mov>(reg("r3"), imm(30)), // r3 live
      std::make_shared<Mov>(reg("r4"), imm(40)), // r4 live
      std::make_shared<Mov>(reg("r5"), imm(50)), // r5 live
      std::make_shared<Mov>(reg("r6"), imm(60)), // r6 live

      // Keep r2–r5 live with dummy uses
      std::make_shared<Add>(reg("r2"), imm(0)),
      std::make_shared<Add>(reg("r3"), imm(0)),
      std::make_shared<Add>(reg("r4"), imm(0)),
      std::make_shared<Add>(reg("r5"), imm(0)),

      // Both r1 and r6 are used here → force reload if spilled
      std::make_shared<Cmp>(reg("r1"), reg("r6")),

      std::make_shared<Mov>(reg("eax"), reg("r1")), std::make_shared<Ret>()};

  int spilled = allocateFor(instructions);
  assert(spilled >= 2); // We want at least r1 and r6 to be spilled

  std::cout << "Double spill read test done (spilled = " << spilled << ")\n";
}

void LinearScanAllocator::testSpillReadWriteAndRead() {
  using namespace assembly;

  auto reg = [](const std::string &name) {
    return std::make_shared<RegisterOp>(name);
  };

  auto imm = [](int val) { return std::make_shared<ImmediateOp>(val); };

  std::vector<std::shared_ptr<Instruction>> instructions = {
      std::make_shared<Mov>(reg("r1"), imm(10)), // r1 live
      std::make_shared<Mov>(reg("r6"), imm(60)), // r6 live
      std::make_shared<Mov>(reg("r2"), imm(20)), // r2 live
      std::make_shared<Mov>(reg("r3"), imm(30)), // r3 live
      std::make_shared<Mov>(reg("r4"), imm(40)), // r4 live
      std::make_shared<Mov>(reg("r5"), imm(50)), // r5 live

      // Keep r2–r5 live longer to increase pressure
      std::make_shared<Add>(reg("r2"), imm(0)),
      std::make_shared<Add>(reg("r3"), imm(0)),
      std::make_shared<Add>(reg("r4"), imm(0)),
      std::make_shared<Add>(reg("r5"), imm(0)),

      // Force spill of both r1 (read/write) and r6 (read)
      std::make_shared<Add>(reg("r1"), reg("r6")),

      std::make_shared<Mov>(reg("eax"), reg("r1")), std::make_shared<Ret>()};

  int spilled = allocateFor(instructions);
  assert(spilled >= 2); // Ensure both r1 and r6 spilled

  std::cout << "Spill test for read+write + read done (spilled = " << spilled
            << ")\n";
}

void LinearScanAllocator::testHeavySpillingArithmetic() {
  using namespace assembly;

  auto reg = [](const std::string &name) {
    return std::make_shared<RegisterOp>(name);
  };

  auto imm = [](int val) { return std::make_shared<ImmediateOp>(val); };

  std::vector<std::shared_ptr<Instruction>> instructions = {
      std::make_shared<Mov>(reg("r1"), imm(10)), // r1 - read/write
      std::make_shared<Mov>(reg("r2"), imm(20)),
      std::make_shared<Mov>(reg("r3"), imm(30)),
      std::make_shared<Mov>(reg("r4"), imm(40)),
      std::make_shared<Mov>(reg("r5"), imm(50)),
      std::make_shared<Mov>(reg("r6"), imm(60)), // r6 - read
      std::make_shared<Mov>(reg("r7"), imm(70)), // just keep alive
      std::make_shared<Mov>(reg("r8"), imm(80)), // just keep alive

      std::make_shared<Add>(reg("r2"), imm(0)),
      std::make_shared<Add>(reg("r3"), imm(0)),
      std::make_shared<Add>(reg("r4"), imm(0)),
      std::make_shared<Add>(reg("r5"), imm(0)),
      std::make_shared<Add>(reg("r7"), imm(0)),
      std::make_shared<Add>(reg("r8"), imm(0)),

      // r1 is read+write, r6 is read
      std::make_shared<Sub>(reg("r1"), reg("r6")),

      std::make_shared<Mov>(reg("eax"), reg("r1")), std::make_shared<Ret>()};

  int spilled = allocateFor(instructions);
  assert(spilled >=
         4); // Expect at least 4 spills with 8 virtuals and 4 registers

  std::cout << "Heavy spilling arithmetic test done (spilled = " << spilled
            << ")\n";
}

} // namespace codegen
