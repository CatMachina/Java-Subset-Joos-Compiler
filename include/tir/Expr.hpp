#pragma once

#include "tir/Node.hpp"

namespace tir {

class Expr : public Node {

public:
  virtual bool isConstant() const { return false; }
};

} // namespace tir