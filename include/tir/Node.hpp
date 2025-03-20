#pragma once

#include <memory>
#include <string>

namespace tir {

class Node {
public:
  virtual ~Node() = default;
  virtual std::string label() const = 0;
};

} // namespace tir