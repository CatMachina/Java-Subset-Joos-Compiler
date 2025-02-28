#pragma once

#include "ast/ast.hpp"
#include <memory>
#include <vector>

namespace static_check {

class TypeResolver {
public:
  std::shared_ptr<parsetree::ast::Type>
  evaluateList(const std::vector<parsetree::ast::ExprNode> &list) const;

  std::shared_ptr<parsetree::ast::Type>
  evaluate(const std::shared_ptr<parsetree::ast::Expr> &node) const;

  bool isAssignableTo(const std::shared_ptr<parsetree::ast::Type> &lhs,
                      const std::shared_ptr<parsetree::ast::Type> &rhs) const;

  bool isValidCast(const std::shared_ptr<parsetree::ast::Type> &exprType,
                   const std::shared_ptr<parsetree::ast::Type> &castType) const;

private:
  std::shared_ptr<parsetree::ast::Type>
  evalBinOp(const std::shared_ptr<parsetree::ast::BinOp> &op,
            const std::shared_ptr<parsetree::ast::Type> &lhs,
            const std::shared_ptr<parsetree::ast::Type> &rhs) const;

  std::shared_ptr<parsetree::ast::Type>
  evalUnOp(const std::shared_ptr<parsetree::ast::UnOp> &op,
           const std::shared_ptr<parsetree::ast::Type> &rhs) const;

  std::shared_ptr<parsetree::ast::Type>
  evalFieldAccess(const std::shared_ptr<parsetree::ast::FieldAccess> &op,
                  const std::shared_ptr<parsetree::ast::Type> &lhs,
                  const std::shared_ptr<parsetree::ast::Type> &field) const;

  std::shared_ptr<parsetree::ast::Type> evalMethodInvocation(
      const std::shared_ptr<parsetree::ast::MethodInvocation> &op,
      const std::shared_ptr<parsetree::ast::Type> &method,
      const std::vector<parsetree::ast::Type> &args) const;

  std::shared_ptr<parsetree::ast::Type>
  evalNewObject(const std::shared_ptr<parsetree::ast::ClassCreation> &op,
                const std::shared_ptr<parsetree::ast::Type> &object,
                const std::vector<parsetree::ast::Type> &args) const;

  std::shared_ptr<parsetree::ast::Type>
  evalNewArray(const std::shared_ptr<parsetree::ast::ArrayCreation> &op,
               const std::shared_ptr<parsetree::ast::Type> &type,
               const std::shared_ptr<parsetree::ast::Type> &size) const;

  std::shared_ptr<parsetree::ast::Type>
  evalArrayAccess(const std::shared_ptr<parsetree::ast::ArrayAccess> &op,
                  const std::shared_ptr<parsetree::ast::Type> &array,
                  const std::shared_ptr<parsetree::ast::Type> &index) const;

  std::shared_ptr<parsetree::ast::Type>
  evalCast(const std::shared_ptr<parsetree::ast::Cast> &op,
           const std::shared_ptr<parsetree::ast::Type> &type,
           const std::shared_ptr<parsetree::ast::Type> &value) const;

  std::shared_ptr<parsetree::ast::Type> popStack() {
    if (op_stack.empty()) {
      throw std::runtime_error("Popping an empty stack!");
    }
    std::shared_ptr<parsetree::ast::Type> value = op_stack.top();
    op_stack.pop();
    return value;
  }

private:
  std::stack<std::shared_ptr<parsetree::ast::Type>> op_stack;
  std::shared_ptr<parsetree::ast::ExprOp> current_op;
};

} // namespace static_check
