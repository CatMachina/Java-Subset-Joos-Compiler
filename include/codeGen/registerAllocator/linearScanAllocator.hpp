#pragma once

#include "codeGen/assembly/assembly.hpp"
#include "codeGen/registerAllocator/registerAllocator.hpp"
#include <unordered_map>
#include <unordered_set>

namespace codegen {

class LiveVariableAnalysisInfo {
public:
  bool
  isLiveOutBefore(std::shared_ptr<assembly::RegisterOp> op,
                  std::shared_ptr<assembly::Instruction> instruction) const {
    return liveIntervalMap[op].contains(instruction);
  }

  void addToMap(std::shared_ptr<assembly::RegisterOp> op,
                std::shared_ptr<assembly::Instruction> instruction) {
    liveIntervalMap[op].insert(instruction);
  };

private:
  // operand is in the live-out set before these instructions
  std::unordered_map<std::shared_ptr<assembly::RegisterOp>>,
      std::unordered_set<std::shared_ptr<assembly::Instruction>>
          liveIntervalMap;
};

class LinearScanAllocator : public RegisterAllocator {
public:
  // TODO: run live variable analysis and allocate registers
  int allocateFor(std::vector<std::shared_ptr<assembly::Instruction>>
                      &instructions) override;

private:
  std::shared_ptr<LiveVariableAnalysisInfo> lvaInfo;

  void runLiveVariableAnalysis(
      std::vector<std::shared_ptr<assembly::Instruction>> &instructions);
}

} // namespace codegen
