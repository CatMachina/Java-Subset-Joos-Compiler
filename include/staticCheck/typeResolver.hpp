#pragma once

#include "ast/ast.hpp"
#include "staticCheck/envManager.hpp"
#include "staticCheck/environment.hpp"
#include <memory>
#include <stack>
#include <vector>

namespace static_check {

class TypeResolver {
public:
  TypeResolver(std::shared_ptr<parsetree::ast::ASTManager> astManager,
               std::shared_ptr<EnvManager> envManager)
      : astManager(astManager), envManager(envManager) {}
  std::shared_ptr<parsetree::ast::Type> evaluateList(
      const std::vector<std::shared_ptr<parsetree::ast::ExprNode>> &list);

  std::shared_ptr<parsetree::ast::Type>
  evaluate(const std::shared_ptr<parsetree::ast::Expr> &node) const;

  bool isAssignableTo(const std::shared_ptr<parsetree::ast::Type> &lhs,
                      const std::shared_ptr<parsetree::ast::Type> &rhs) const;

  bool isValidCast(const std::shared_ptr<parsetree::ast::Type> &exprType,
                   const std::shared_ptr<parsetree::ast::Type> &castType) const;

  void resolve() {
    for (auto ast : astManager->getASTs()) {
      resolveAST(ast);
    }
  }

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
      const std::vector<std::shared_ptr<parsetree::ast::Type>> &args) const;

  std::shared_ptr<parsetree::ast::Type> evalNewObject(
      const std::shared_ptr<parsetree::ast::ClassCreation> &op,
      const std::shared_ptr<parsetree::ast::Type> &object,
      const std::vector<std::shared_ptr<parsetree::ast::Type>> &args) const;

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

  std::shared_ptr<parsetree::ast::Type>
  evalAssignment(const std::shared_ptr<parsetree::ast::Assignment> &op,
                 const std::shared_ptr<parsetree::ast::Type> &lhs,
                 const std::shared_ptr<parsetree::ast::Type> &rhs) const;

  bool isReferenceOrArrType(std::shared_ptr<parsetree::ast::Type> type) const;

  bool isTypeString(std::shared_ptr<parsetree::ast::Type> type) const;

  std::shared_ptr<parsetree::ast::Type> popStack() {
    if (op_stack.empty()) {
      throw std::runtime_error("Popping an empty stack!");
    }
    std::shared_ptr<parsetree::ast::Type> value = op_stack.top();
    op_stack.pop();
    return value;
  }

  void resolveAST(std::shared_ptr<parsetree::ast::AstNode> node);

private:
  std::stack<std::shared_ptr<parsetree::ast::Type>> op_stack;
  std::shared_ptr<parsetree::ast::ExprOp> current_op;
  std::shared_ptr<EnvManager> envManager;
  std::shared_ptr<parsetree::ast::ASTManager> astManager;
};

} // namespace static_check
