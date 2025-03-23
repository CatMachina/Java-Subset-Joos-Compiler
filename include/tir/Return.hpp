#pragma once

#include "tir/Expr.hpp"
#include "tir/Stmt.hpp"
#include "tir/IRVisitor.hpp"
#include "tirAggregateVisitor.hpp"

#include <string>

namespace tir {
class Return : public Stmt {
protected:
  Expr *ret;

public:
  Return(Expr *ret) { this->ret = ret; }

  Expr *ret() const { return ret; }

  std::string label() const override { return "RETURN"; }

  Node *visitChildren(IRVisitor *v) override {
    bool modified = false;

    Expr *newExpr = dynamic_cast<Expr *>(v->visit(this, ret));
    if (newExpr != ret)
      modified = true;
    Expr *result = newExpr;

    if (modified)
      return v->nodeFactory()->IRReturn(result);

    return this;
  }

  template <typename T> T aggregateChildren(AggregateVisitor<T> *v) {
    T result = v->unit();
    result = v->bind(result, v->visit(ret));
    return result;
  }
}
}; // namespace tir
