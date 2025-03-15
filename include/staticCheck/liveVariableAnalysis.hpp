#pragma once

#include "ast/ast.hpp"
#include "cfgBuilder.hpp"
#include <unordered_set>

namespace static_check {

class LiveVariableAnalysis {
  /*
  Note, in A4, only need to report warnings for dead assignment statements
  */
public:
  static bool checkDeadAssignments(std::shared_ptr<CFG> cfg);
};

} // namespace static_check
