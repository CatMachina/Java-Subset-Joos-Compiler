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

  // Node *visitChildren(IRVisitor *v) override {
  //   bool modified = false;

  //   std::shared_ptr<Expr> newExpr = dynamic_cast<std::shared_ptr<Expr>
  //   >(v->visit(this, ret)); if (newExpr != ret)
  //     modified = true;
  //   std::shared_ptr<Expr> result = newExpr;

  //   if (modified)
  //     return v->nodeFactory()->IRReturn(result);

  //   return this;
  // }

  // template <typename T> T aggregateChildren(AggregateVisitor<T> *v) {
  //   T result = v->unit();
  //   result = v->bind(result, v->visit(ret));
  //   return result;
  // }
};

}; // namespace tir
