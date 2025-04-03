#pragma once

#include "codeGen/assembly/assembly.hpp"

namespace codegen {

// base class for register allocators
class RegisterAllocator {
  std::unordered_map<std::string, int> registerOffsets;

  // Registers stack-allocated temporaries are loaded into before being used in
  // an instruction
  std::vector<std::string> instructionRegisters = {
      assembly::R32_ECX, assembly::R32_ESI, assembly::R32_EDI};

protected:
  void
  replaceVirtualRegisters(assembly::Instruction &instruction,
                          std::vector<assembly::Instruction> &newInstructions) {
    auto readRegisters = instruction.getReadVirtualRegisters();
    auto writeRegisters = instruction.getWriteVirtualRegisters();
    auto usedRegisters = instruction.getAllUsedVirtualRegisters();

    if (usedRegisters.size() > 3) {
      throw std::runtime_error(
          "x86 instructions can only use up to 3 registers");
    }

    size_t nextGPR = 0;
    std::unordered_map<std::string, std::string> virtualToGPR;
    for (const auto &reg : usedRegisters) {
      virtualToGPR[reg] = instructionRegisters[nextGPR++];
      instruction->replaceRegister(reg, virtualToGPR[reg]);
    }

    // load from each read
    for (const auto &reg : readRegisters) {
      newInstructions.push_back(std::make_unique<assembly::Mov>(
          virtualToGPR[reg],
          std::make_unique<assembly::MemAddrOp>(assembly::R32_EBP,
                                                -1 * registerOffsets[reg])));
    }

    // add back the original instruction
    newInstructions.push_back(instruction);

    // store for each write
    for (const auto &reg : writeRegisters) {
      newInstructions.push_back(std::make_unique<assembly::Mov>(
          std::make_unique<assembly::MemAddrOp>(assembly::R32_EBP,
                                                -1 * registerOffsets[reg]),
          virtualToGPR[reg]));
    }
  }

public:
  virtual int allocateFor(std::vector<assembly::Instruction> &instructions) = 0;
  virtual ~RegisterAllocator() = default;
};

} // namespace codegen
