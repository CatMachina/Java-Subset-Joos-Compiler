#pragma once

#include "tir/Expr.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace tir {

class Call : public Expr {
protected:
  std::shared_ptr<Expr> target;
  std::vector<std::shared_ptr<Expr>> args;

public:
  Call(std::shared_ptr<Expr> target, std::vector<std::shared_ptr<Expr>> args)
      : target{target}, args{args} {}

  Call(std::shared_ptr<Expr> target, std::shared_ptr<Expr> _this,
       std::vector<std::shared_ptr<Expr>> args) {
    this->target = target;

    if (!_this) {
      this->args = args;
      return;
    }

    std::vector<std::shared_ptr<Expr>> passed_args;
    passed_args.push_back(_this);
    for (auto &arg : args) {
      passed_args.push_back(arg);
    }

    this->args = passed_args;
  }

  std::shared_ptr<Expr> &getTarget() { return target; }
  std::vector<std::shared_ptr<Expr>> &getArgs() { return args; };

  int getNumArgs() { return args.size(); };
  std::string label() const override { return "CALL"; }

  static std::shared_ptr<Expr>
  makeExpr(std::shared_ptr<Expr> target, std::shared_ptr<Expr> _this,
           std::vector<std::shared_ptr<Expr>> args) {
    if (!_this) {
      return std::make_shared<Call>(target, args);
    }

    std::vector<std::shared_ptr<Expr>> passed_args;

    // this should be first
    passed_args.push_back(_this);
    // expect arg to be reversed
    for (auto it = args.rbegin(); it != args.rend(); ++it) {
      passed_args.push_back(*it);
    }

    // std::cout << "created call with args: " << std::endl;
    // for (auto &arg : passed_args) {
    //   arg->print(std::cout);
    // }

    return std::make_shared<Call>(target, passed_args);
  }

  void visitChildren(InsnMapsBuilder &v) override {
    v.visit(target);
    for (auto arg : args) {
      v.visit(arg);
    }
  }

  static std::shared_ptr<Expr> makeMalloc(std::shared_ptr<Expr> arg);
  static std::shared_ptr<Expr> makeException();
  std::ostream &print(std::ostream &os, int indent = 0) const override;

  std::vector<std::shared_ptr<Node>> getChildren() const override {
    std::vector<std::shared_ptr<Node>> children;
    children.push_back(target);
    for (auto &arg : args) {
      children.push_back(arg);
    }
    return children;
  }
};

} // namespace tir
