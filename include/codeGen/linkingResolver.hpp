#pragma once

#include "codeGen/instructionSelector.hpp"

#include <list>
#include <unordered_set>

namespace codegen {

class LinkingResolver {
  std::unordered_set<std::string> requiredStaticFields;
  std::unordered_set<std::string> requiredMethods;

  std::shared_ptr<tir::CompUnit> root;

public:
  void visitNode(std::shared_ptr<tir::Node> node) {
    if (!node)
      throw std::runtime_error("Node is null when resolving linking");

    if (auto call = std::dynamic_pointer_cast<tir::Call>(node)) {
      if (auto name = std::dynamic_pointer_cast<tir::Name>(call->getTarget())) {
        std::string methodName = name->getName();
        if (!(requiredMethods.contains(methodName) ||
              root->getFunctions().contains(methodName))) {
          requiredMethods.insert(methodName);
        }
      }

    } else if (auto temp = std::dynamic_pointer_cast<tir::Temp>(node)) {
      std::string tempName = temp->getName();
      if (!requiredStaticFields.contains(tempName) && temp->isGlobal) {
        requiredStaticFields.insert(tempName);
      }

    } else if (auto name = std::dynamic_pointer_cast<tir::Name>(node)) {
      std::string nodeName = name->getName();
      if (!(root->getFunctions().contains(nodeName)) && name->isGlobal) {
        requiredMethods.insert(nodeName);
      }
    }

    for (const auto &child : node->getChildren()) {
      if (!child)
        continue;
      visitNode(child);
    }
  }

  LinkingResolver(std::shared_ptr<tir::CompUnit> root) : root(root) {
    visitNode(root);
    requiredMethods.insert("__malloc");
    requiredMethods.insert("__exception");
  }

  std::unordered_set<std::string> getRequiredStaticFields() {
    return requiredStaticFields;
  }
  std::unordered_set<std::string> getRequiredMethods() {
    return requiredMethods;
  }
};

} // namespace codegen
