#pragma once

#include "tir/Call.hpp"

namespace tir {

// For canonical IR
class CallStmt : public Stmt {
private:
  std::shared_ptr<Call> call;

public:
  explicit CallStmt(std::shared_ptr<Call> call) : call(call) {}

  std::shared_ptr<Call> getCall() const { return call; }

  std::string label() const override { return call->label(); }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    return call->print(os, indent);
  }

  std::vector<std::shared_ptr<Node>> getChildren() const override {
    std::vector<std::shared_ptr<Node>> children;
    children.push_back(call);
    return children;
  }

  void visitChildren(InsnMapsBuilder &v) override { call->visitChildren(v); }
};

} // namespace tir