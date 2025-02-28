#include "staticCheck/typeResolver.hpp"

namespace static_check {

std::shared_ptr<parsetree::ast::Type> TypeResolver::evaluateList(
    const std::vector<std::shared_ptr<parsetree::ast::ExprNode>> &list) const {
  op_stack.clear();

  for (const auto &node : list) {
    if (auto value =
            std::dynamic_pointer_cast<parsetree::ast::ExprValue>(node)) {
      op_stack.push_back(mapValue(value));
    } else if (auto unary =
                   std::dynamic_pointer_cast<parsetree::ast::UnOp>(node)) {
      auto rhs = popStack();
      op_stack.push_back(evalUnOp(unary, rhs));
    } else if (auto binary =
                   std::dynamic_pointer_cast<parsetree::ast::BinOp>(node)) {
      auto rhs = popStack();
      auto lhs = popStack();
      op_stack.push_back(evalBinOp(binary, lhs, rhs));
    } else if (auto field =
                   std::dynamic_pointer_cast<parsetree::ast::FieldAccess>(
                       node)) {
      auto rhs = popStack();
      auto lhs = popStack();
      op_stack.push_back(evalFieldAccess(field, lhs, rhs));
    } else if (auto method =
                   std::dynamic_pointer_cast<parsetree::ast::MethodInvocation>(
                       node)) {
      std::vector<std::shared_ptr<parsetree::ast::Type>> args(method->nargs() -
                                                              1);
      std::generate(args.rbegin(), args.rend(), [this] { return popStack(); });
      auto method_name = popStack();
      op_stack.push_back(evalMethodInvocation(method, method_name, args));
    } else if (auto newObj =
                   std::dynamic_pointer_cast<parsetree::ast::ClassCreation>(
                       node)) {
      std::vector<std::shared_ptr<parsetree::ast::Type>> args(newObj->nargs() -
                                                              1);
      std::generate(args.rbegin(), args.rend(), [this] { return popStack(); });
      auto type = popStack();
      op_stack.push_back(evalNewObject(newObj, type, args));
    } else if (auto array =
                   std::dynamic_pointer_cast<parsetree::ast::ArrayCreation>(
                       node)) {
      auto size = popStack();
      auto type = popStack();
      op_stack.push_back(evalNewArray(array, type, size));
    } else if (auto access =
                   std::dynamic_pointer_cast<parsetree::ast::ArrayAccess>(
                       node)) {
      auto index = popStack();
      auto array = popStack();
      op_stack.push_back(evalArrayAccess(access, array, index));
    } else if (auto cast =
                   std::dynamic_pointer_cast<parsetree::ast::Cast>(node)) {
      auto value = popStack();
      auto type = popStack();
      op_stack.push_back(evalCast(cast, type, value));
    }
  }
  if (op_stack.size() != 1) {
    throw std::runtime_error("Stack not empty after evaluation!");
  }
  return popStack();
}

std::shared_ptr<parsetree::ast::Type> TypeResolver::evaluate(
    const std::shared_ptr<parsetree::ast::Expr> &node) const {
  return evaluateList(node->getExprNodes());
}

} // namespace static_check
