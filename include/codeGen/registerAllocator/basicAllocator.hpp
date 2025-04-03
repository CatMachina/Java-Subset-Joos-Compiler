#pragma once

#include "codeGen/registerAllocator/registerAllocator.hpp"

namespace codegen {

// spill to stack
class BasicAllocator : public RegisterAllocator {

  int offset = 4;

  void populateOffsets(std::vector<assembly::Instruction> &instructions) {
    for (auto &instruction : instructions) {
      for (auto &reg : instruction->getAllUsedRegisters()) {
        // assign offset to all virtual registers not assigned yet
        if (!assembly::isGPR(reg) &&
            registerOffsets.find(reg) == registerOffsets.end()) {
          registerOffsets[reg] = offset;
          offset += 4;
        }
      }
    }
  }

public:
  int allocateFor(std::vector<assembly::Instruction> &instructions) override {
    populateOffsets(instructions);

    std::vector<assembly::Instruction> newInstructions;
    for (auto &instruction : instructions) {
      replaceVirtualRegisters(instruction, newInstructions);
    }
    instructions = newInstructions;

    return registerOffsets.size();
  }
};

} // namespace codegen
