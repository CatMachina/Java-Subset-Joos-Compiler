#pragma once

#include "codeGen/linkingResolver.hpp"
#include "codeGen/registerAllocator/registerAllocator.hpp"

namespace codegen {

class AssembyGenerator {
  std::shared_ptr<InstructionSelector> instructionSelector;

  std::shared_ptr<LinkingResolver> linkingResolver;
  std::shared_ptr<tir::CompUnit> root;

  std::vector<std::pair<std::string, std::vector<AssemblyInstruction>>>
      staticFields;
  std::vector<std::vector<AssemblyInstruction>> startInstructions;
  std::vector<AssemblyInstruction> staticInitializers;

  std::string entryMethod;
  std::shared_ptr<RegisterAllocator> registerAllocator;

  // Callee save
  std::string emitFunctionPrologue(size_t stackSize) {
    std::string prologue;
    prologue += "push " + assembly::R32_EBP + "\n";
    prologue += "mov " + assembly::R32_EBP + ", " + assembly::R32_ESP + "\n";
    prologue += "sub " + assembly::R32_ESP + ", " +
                std::to_string(4 * stackSize) + "\n";
    return prologue;
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

    // start output to file

    std::ofstream outputFile{"output/file_" + std::to_string(fileId) + ".s"};
    outputFile << "section .text\n\n";

    // global function
    for (auto &function : irTree->getFunctionList()) {
      outputFile << "global " << function->getName() << "\n";
    }
    outputFile << "\n";

    // import method & static fields (extern)
    auto dependencyResolver = std::make_shared<LinkingResolver>(irTree);
    for (auto &method : dependencyResolver->getRequiredMethods()) {
      outputFile << "extern " << method << "\n";
    }
    for (auto &staticField : dependencyResolver->getRequiredStaticFields()) {
      outputFile << "extern " << staticField << "\n";
    }
    outputFile << "\n";

    // Tiling for each method of the program
    for (auto &function : irTree->getFunctionList()) {
      StmtTile bodyTile = instructionSelector->selectTile(function->getBody());
      auto bodyInstructions = bodyTile->getInstructions();

      // label
      outputFile << function->getName() << ":\n";
      // prologue
      size_t stackSize = registerAllocator->allocateFor(bodyInstructions);
      outputFile << emitFunctionPrologue(stackSize) << "\n";
      // method body
      for (auto &instruction : bodyInstructions) {
        outputFile << instruction->toString() << "\n";
      }
      outputFile << "\n";
    }
  }

public:
  AssembyGenerator(std::shared_ptr<CodeGenLabels> codeGenLabels,
                   std::shared_ptr<RegisterAllocator> registerAllocator,
                   std::string entryMethod) {
    this->entryMethod = entryMethod;
    this->registerAllocator = registerAllocator;
    instructionSelector = std::make_shared<InstructionSelector>(codeGenLabels);
  }

  void generateAssembly(std::vector<std::shared_ptr<tir::CompUnit>> irTrees) {
    std::shared_ptr<tir::CompUnit> startUp =
        std::make_shared<tir::CompUnit>("");
    linkingResolver = std::make_shared<LinkingResolver>(startUp);

    int fileId = 0;
    for (auto irTree : irTrees) {
      root = irTree;
      generateIRTree(root, fileId++);
    }

    for (auto &[name, initializers] : staticFields) {
      staticInitializers.insert(staticInitializers.end(), initializers.begin(),
                                initializers.end());
    }
    // DV initializer
    for (auto &startInstruction : startInstructions) {
      staticInitializers.insert(staticInitializers.end(),
                                startInstruction.begin(),
                                startInstruction.end());
    }

    // entrypoint main file
    std::ofstream outputFile{"output/main.s"};

    // static (global) variables
    outputFile << "section .data\n\n";
    for (auto &[name, initializers] : staticFields) {
      outputFile << name << ": dd 0\n";
    }
    outputFile << "\n";

    // instructions sections tart
    outputFile << "section .text\n\n";
    for (auto &[name, initializers] : staticFields) {
      outputFile << "global " << name << "\n";
    }
    outputFile << "global _start\n";
    outputFile << "extern " << entryMethod << "\n";

    // method dependencies
    for (auto &method : linkingResolver->getRequiredMethods()) {
      outputFile << "extern " << method << "\n";
    }
    outputFile << "\n";

    outputFile << "_start:\n";

    // initialize all the static fields of all the compilation units in order
    int stackSize = registerAllocator->allocateFor(staticInitializers);
    outputFile << emitFunctionPrologue(stackSize) << "\n";
    for (auto &instruction : staticInitializers) {
      outputFile << instruction->toString() << "\n";
    }
    outputFile << "mov " << assembly::R32_ESP << ", " << assembly::R32_EBP
               << "\n";
    outputFile << "pop " << assembly::R32_EBP << "\n";
    outputFile << "\n";

    // call entrypoint method and execute exit() system call with return value
    // in R32_EBP
    outputFile << "call " << entryMethod << "\n";
    outputFile << "mov " << assembly::R32_EBP << ", " << assembly::R32_EAX
               << "\n";
    outputFile << "mov " << assembly::R32_EAX << ", 1\n";
    outputFile << "int 0x80\n";
  }
};

} // namespace codegen