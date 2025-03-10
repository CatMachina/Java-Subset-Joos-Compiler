#include "staticCheck/cfg.hpp"

void CFG::addEdge(std::shared_ptr<CFGNode> src, std::shared_ptr<CFGNode> dst) {
  if (!src) {
    std::cout << "addEdge: src is null" << std::endl;
    return;
  }
  if (!dst) {
    std::cout << "addEdge: dst is null" << std::endl;
    return;
  }
  std::cout << "Adding edge from node " << src->getId() << " to node "
            << dst->getId() << std::endl;
  src->addSuccessor(dst);
  dst->addPredecessor(src);
}

std::ostream &CFG::print(std::ostream &os) {
  if (!entryNode) {
    std::cout << "No entry node. Method is empty." << std::endl;
    return os;
  }
  std::cout << "CFG has " << nodes.size() << " nodes" << std::endl;
  for (auto it : nodes) {
    auto node = it.second;
    os << "Node " << node->getId() << std::endl;
    os << "Is a branch node? " << node->isBranchNode() << std::endl;
    if (node->isBranchNode()) {
      node->getCondition()->print(os);
    }
    os << "Predecessors: ";
    for (const auto predecessor : node->getPredecessors()) {
      if (predecessor) {
        os << predecessor->getId() << " ";
      }
    }
    os << std::endl;
    os << "Successors: ";
    for (const auto successor : node->getSuccessors()) {
      if (successor) {
        os << successor->getId() << " ";
      }
    }
    os << std::endl;
  }
  return os;
}
