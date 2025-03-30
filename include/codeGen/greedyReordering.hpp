#pragma once

#include "codeGen/cfg.hpp"
#include <memory>
#include <unordered_set>
#include <vector>

namespace codegen {

class GreedyReordering {
public:
  GreedyReordering(std::shared_ptr<CFG> cfg) : cfg{cfg} {}
  void run();

private:
  std::shared_ptr<CFG> cfg;
  std::unordered_set<std::shared_ptr<BasicBlock>> unmarked;
  std::vector<std::shared_ptr<BasicBlock>> reorderResult;
};

} // namespace codegen