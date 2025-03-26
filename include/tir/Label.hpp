#pragma once

// #include "tir/InsnMapsBuilder.hpp"
#include "tir/Stmt.hpp"

#include <string>

namespace tir {
class Label : public Stmt {
private:
  std::string name;

public:
  Label(std::string name) { this->name = name; }

  std::string getName() const { return name; }

  std::string label() const override { return "LABEL(" + name + ")"; }

  // InsnMapsBuilder *buildInsnMapsEnter(InsnMapsBuilder *v) override {
  //   v->addNameToCurrentIndex(name);
  //   return v;
  // }
};

}; // namespace tir