#pragma once

#include "ast/astNode.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

class LiveVariableAnalysisInfo;
class ReachabilityAnalysisInfo;

class CFGNode {
  int id;
  std::shared_ptr<parsetree::ast::Stmt> stmt;
  std::shared_ptr<parsetree::ast::Expr> condition;
  std::vector<std::shared_ptr<CFGNode>> predecessors;
  std::vector<std::shared_ptr<CFGNode>> successors;
  std::shared_ptr<LiveVariableAnalysisInfo> lvaInfo;
  std::shared_ptr<ReachabilityAnalysisInfo> rsaInfo;

public:
  CFGNode(int id, std::shared_ptr<parsetree::ast::Stmt> stmt,
          std::shared_ptr<parsetree::ast::Expr> conditon = nullptr)
      : id{id}, stmt{stmt}, condition{condition} {}

  // Getters / Setters
  int getId() const { return id; }
  std::shared_ptr<parsetree::ast::Stmt> getStmt() const { return stmt; }
  std::shared_ptr<parsetree::ast::Expr> getCondition() const {
    return condition;
  }
  bool isBranchNode() const { return condition != nullptr; }

  std::shared_ptr<CFGNode> getPredecessor(int i) const {
    return predecessors[i];
  }
  std::shared_ptr<CFGNode> getSuccessor(int i) const { return successors[i]; }

  bool hasPredecessor() const { return !predecessors.empty(); }
  bool hasSuccessor() const { return !successors.empty(); }

  int numPredecessors() const { return predecessors.size(); }
  int numSuccessors() const { return successors.size(); }

  void addPredecessor(std::shared_ptr<CFGNode> node) {
    predecessors.push_back(node);
  }
  void addSuccessor(std::shared_ptr<CFGNode> node) {
    successors.push_back(node);
  }

  std::shared_ptr<LiveVariableAnalysisInfo>
  getLiveVariableAnalysisInfo() const {
    return lvaInfo;
  }
  std::shared_ptr<ReachabilityAnalysisInfo>
  getReachabilityAnalysisInfo() const {
    return rsaInfo;
  }
};

class CFG {
  std::shared_ptr<CFGNode> entryNode;
  std::unordered_map<int, std::shared_ptr<CFGNode>> nodes;

public:
  CFG(std::shared_ptr<CFGNode> entryNode) : entryNode{entryNode} {}

  std::shared_ptr<CFGNode> getEntryNode() const { return entryNode; }
  void setEntryNode(std::shared_ptr<CFGNode> entryNode) {
    this->entryNode = entryNode;
  }

  void addNode(std::shared_ptr<CFGNode> node) {
    nodes.insert({node->getId(), node});
  }

  void addEdge(std::shared_ptr<CFGNode> src, std::shared_ptr<CFGNode> dest) {
    if (!src) {
      std::cout << "addEdge: src is null" << std::endl;
      return;
    }
    if (!dest) {
      std::cout << "addEdge: dest is null" << std::endl;
      return;
    }
    src->addSuccessor(dest);
    dest->addPredecessor(src);
  }
};
