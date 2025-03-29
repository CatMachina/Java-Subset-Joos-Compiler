#pragma once

#include "codeGen/instructionSelector.hpp"

namespace codegen {

class AssembyGenerator {
  std::shared_ptr<InstructionSelector> instructionSelector;

public:
  void generateAssembly(std::vector<std::shared_ptr<tir::CompUnit>> irTrees);
};

} // namespace codegen