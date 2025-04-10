#pragma once

#include "tir/TIR.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace codegen {

class BasicBlock {
public:
  BasicBlock(int id, std::shared_ptr<tir::Stmt> startStmt = nullptr) : id{id} {
    if (startStmt) {
      stmts.push_back(startStmt);
    }
  }

  std::vector<std::shared_ptr<tir::Stmt>> getStmts() const { return stmts; }

  void addStmt(std::shared_ptr<tir::Stmt> stmt) { stmts.push_back(stmt); }

  std::shared_ptr<tir::Stmt> getFirstStmt() const { return stmts.front(); }

  std::shared_ptr<tir::Stmt> getLastStmt() const { return stmts.back(); }

  std::vector<std::shared_ptr<BasicBlock>> getPredecessors() const {
    return predecessors;
  }

  std::vector<std::shared_ptr<BasicBlock>> getSuccessors() const {
    return successors;
  }

  void addPredecessor(std::shared_ptr<BasicBlock> bb) {
    predecessors.push_back(bb);
  }

  void addSuccessor(std::shared_ptr<BasicBlock> bb) {
    successors.push_back(bb);
  }

private:
  int id;
  std::vector<std::shared_ptr<tir::Stmt>> stmts;
  std::vector<std::shared_ptr<BasicBlock>> predecessors;
  std::vector<std::shared_ptr<BasicBlock>> successors;
};

class CFG {
public:
  void addNode(std::shared_ptr<BasicBlock> node) {
    nodes.insert({node->getId(), node});
  }

  int getNumNodes() const { return nodes.size(); }

  void addEdge(std::shared_ptr<BasicBlock> src,
               std::shared_ptr<BasicBlock> dst) {
    if (!src) {
      std::cout << "CFG::addEdge: src is null" << std::endl;
      return;
    }
    if (!edge) {
      std::cout << "CFG::addEdge: dst is null" << std::endl;
      return;
    }
    src->addSuccessor(dst);
    dst->addPredecessor(src);
  }

private:
  std::unordered_map<int, std::shared_ptr<BasicBlock>> nodes;
};

} // namespace codegen
