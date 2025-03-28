#pragma once

#include "tir/Expr.hpp"
// #include "tir/IRVisitor.hpp"
#include "tir/Stmt.hpp"
// #include "tirAggregateVisitor.hpp"

#include <memory>
#include <string>

namespace tir {
class Return : public Stmt {
protected:
  std::shared_ptr<Expr> ret;

public:
  Return(std::shared_ptr<Expr> ret) { this->ret = ret; }

  std::shared_ptr<Expr> getRet() const { return ret; }

  std::string label() const override { return "RETURN"; }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Return\n";
    ret->print(os, indent + 1);
    printIndent(os, indent);
    os << ")\n";
    return os;
  }

  std::vector<std::shared_ptr<Node>> getChildren() const override {
    std::vector<std::shared_ptr<Node>> children;
    children.push_back(ret);
    return children;
  }

  void visitChildren(InsnMapsBuilder &v) { v.visit(ret); }

  // template <typename T> T aggregateChildren(AggregateVisitor<T> *v) {
  //   T result = v->unit();
  //   result = v->bind(result, v->visit(ret));
  //   return result;
  // }
};

}; // namespace tir
