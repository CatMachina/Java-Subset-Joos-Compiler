#pragma once

#include "codeGen/registerAllocator/registerAllocator.hpp"

namespace codegen {

// spill to stack
class BasicAllocator : public RegisterAllocator {

public:
  int allocateFor(std::vector<assembly::Instruction> &instructions) override;
};

} // namespace codegen
