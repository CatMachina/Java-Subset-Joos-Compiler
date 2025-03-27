#pragma once

#include "ast/astNode.hpp"
#include "staticCheck/liveVariableAnalysisInfo.hpp"
#include "staticCheck/reachabilityAnalysisInfo.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

class CFGNode {
  int id;
  std::shared_ptr<parsetree::ast::Stmt> statement;
  std::shared_ptr<parsetree::ast::Expr> condition;
  std::vector<std::shared_ptr<CFGNode>> predecessors;
  std::vector<std::shared_ptr<CFGNode>> successors;
  std::shared_ptr<static_check::LiveVariableAnalysisInfo> lvaInfo;
  std::shared_ptr<static_check::ReachabilityAnalysisInfo> rsaInfo;

public:
  CFGNode(int id, std::shared_ptr<parsetree::ast::Stmt> statement,
          std::shared_ptr<parsetree::ast::Expr> condition = nullptr)
      : id{id}, statement{statement}, condition{condition},
        rsaInfo{std::make_shared<static_check::ReachabilityAnalysisInfo>()},
        lvaInfo{std::make_shared<static_check::LiveVariableAnalysisInfo>()} {}

  // Getters / Setters
  int getId() const { return id; }
  std::shared_ptr<parsetree::ast::Stmt> getStatement() const {
    return statement;
  }
  std::shared_ptr<parsetree::ast::Expr> getCondition() const {
    return condition;
  }
  bool isBranchNode() const { return condition != nullptr; }

  std::vector<std::shared_ptr<CFGNode>> &getPredecessors() {
    return predecessors;
  }
  std::vector<std::shared_ptr<CFGNode>> &getSuccessors() { return successors; }
  std::shared_ptr<CFGNode> getPredecessorAt(int i) const {
    return predecessors[i];
  }
  std::shared_ptr<CFGNode> getSuccessorAt(int i) const { return successors[i]; }

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

  std::shared_ptr<static_check::LiveVariableAnalysisInfo>
  getLiveVariableAnalysisInfo() const {
    return lvaInfo;
  }
  std::shared_ptr<static_check::ReachabilityAnalysisInfo>
  getReachabilityAnalysisInfo() const {
    return rsaInfo;
  }
};

class CFG {
  std::shared_ptr<CFGNode> entryNode;
  std::shared_ptr<CFGNode> endNode; // Sentinel node
  std::unordered_map<int, std::shared_ptr<CFGNode>> nodes;

public:
  std::shared_ptr<CFGNode> getEntryNode() const { return entryNode; }
  void setEntryNode(std::shared_ptr<CFGNode> entryNode) {
    this->entryNode = entryNode;
  }

  std::shared_ptr<CFGNode> getEndNode() const { return endNode; }
  void setEndNode(std::shared_ptr<CFGNode> endNode) { this->endNode = endNode; }

  void addNode(std::shared_ptr<CFGNode> node) {
    // std::cout << "Adding node " << node->getId() << std::endl;
    nodes.insert({node->getId(), node});
  }

  void addEdge(std::shared_ptr<CFGNode> src, std::shared_ptr<CFGNode> dst);

  std::vector<std::shared_ptr<CFGNode>> getNodes() {
    std::vector<std::shared_ptr<CFGNode>> ret;
    for (const auto &[id, node] : nodes) {
      ret.push_back(node);
    }
    return ret;
  }

  std::ostream &print(std::ostream &os);
};