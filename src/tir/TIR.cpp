#include "tir/TIR.hpp"

namespace tir {

int Label::numLabels = 0;
int Temp::numTemps = 0;

std::shared_ptr<Expr> Call::makeMalloc(std::shared_ptr<Expr> arg) {
  std::vector<std::shared_ptr<Expr>> args;
  args.push_back(arg);
  return makeExpr(Name::makeMalloc(), nullptr, args);
}

std::shared_ptr<Expr> Call::makeException() {
  return makeExpr(Name::makeException(), nullptr, {});
}

std::shared_ptr<Expr> BinOp::makeNegate(std::shared_ptr<Expr> negated) {
  return makeExpr(BinOp::OpType::SUB, Const::makeExpr(0), negated);
}

// Expressions
std::ostream &BinOp::print(std::ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(BinOp " << magic_enum::enum_name(op) << "\n";
  printIndent(os, indent + 1);
  os << "lhs: { \n";
  left->print(os, indent + 2);
  printIndent(os, indent + 1);
  os << "}\n";
  printIndent(os, indent + 1);
  os << "rhs: { \n";
  right->print(os, indent + 2);
  printIndent(os, indent + 1);
  os << "}\n";
  printIndent(os, indent);
  os << ")\n";
  return os;
}

std::ostream &Call::print(std::ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(Call \n";
  printIndent(os, indent + 1);
  os << "target: { \n";
  target->print(os, indent + 2);
  printIndent(os, indent + 1);
  os << "}\n";
  printIndent(os, indent + 1);
  os << "arg: { \n";
  for (auto &arg : args) {
    arg->print(os, indent + 2);
  }
  printIndent(os, indent + 1);
  os << "}\n";
  printIndent(os, indent);
  os << ")\n";
  return os;
}

std::ostream &CJump::print(std::ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(CJump \n";
  printIndent(os, indent + 1);
  os << "condition: { \n";
  condition->print(os, indent + 2);
  printIndent(os, indent + 1);
  os << "}\n";
  printIndent(os, indent + 1);
  os << "trueLabel: " << trueLabel << " falseLabel: " << falseLabel << "\n";
  printIndent(os, indent);
  os << ")\n";
  return os;
}

} // namespace tir
