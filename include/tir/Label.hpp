#pragma once

// #include "tir/InsnMapsBuilder.hpp"
#include "tir/Stmt.hpp"

#include <string>

namespace tir {
class Label : public Stmt {
private:
  std::string name;
  static int numLabels;

public:
  Label(std::string name) { this->name = name; }

  std::string getName() const { return name; }

  std::string label() const override { return "LABEL(" + name + ")"; }

  static std::string generateName(std::string prefix = "") {
    numLabels++;
    return ((prefix.empty() ? "label" : prefix) + std::to_string(numLabels));
  }

  void buildInsnMapsEnter(InsnMapsBuilder &v) override {
    v.addNameToCurrentIndex(name);
    return;
  }

  void visitChildren(InsnMapsBuilder &v) override { v.visit(nullptr); }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Label " << name << ")\n";
    return os;
  }

  std::vector<std::shared_ptr<Node>> getChildren() const override { return {}; }
};

}; // namespace tir