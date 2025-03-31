#pragma once

#include <memory>
#include <string>
#include <vector>

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
  virtual std::string label() const = 0;
  virtual std::ostream &print(std::ostream &os, int indent = 0) const = 0;
  virtual std::vector<std::shared_ptr<Node>> getChildren() const;
};

} // namespace tir