#pragma once

#include <stack>
#include <vector>

#include "ast/ast.hpp"

namespace static_check {
template <typename T> class Evaluator {
protected:
  using op_array = std::vector<T>;

  virtual T mapValue(std::shared_ptr<parsetree::ast::ExprValue> &node) = 0;

  virtual T evalBinOp(std::shared_ptr<parsetree::ast::BinOp> &op, const T lhs,
                      const T rhs) = 0;
  virtual T evalUnOp(std::shared_ptr<parsetree::ast::UnOp> &op,
                     const T rhs) = 0;
  virtual T evalFieldAccess(std::shared_ptr<parsetree::ast::FieldAccess> &op,
                            const T lhs, const T field) = 0;
  virtual T
  evalMethodInvocation(std::shared_ptr<parsetree::ast::MethodInvocation> &op,
                       const T method, const op_array &args) = 0;
  virtual T evalNewObject(std::shared_ptr<parsetree::ast::ClassCreation> &op,
                          const T object, const op_array &args) = 0;
  virtual T evalNewArray(std::shared_ptr<parsetree::ast::ArrayCreation> &op,
                         const T type, const T size) = 0;
  virtual T evalArrayAccess(std::shared_ptr<parsetree::ast::ArrayAccess> &op,
                            const T array, const T index) = 0;
  virtual T evalCast(std::shared_ptr<parsetree::ast::Cast> &op, const T type,
                     const T value) = 0;
  virtual T evalAssignment(std::shared_ptr<parsetree::ast::Assignment> &op,
                           const T lhs, const T rhs) = 0;

public:
  T evaluate(std::shared_ptr<parsetree::ast::Expr> expr) {
    auto exprNodes = expr->getExprNodes();
    return evaluateList(exprNodes);
  }

  virtual T
  evaluateList(std::vector<std::shared_ptr<parsetree::ast::ExprNode>> &list) {
    std::cout << "Evaluator::evaluateList" << std::endl;
    while (!op_stack.empty())
      popStack();

    std::cout << "Evaluating " << list.size() << " nodes\n";
    for (const auto &node : list) {
      if (auto value =
              std::dynamic_pointer_cast<parsetree::ast::ExprValue>(node)) {
        std::cout << "-->Start mapValue with stack size " << op_stack.size()
                  << std::endl;
        op_stack.push(mapValue(value));
        std::cout << "-->Pushed mapValue with stack size " << op_stack.size()
                  << std::endl;
      } else if (auto unary =
                     std::dynamic_pointer_cast<parsetree::ast::UnOp>(node)) {
        std::cout << "-->Start evalUnOp with stack size " << op_stack.size()
                  << std::endl;
        auto rhs = popStack();
        op_stack.push(evalUnOp(unary, rhs));
        std::cout << "-->Pushed evalUnOp with stack size " << op_stack.size()
                  << std::endl;
      } else if (auto binary =
                     std::dynamic_pointer_cast<parsetree::ast::BinOp>(node)) {
        std::cout << "-->Start evalBinOp with stack size " << op_stack.size()
                  << std::endl;
        auto rhs = popStack();
        auto lhs = popStack();
        op_stack.push(evalBinOp(binary, lhs, rhs));
        std::cout << "-->Pushed evalBinOp with stack size " << op_stack.size()
                  << std::endl;
      } else if (auto field =
                     std::dynamic_pointer_cast<parsetree::ast::FieldAccess>(
                         node)) {
        std::cout << "-->Start evalFieldAccess with stack size "
                  << op_stack.size() << std::endl;
        auto rhs = popStack();
        auto lhs = popStack();
        op_stack.push(evalFieldAccess(field, lhs, rhs));
        std::cout << "-->Pushed evalFieldAccess with stack size "
                  << op_stack.size() << std::endl;
      } else if (auto method = std::dynamic_pointer_cast<
                     parsetree::ast::MethodInvocation>(node)) {
        std::cout << "-->Start evalMethodInvocation with stack size "
                  << op_stack.size() << std::endl;
        // Note: reverse order
        // Q: What happens if there are 3 children (primaryExpr, id, arg)?
        std::vector<T> args;
        if (method->getNumArgs() > 1) {
          for (int i = 0; i < method->getNumArgs() - 1; ++i) {
            args.push_back(popStack());
          }
        }
        auto method_name = popStack();
        op_stack.push(evalMethodInvocation(method, method_name, args));
        std::cout << "-->Pushed evalMethodInvocation with stack size "
                  << op_stack.size() << std::endl;
      } else if (auto newObj =
                     std::dynamic_pointer_cast<parsetree::ast::ClassCreation>(
                         node)) {
        std::cout << "-->Start evalNewObject with stack size "
                  << op_stack.size() << std::endl;
        std::cout << "ClassCreation with " << newObj->getNumArgs() << " args\n";
        std::vector<T> args;
        if (newObj->getNumArgs() > 1) {
          for (int i = 0; i < newObj->getNumArgs() - 1; ++i) {
            args.push_back(popStack());
          }
        }
        auto type = popStack();
        op_stack.push(evalNewObject(newObj, type, args));
        std::cout << "-->Pushed evalNewObject with stack size "
                  << op_stack.size() << std::endl;
      } else if (auto array =
                     std::dynamic_pointer_cast<parsetree::ast::ArrayCreation>(
                         node)) {
        std::cout << "-->Start evalNewArray with stack size " << op_stack.size()
                  << std::endl;
        auto size = popStack();
        auto type = popStack();
        op_stack.push(evalNewArray(array, type, size));
        std::cout << "-->Pushed evalNewArray with stack size "
                  << op_stack.size() << std::endl;
      } else if (auto access =
                     std::dynamic_pointer_cast<parsetree::ast::ArrayAccess>(
                         node)) {
        std::cout << "-->Start evalArrayAccess with stack size "
                  << op_stack.size() << std::endl;
        auto index = popStack();
        auto array = popStack();
        op_stack.push(evalArrayAccess(access, array, index));
        std::cout << "-->Pushed evalArrayAccess with stack size "
                  << op_stack.size() << std::endl;
      } else if (auto cast =
                     std::dynamic_pointer_cast<parsetree::ast::Cast>(node)) {
        std::cout << "-->Start evalCast with stack size " << op_stack.size()
                  << std::endl;
        auto value = popStack();
        auto type = popStack();
        op_stack.push(evalCast(cast, type, value));
        std::cout << "-->Pushed evalCast with stack size " << op_stack.size()
                  << std::endl;
      } else if (auto assignment =
                     std::dynamic_pointer_cast<parsetree::ast::Assignment>(
                         node)) {
        std::cout << "-->Start evalAssignment with stack size "
                  << op_stack.size() << std::endl;
        auto rhs = popStack();
        auto lhs = popStack();
        op_stack.push(evalAssignment(assignment, lhs, rhs));
        std::cout << "-->Pushed evalAssignment with stack size "
                  << op_stack.size() << std::endl;
      } else {
        node->print(std::cout);
        throw std::runtime_error("Unknown node type in evalList!");
      }
    }
    std::cout << "Stack size: " << op_stack.size() << std::endl;
    if (op_stack.size() != 1) {
      throw std::runtime_error("Stack not empty after evaluation!");
    }
    return popStack();
  }

private:
  T popStack() {
    if (op_stack.empty()) {
      throw std::runtime_error("Popping an empty stack!");
    }
    T value = op_stack.top();
    std::cout << "Popping stack:\n";
    op_stack.pop();
    return value;
  }

private:
  std::stack<T> op_stack;
  std::shared_ptr<parsetree::ast::ExprOp> cur_op;
};
} // namespace static_check
