#pragma once

#include "tir/Stmt.hpp"
#include <memory>
#include <vector>

namespace tir {

class Seq : public Stmt {
private:
  std::vector<std::shared_ptr<Stmt>> stmts;
  bool replaceParent;

public:
  Seq(std::vector<std::shared_ptr<Stmt>> stmts, bool replaceParent = false)
      : stmts(filterNulls(stmts)), replaceParent(replaceParent) {}

  static std::vector<std::shared_ptr<Stmt>>
  filterNulls(const std::vector<std::shared_ptr<Stmt>> &list) {
    std::vector<std::shared_ptr<Stmt>> filtered;
    for (const auto &stmt : list) {
      if (stmt) {
        filtered.push_back(stmt);
      }
    }
    return filtered;
  }

  std::vector<std::shared_ptr<Stmt>> getStmts() const { return stmts; }

  std::string label() const override { return "SEQ"; }

  void visitChildren(InsnMapsBuilder &v) {
    for (auto stmt : stmts) {
      v.visit(stmt);
    }
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Seq\n";
    for (const auto &stmt : stmts) {
      stmt->print(os, indent + 1);
    }
    printIndent(os, indent);
    os << ")\n";
    return os;
  }

  std::vector<std::shared_ptr<Node>> getChildren() const override {
    std::vector<std::shared_ptr<Node>> children;
    for (const auto &stmt : stmts) {
      children.push_back(stmt);
    }
    return children;
  }
};

} // namespace tir