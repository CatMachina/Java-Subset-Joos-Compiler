#include "tir/TIR.hpp"

namespace tir {

std::shared_ptr<Expr> Call::makeMalloc(std::shared_ptr<Expr> arg) {
  std::vector<std::shared_ptr<Expr>> args;
  args.push_back(arg);
  return makeExpr(Name::makeMalloc(), nullptr, args);
}

std::shared_ptr<Expr> Call::makeException() {
  return makeExpr(Name::makeException(), nullptr, {});
}

} // namespace tir
