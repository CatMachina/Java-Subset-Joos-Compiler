#pragma once

#include "tir/Stmt.hpp"
#include <memory>

namespace tir {

class Comment : public Stmt {
public:
  Comment(std::string comment) : comment(comment) {}
  std::string label() const override { return "COMMENT"; }
  std::string getComment() const { return comment; }

  void visitChildren(InsnMapsBuilder &v) override { ; }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Comment " << comment << ")\n";
    return os;
  }

  std::vector<std::shared_ptr<Node>> getChildren() const override {
    return std::vector<std::shared_ptr<Node>>{};
  }

private:
  std::string comment;
};

} // namespace tir
