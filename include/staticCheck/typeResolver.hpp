#pragma once

#include "ast/ast.hpp"
#include "staticCheck/envManager.hpp"
#include "staticCheck/environment.hpp"
#include "staticCheck/evaluator.hpp"
#include <memory>
#include <stack>
#include <vector>

namespace static_check {

class TypeResolver final
    : private Evaluator<std::shared_ptr<parsetree::ast::Type>> {
public:
  TypeResolver(std::shared_ptr<parsetree::ast::ASTManager> astManager,
               std::shared_ptr<EnvManager> envManager)
      : astManager(astManager), envManager(envManager) {}
  std::shared_ptr<parsetree::ast::Type>
  EvalList(std::vector<std::shared_ptr<parsetree::ast::ExprNode>> &list) {
    std::cout << "TypeResolver::EvalList" << std::endl;
    return Evaluator<std::shared_ptr<parsetree::ast::Type>>::evaluateList(list);
  }

  std::shared_ptr<parsetree::ast::Type>
  Eval(std::shared_ptr<parsetree::ast::Expr> &node) {
    return Evaluator<std::shared_ptr<parsetree::ast::Type>>::evaluate(node);
  }

  bool isAssignableTo(const std::shared_ptr<parsetree::ast::Type> &lhs,
                      const std::shared_ptr<parsetree::ast::Type> &rhs) const;

  bool isValidCast(const std::shared_ptr<parsetree::ast::Type> &exprType,
                   const std::shared_ptr<parsetree::ast::Type> &castType) const;

  void resolve();

private:
  std::shared_ptr<parsetree::ast::Type>
  evalBinOp(std::shared_ptr<parsetree::ast::BinOp> &op,
            const std::shared_ptr<parsetree::ast::Type> lhs,
            const std::shared_ptr<parsetree::ast::Type> rhs) override;

  std::shared_ptr<parsetree::ast::Type>
  evalUnOp(std::shared_ptr<parsetree::ast::UnOp> &op,
           const std::shared_ptr<parsetree::ast::Type> rhs) override;

  std::shared_ptr<parsetree::ast::Type>
  evalFieldAccess(std::shared_ptr<parsetree::ast::FieldAccess> &op,
                  const std::shared_ptr<parsetree::ast::Type> lhs,
                  const std::shared_ptr<parsetree::ast::Type> field) override;

  std::shared_ptr<parsetree::ast::Type> evalMethodInvocation(
      std::shared_ptr<parsetree::ast::MethodInvocation> &op,
      const std::shared_ptr<parsetree::ast::Type> method,
      const std::vector<std::shared_ptr<parsetree::ast::Type>> &args) override;

  std::shared_ptr<parsetree::ast::Type> evalNewObject(
      std::shared_ptr<parsetree::ast::ClassCreation> &op,
      const std::shared_ptr<parsetree::ast::Type> object,
      const std::vector<std::shared_ptr<parsetree::ast::Type>> &args) override;

  std::shared_ptr<parsetree::ast::Type>
  evalNewArray(std::shared_ptr<parsetree::ast::ArrayCreation> &op,
               const std::shared_ptr<parsetree::ast::Type> type,
               const std::shared_ptr<parsetree::ast::Type> size) override;

  std::shared_ptr<parsetree::ast::Type>
  evalArrayAccess(std::shared_ptr<parsetree::ast::ArrayAccess> &op,
                  const std::shared_ptr<parsetree::ast::Type> array,
                  const std::shared_ptr<parsetree::ast::Type> index) override;

  std::shared_ptr<parsetree::ast::Type>
  evalCast(std::shared_ptr<parsetree::ast::Cast> &op,
           const std::shared_ptr<parsetree::ast::Type> type,
           const std::shared_ptr<parsetree::ast::Type> value) override;

  // std::shared_ptr<parsetree::ast::Type>
  // evalAssignment(std::shared_ptr<parsetree::ast::Assignment> &op,
  //                const std::shared_ptr<parsetree::ast::Type> lhs,
  //                const std::shared_ptr<parsetree::ast::Type> rhs) override;

  bool isReferenceOrArrType(std::shared_ptr<parsetree::ast::Type> type) const;

  bool isTypeString(std::shared_ptr<parsetree::ast::Type> type) const;

  void resolveAST(const std::shared_ptr<parsetree::ast::AstNode> &node);

  std::shared_ptr<parsetree::ast::Type>
  mapValue(std::shared_ptr<parsetree::ast::ExprValue> &value) override;

private:
  std::shared_ptr<EnvManager> envManager;
  std::shared_ptr<parsetree::ast::ASTManager> astManager;
};

} // namespace static_check
