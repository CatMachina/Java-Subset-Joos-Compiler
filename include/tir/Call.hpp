#pragma once

#include "tir/Expr.hpp"

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

  std::shared_ptr<Expr> &getTarget() { return target; }
  std::vector<std::shared_ptr<Expr>> &getArgs() { return args; };

  int getNumArgs() { return args.size(); };
  std::string label() { return "CALL"; }

  static std::shared_ptr<Expr>
  makeExpr(std::shared_ptr<Expr> target, std::shared_ptr<Expr> _this,
           std::vector<std::shared_ptr<Expr>> args) {
    if (!_this) {
      return std::make_shared<Call>(target, args);
    }

    std::vector<std::shared_ptr<Expr>> passed_args;
    passed_args.push_back(_this);
    for (auto &arg : args) {
      passed_args.push_back(arg);
    }

    return std::make_shared<Call>(target, passed_args);
  }

  static std::shared_ptr<Expr> makeMalloc(std::shared_ptr<Expr> arg);
  static std::shared_ptr<Expr> makeException();
};

} // namespace tir
