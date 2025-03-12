#pragma once

#include "cfgBuilder.hpp"
#include "reachabilityAnalysisInfo.hpp"

namespace static_check {

class ReachabilityAnalysis {

public:
  static bool run(std::shared_ptr<CFG> cfg);
};

} // namespace static_check
