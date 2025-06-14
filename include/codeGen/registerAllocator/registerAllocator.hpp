#pragma once

#include "codeGen/assembly/assembly.hpp"

namespace codegen {

// base class for register allocators
class RegisterAllocator {
protected:
  std::unordered_map<std::string, int> registerOffsets;

  // Registers stack-allocated temporaries are loaded into before being used in
  // an instruction
  std::vector<std::string> instructionRegisters = {
      assembly::R32_ECX, assembly::R32_ESI, assembly::R32_EDI};

  void replaceVirtualRegisters(
      std::shared_ptr<assembly::Instruction> &instruction,
      std::vector<std::shared_ptr<assembly::Instruction>> &newInstructions,
      bool needSave = false) {
    auto originalInstructionString = instruction->toString();
    auto readRegisters = instruction->getReadVirtualRegisters();
    auto writeRegisters = instruction->getWriteVirtualRegisters();
    auto usedRegisters = instruction->getAllUsedVirtualRegisters();

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
      if (needSave) {
        newInstructions.push_back(
            std::make_unique<assembly::Comment>("Save " + virtualToGPR[reg]));
        newInstructions.push_back(std::make_shared<assembly::Push>(
            std::make_shared<assembly::RegisterOp>(virtualToGPR[reg])));
      }
      newInstructions.push_back(
          std::make_unique<assembly::Comment>("Load from " + reg));
      newInstructions.push_back(std::make_unique<assembly::Mov>(
          std::make_shared<assembly::RegisterOp>(virtualToGPR[reg]),
          std::make_unique<assembly::MemAddrOp>(assembly::R32_EBP,
                                                -1 * registerOffsets[reg])));
      if (needSave) {
        newInstructions.push_back(std::make_unique<assembly::Comment>(
            "Restore " + virtualToGPR[reg]));
        newInstructions.push_back(std::make_shared<assembly::Pop>(
            std::make_shared<assembly::RegisterOp>(virtualToGPR[reg])));
      }
    }

    // add back the original instruction
    if (nextGPR > 0) {
      newInstructions.push_back(
          std::make_unique<assembly::Comment>(originalInstructionString));
    }
    newInstructions.push_back(instruction);

    // store for each write
    for (const auto &reg : writeRegisters) {
      if (needSave) {
        newInstructions.push_back(
            std::make_unique<assembly::Comment>("Save " + virtualToGPR[reg]));
        newInstructions.push_back(std::make_shared<assembly::Push>(
            std::make_shared<assembly::RegisterOp>(virtualToGPR[reg])));
      }
      newInstructions.push_back(
          std::make_unique<assembly::Comment>("Store to " + reg));
      newInstructions.push_back(std::make_unique<assembly::Mov>(
          std::make_unique<assembly::MemAddrOp>(assembly::R32_EBP,
                                                -1 * registerOffsets[reg]),
          std::make_shared<assembly::RegisterOp>(virtualToGPR[reg])));
      if (needSave) {
        newInstructions.push_back(std::make_unique<assembly::Comment>(
            "Restore " + virtualToGPR[reg]));
        newInstructions.push_back(std::make_shared<assembly::Pop>(
            std::make_shared<assembly::RegisterOp>(virtualToGPR[reg])));
      }
    }
  }

public:
  virtual int allocateFor(
      std::vector<std::shared_ptr<assembly::Instruction>> &instructions) = 0;
  virtual ~RegisterAllocator() = default;
};

} // namespace codegen
