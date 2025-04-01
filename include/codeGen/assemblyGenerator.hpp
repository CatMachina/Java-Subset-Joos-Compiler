#pragma once

#include "codeGen/linkingResolver.hpp"

namespace codegen {

class AssembyGenerator {
  std::shared_ptr<InstructionSelector> instructionSelector;

  std::shared_ptr<LinkingResolver> linkingResolver;
  std::shared_ptr<tir::CompUnit> root;

  std::vector<std::pair<std::string, std::list<AssemblyInstruction>>>
      staticFields;
  std::vector<std::list<AssemblyInstruction>> startInstructions;

public:
  AssembyGenerator(std::shared_ptr<CodeGenLabels> codeGenLabels) {
    instructionSelector = std::make_shared<InstructionSelector>(codeGenLabels);
  }

  void generateIRTree(std::shared_ptr<tir::CompUnit> irTree, int fileId) {
    if (!irTree)
      throw std::runtime_error("irTree is null when resolving linking");

    // get static field stmt to dump in .data section of entrypoint
    for (auto &pair : irTree->getCanonFieldList()) {
      auto name = pair.first;
      auto initalizer = pair.second;
      if (!initalizer)
        throw std::runtime_error("Initalizer is null when resolving linking");

      linkingResolver->visitNode(initalizer);
      auto tile = instructionSelector->selectTile(initalizer);
      auto instructions = tile->getInstructions();
      staticFields.push_back(std::make_pair(name, instructions));
    }

    // get start instructions to dump in .text section of entrypoint
    for (auto &startStmt : irTree->getStartStmts()) {
      if (!startStmt)
        throw std::runtime_error("Start stmt is null when resolving linking");

      linkingResolver->visitNode(startStmt);
      auto tile = instructionSelector->selectTile(startStmt);
      auto instructions = tile->getInstructions();
      startInstructions.push_back(instructions);
    }

    // TODO: continue
  }

  void generateAssembly(std::vector<std::shared_ptr<tir::CompUnit>> irTrees) {
    std::shared_ptr<tir::CompUnit> startUp =
        std::make_shared<tir::CompUnit>("");
    linkingResolver = std::make_shared<LinkingResolver>(startUp);

    int fileId = 0;
    for (auto irTree : irTrees) {
      staticFields.clear();
      startInstructions.clear();
      root = irTree;
      generateIRTree(root, fileId++);
    }
  }
};

} // namespace codegen