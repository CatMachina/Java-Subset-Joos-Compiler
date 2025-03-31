#pragma once

#include "codeGen/instructionSelector.hpp"

#include <list>
#include <unordered_set>

namespace codegen {

class LinkingResolver {
  std::unordered_set<std::string> requiredStaticFields;
  std::unordered_set<std::string> requiredMethods;

  std::shared_ptr<tir::CompUnit> root;
};

} // namespace codegen
