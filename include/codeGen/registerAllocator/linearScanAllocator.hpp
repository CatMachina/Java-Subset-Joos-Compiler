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
  void testAllocateWithSpilling();
  void testDoubleSpillReads();
  void testSpillReadWriteAndRead();
  void testHeavySpillingArithmetic();

private:
  // Live Variable Analysis

  // Mapping from variables to the list of locations before which variable is
  // live-out
  std::unordered_map<std::string, std::vector<int>> liveOutBeforeMap;

  std::unordered_map<std::string, std::shared_ptr<LiveInterval>>
      liveIntervalMap;

  void addLiveInterval(std::string reg,
                       std::shared_ptr<LiveInterval> interval) {
    // std::cout << "addLiveInterval: " << interval->toString() << std::endl;
    liveIntervalMap.insert({reg, interval});
  }

  std::shared_ptr<LiveInterval> getLiveInterval(std::string reg) {
    if (!liveIntervalMap.contains(reg)) {
      return nullptr;
    }
    return liveIntervalMap[reg];
  }

  // Register Allocation

  const std::unordered_set<std::string> availableRegisters = {
      assembly::R32_EBX, assembly::R32_EDX, assembly::R32_ESI,
      assembly::R32_EDI};

  std::unordered_set<std::string> freeRegisters = {
      assembly::R32_EBX, assembly::R32_EDX, assembly::R32_ESI,
      assembly::R32_EDI};

  std::unordered_map<std::string, std::string> virtualToGPR;

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
    liveOutBeforeMap.clear();
    liveIntervalMap.clear();
    freeRegisters = availableRegisters;
    virtualToGPR.clear();
    intervals.clear();
    activeIntervals.clear();
    registerOffsets.clear();
  }

  bool hasFreeRegister() const { return !freeRegisters.empty(); }

  std::string allocateFreeRegister() {
    std::string reg = *freeRegisters.begin();
    freeRegisters.erase(reg);
    // std::cout << "allocateFreeRegister: " << reg << std::endl;
    return reg;
  }

  void markAsInUse(std::string reg) {
    // std::cout << "markAsInUse: " << reg << std::endl;
    freeRegisters.erase(reg);
  }

  void markAsFree(std::string reg) {
    // std::cout << "markAsFree: " << reg << std::endl;
    freeRegisters.insert(reg);
  }

  void activate(std::shared_ptr<LiveInterval> interval) {
    // std::cout << "activate: " << interval->toString() << std::endl;
    activeIntervals.insert(interval);
  }

  void deactivate(std::shared_ptr<LiveInterval> interval) {
    // std::cout << "deactivate: " << interval->toString() << std::endl;
    activeIntervals.erase(interval);
  }

  void replaceRegisters(
      std::vector<std::shared_ptr<assembly::Instruction>> &instructions);

  // Spilling to Stack

  const std::string SPILL_REG = assembly::R32_ECX;
  const std::string SCRATCH_REG = assembly::R32_EAX;

  void populateOffsets(
      std::vector<std::shared_ptr<assembly::Instruction>> &instructions);

  void spillToStack(
      std::vector<std::shared_ptr<assembly::Instruction>> &instructions);
};

} // namespace codegen
