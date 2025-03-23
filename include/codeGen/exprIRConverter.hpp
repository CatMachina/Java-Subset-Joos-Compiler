#pragma once

#include "ast/ast.hpp"
#include "staticCheck/evaluator.hpp"
#include "tir/TIR.hpp"

namespace tir {

// placeholder, should not be in the final IR tree
class TempTIR : public Node {

public:
  enum class Type { MethodName, TypeNode };

  TempTIR(std::shared_ptr<parsetree::ast::ExprValue> &astNode, Type type)
      : astNode{astNode}, type{type} {}

  Type type;
  std::shared_ptr<parsetree::ast::ExprValue> astNode;
};

} // namespace tir

namespace codegen {

class ExprIRConverter final
    : public static_check::ExprEvaluator<std::shared_ptr<tir::Node>> {
private:
  std::shared_ptr<tir::Node>
  evalBinOp(std::shared_ptr<parsetree::ast::BinOp> &op,
            const std::shared_ptr<tir::Node> lhs,
            const std::shared_ptr<tir::Node> rhs) override;

  std::shared_ptr<tir::Node>
  evalUnOp(std::shared_ptr<parsetree::ast::UnOp> &op,
           const std::shared_ptr<tir::Node> rhs) override;

  std::shared_ptr<tir::Node>
  evalFieldAccess(std::shared_ptr<parsetree::ast::FieldAccess> &op,
                  const std::shared_ptr<tir::Node> lhs,
                  const std::shared_ptr<tir::Node> field) override;

  std::shared_ptr<tir::Node> evalMethodInvocation(
      std::shared_ptr<parsetree::ast::MethodInvocation> &op,
      const std::shared_ptr<tir::Node> method,
      const std::vector<std::shared_ptr<tir::Node>> &args) override;

  std::shared_ptr<tir::Node>
  evalNewObject(std::shared_ptr<parsetree::ast::ClassCreation> &op,
                const std::shared_ptr<tir::Node> object,
                const std::vector<std::shared_ptr<tir::Node>> &args) override;

  std::shared_ptr<tir::Node>
  evalNewArray(std::shared_ptr<parsetree::ast::ArrayCreation> &op,
               const std::shared_ptr<tir::Node> type,
               const std::shared_ptr<tir::Node> size) override;

  std::shared_ptr<tir::Node>
  evalArrayAccess(std::shared_ptr<parsetree::ast::ArrayAccess> &op,
                  const std::shared_ptr<tir::Node> array,
                  const std::shared_ptr<tir::Node> index) override;

  std::shared_ptr<tir::Node>
  evalCast(std::shared_ptr<parsetree::ast::Cast> &op,
           const std::shared_ptr<tir::Node> type,
           const std::shared_ptr<tir::Node> value) override;

  std::shared_ptr<tir::Node>
  evalAssignment(std::shared_ptr<parsetree::ast::Assignment> &op,
                 const std::shared_ptr<tir::Node> lhs,
                 const std::shared_ptr<tir::Node> rhs) override;

  std::shared_ptr<tir::Node>
  mapValue(std::shared_ptr<parsetree::ast::ExprValue> &value) override;
};

} // namespace codegen
