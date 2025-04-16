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
  std::string reg;
  int begin;
  int end;

  std::string toString() const {
    return "Live Interval " + reg + " [" + std::to_string(begin) + ", " +
           std::to_string(end) + "]";
  }

  bool isAllocated() { return assembly::isGPR(reg); }
};

class LinearScanAllocator : public RegisterAllocator {
public:
  void runLiveVariableAnalysis(
      const std::vector<std::shared_ptr<assembly::Instruction>> &instructions);

  int allocateFor(std::vector<std::shared_ptr<assembly::Instruction>>
                      &instructions) override;

  // Tests
  // TODO: remove later
  void testLiveVariableAnalysis();
  void testAllocateFor();

private:
  // Live Variable Analysis

  // Mapping from variables to the list of locations before which variable is
  // live-out
  std::unordered_map<std::string, std::vector<int>> liveOutBeforeMap;

  std::unordered_map<std::string, std::shared_ptr<LiveInterval>>
      liveIntervalMap;

  void addLiveInterval(std::string reg,
                       std::shared_ptr<LiveInterval> interval) {
    liveIntervalMap.insert({reg, interval});
  }

  std::shared_ptr<LiveInterval> getLiveInterval(std::string reg) {
    if (!liveIntervalMap.contains(reg)) {
      return nullptr;
    }
    return liveIntervalMap[reg];
  }

  // Register Allocation

  std::unordered_set<std::string> freeRegisters = {
      assembly::R32_EBX, assembly::R32_EDX, assembly::R32_ESI,
      assembly::R32_EDI};

  struct CompareEnd {
    bool operator()(const std::shared_ptr<LiveInterval> &a,
                    const std::shared_ptr<LiveInterval> &b) const {
      return a->end < b->end;
    }
  };

  // Sorted in order of their first instructions
  std::vector<std::shared_ptr<LiveInterval>> intervals;

  // Second list of "active" live intervals, sorted in order of the intervals'
  // last instruction
  std::set<std::shared_ptr<LiveInterval>, CompareEnd> activeIntervals;

  void init() {
    intervals.clear();
    activeIntervals.clear();
    registerOffsets.clear();
    toSpill.clear();
  }

  bool hasFreeRegister() const { return !freeRegisters.empty(); }

  std::string allocateFreeRegister() {
    std::string reg = *freeRegisters.begin();
    freeRegisters.erase(reg);
    std::cout << "allocateFreeRegister: " << reg << std::endl;
    return reg;
  }

  void markAsInUse(std::string reg) {
    std::cout << "markAsInUse: " << reg << std::endl;
    freeRegisters.erase(reg);
  }

  void markAsFree(std::string reg) {
    std::cout << "markAsFree: " << reg << std::endl;
    freeRegisters.insert(reg);
  }

  void activate(std::shared_ptr<LiveInterval> interval) {
    std::cout << "activate: " << interval->toString() << std::endl;
    activeIntervals.insert(interval);
  }

  void deactivate(std::shared_ptr<LiveInterval> interval) {
    std::cout << "deactivate: " << interval->toString() << std::endl;
    activeIntervals.erase(interval);
  }

  // Spilling to Stack

  const std::string SPILL_REG = assembly::R32_ECX;

  std::unordered_set<std::string> toSpill;

  void populateOffset();

  void spillToStack(
      std::vector<std::shared_ptr<assembly::Instruction>> &instructions);
};

} // namespace codegen
