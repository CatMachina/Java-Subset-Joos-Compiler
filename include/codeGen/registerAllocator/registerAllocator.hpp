#pragma once

#include "codeGen/assembly/assembly.hpp"

namespace codegen {

// base class for register allocators
class RegisterAllocator {
public:
  virtual int allocateFor(std::vector<assembly::Instruction> &instructions) = 0;
  virtual ~RegisterAllocator() = default;
};

} // namespace codegen
