#include <string>
#include <unordered_set>

namespace static_check {
class LiveVariableAnalysisInfo {
  // variables that are used at node n
  std::unordered_set<std::string> _use;

  // variables that are written at node n
  std::unordered_set<std::string> _def;

  // variables live on entry
  std::unordered_set<std::string> _in;

  // variables live on exit
  std::unordered_set<std::string> _out;

public:
  LiveVariableAnalysisInfo() {
    _use = std::unordered_set<std::string>();
    _def = std::unordered_set<std::string>();
    _in = std::unordered_set<std::string>();
    _out = std::unordered_set<std::string>();
  }
  std::unordered_set<std::string> &use() { return _use; }
  std::unordered_set<std::string> &def() { return _def; }
  std::unordered_set<std::string> &in() { return _in; }
  std::unordered_set<std::string> &out() { return _out; }
};
} // namespace static_check