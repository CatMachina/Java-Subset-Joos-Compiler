#pragma once

#include <memory>
#include <string>

namespace tir {

class Node {
protected:
  std::ostream &printIndent(std::ostream &os, int indent = 0) const {
    for (int i = 0; i < indent; ++i) {
      os << "  ";
    }
    return os;
  }

public:
  virtual ~Node() = default;
  virtual std::string label() const = 0;
  virtual std::ostream &print(std::ostream &os, int indent = 0) const = 0;
};

} // namespace tir