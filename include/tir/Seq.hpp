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
};

} // namespace tir