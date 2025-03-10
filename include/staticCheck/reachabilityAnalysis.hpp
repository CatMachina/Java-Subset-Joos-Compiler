#pragma once

#include "cfgBuilder.hpp"

namespace static_check {

// TODO
class ReachabilityAnalysisInfo {
  // program point before n may be reached: true/false
  bool in = false;

  // program point after n may be reached: true/false
  bool out = false;
};

// TODO
class ReachabilityAnalysis {};

} // namespace static_check
