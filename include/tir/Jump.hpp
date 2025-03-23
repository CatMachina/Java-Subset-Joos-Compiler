#pragma once

#include "tir/Name.hpp"
#include "tir/Stmt.hpp"
#include <memory>

namespace tir {

// From /u/cs444/pub/tir/src/joosc/ir/ast/Jump.java
class Jump : public Stmt {
private:
  std::shared_ptr<Name> target; // target usually takes a Name

public:
  Jump(std::shared_ptr<Name> target) : target(target) {}

  std::shared_ptr<Name> getTarget() { return target; }

  std::string label() const override { return "JUMP"; }
};

} // namespace tir