#pragma once

namespace static_check {

class ReachabilityAnalysisInfo {
  // program point before n may be reached: true/false
  bool _in = false;

  // program point after n may be reached: true/false
  bool _out = false;

public:
  ReachabilityAnalysisInfo() {}
  bool in() { return _in; }
  void in(bool val) { _in = val; }
  bool out() { return _out; }
  void out(bool val) { _out = val; }
};
} // namespace static_check
