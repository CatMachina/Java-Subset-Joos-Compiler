#pragma once

#include <string>
#include <unordered_map>
// #include "NodeFactory.h"

namespace tir {

class Node;

class InsnMapsBuilder {
  std::unordered_map<std::string, int> nameToIndex;
  std::unordered_map<int, std::shared_ptr<Node>> indexToInsn;
  int index = 0;

public:
  InsnMapsBuilder() {
    // super(null);
    nameToIndex = std::unordered_map<std::string, int>();
    indexToInsn = std::unordered_map<int, std::shared_ptr<Node>>();
  }

  std::unordered_map<std::string, int> getNameToIndex() { return nameToIndex; }

  std::unordered_map<int, std::shared_ptr<Node>> getIndexToInsn() {
    return indexToInsn;
  }

  std::shared_ptr<Node> visit(std::shared_ptr<Node> n);

  void addInsn(std::shared_ptr<Node> n) {
    indexToInsn[index] = n;
    index++;
  }

  void addNameToCurrentIndex(std::string name) {
    if (nameToIndex.count(name))
      throw std::runtime_error("Error - encountered duplicate name " + name +
                               " in the IR tree -- go fix the generator.");
    nameToIndex[name] = index;
  }
};

} // namespace tir