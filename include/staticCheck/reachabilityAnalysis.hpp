#pragma once

#include "cfgBuilder.hpp"
#include "reachabilityAnalysisInfo.hpp"

namespace static_check {

class ReachabilityAnalysis {

public:
  static bool checkUnreachableStatements(std::shared_ptr<CFG> cfg);
  static bool
  checkFiniteLengthReturn(std::shared_ptr<CFG> cfg,
                          std::shared_ptr<parsetree::ast::MethodDecl> method);
};

} // namespace static_check
