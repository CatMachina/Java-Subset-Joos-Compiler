#pragma once

#include "codeGen/assembly/assembly.hpp"
#include "codeGen/registerAllocator/registerAllocator.hpp"

#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace codegen {

struct LiveInterval {
  std::shared_ptr<assembly::RegisterOp> regOp;
  int begin;
  int end;
  bool isAllocated;

  std::string getReg() const { return regOp->getReg(); }
  void setReg(std::string reg) { regOp->setReg(reg); }
};

class LinearScanAllocator : public RegisterAllocator {
public:
  void runLiveVariableAnalysis(
      const std::vector<std::shared_ptr<assembly::Instruction>> &instructions);

  int allocateFor(std::vector<std::shared_ptr<assembly::Instruction>>
                      &instructions) override;

private:
  // Live Variable Analysis

  // Mapping from variables to the list of locations before which variable is
  // live-out
  std::unordered_map<std::shared_ptr<assembly::RegisterOp>, std::vector<int>>
      liveOutBeforeMap;

  std::unordered_map<std::shared_ptr<assembly::RegisterOp>,
                     std::shared_ptr<LiveInterval>>
      liveIntervalMap;

  void addLiveInterval(std::shared_ptr<assembly::RegisterOp> op,
                       std::shared_ptr<LiveInterval> interval) {
    liveIntervalMap.insert({op, interval});
  }

  std::shared_ptr<LiveInterval>
  getLiveInterval(std::shared_ptr<assembly::RegisterOp> op) {
    if (!liveIntervalMap.contains(op)) {
      return nullptr;
    }
    return liveIntervalMap[op];
  }

  // Register Allocation

  std::unordered_set<std::string> freeRegisters = {
      assembly::R32_EBX, assembly::R32_EDX, assembly::R32_ESI,
      assembly::R32_EDI};

  bool hasFreeRegister() const { return !freeRegisters.empty(); }

  std::string getFreeRegister() {
    std::string reg = *freeRegisters.begin();
    freeRegisters.erase(reg);
    return reg;
  }

  void setFreeRegister(std::string reg) { freeRegisters.insert(reg); }

  // Spilling to Stack

  const std::string SPILL_REG = assembly::R32_ECX;

  std::unordered_set<std::shared_ptr<assembly::RegisterOp>> toSpill;

  void spillToStack(
      std::vector<std::shared_ptr<assembly::Instruction>> &instructions);
};

} // namespace codegen
