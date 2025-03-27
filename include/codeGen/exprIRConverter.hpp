#pragma once

#include "ast/ast.hpp"
#include "ast/astManager.hpp"
#include "codeGen/codeGenLables.hpp"
#include "staticCheck/evaluator.hpp"
#include "tir/TIR.hpp"

namespace tir {

// placeholder, should not be in the final IR tree
class TempTIR : public Expr {

public:
  enum class Type { MethodName, TypeNode, FieldAccess };

  TempTIR(std::shared_ptr<parsetree::ast::ExprValue> astNode, Type type)
      : astNode{astNode}, type{type} {}

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(!!! TempTIR" << std::endl;
    printIndent(os, indent + 1);
    os << magic_enum::enum_name(type) << std::endl;
    astNode->print(std::cout, indent + 1);
    printIndent(os, indent);
    os << "!!!)\n";
    return os;
  }

  Type type;
  std::shared_ptr<parsetree::ast::ExprValue> astNode;
};

} // namespace tir

namespace codegen {

class ExprIRConverter final
    : public static_check::Evaluator<std::shared_ptr<tir::Expr>> {

public:
  ExprIRConverter(std::shared_ptr<parsetree::ast::ASTManager> astManager,
                  std::shared_ptr<CodeGenLabels> codeGenLabels) {
    this->astManager = astManager;
    this->codeGenLabels = codeGenLabels;
  }

  void
  setCurrentClass(std::shared_ptr<parsetree::ast::ClassDecl> currentClass) {
    this->currentClass = currentClass;
  }

private:
  std::shared_ptr<tir::Expr>
  evalBinOp(std::shared_ptr<parsetree::ast::BinOp> &op,
            const std::shared_ptr<tir::Expr> lhs,
            const std::shared_ptr<tir::Expr> rhs) override;

  std::shared_ptr<tir::Expr>
  evalUnOp(std::shared_ptr<parsetree::ast::UnOp> &op,
           const std::shared_ptr<tir::Expr> rhs) override;

  std::shared_ptr<tir::Expr>
  evalFieldAccess(std::shared_ptr<parsetree::ast::FieldAccess> &op,
                  const std::shared_ptr<tir::Expr> lhs,
                  const std::shared_ptr<tir::Expr> field) override;

  std::shared_ptr<tir::Expr> evalMethodInvocation(
      std::shared_ptr<parsetree::ast::MethodInvocation> &op,
      const std::shared_ptr<tir::Expr> method,
      const std::vector<std::shared_ptr<tir::Expr>> &args) override;

  std::shared_ptr<tir::Expr>
  evalNewObject(std::shared_ptr<parsetree::ast::ClassCreation> &op,
                const std::shared_ptr<tir::Expr> object,
                const std::vector<std::shared_ptr<tir::Expr>> &args) override;

  std::shared_ptr<tir::Expr>
  evalNewArray(std::shared_ptr<parsetree::ast::ArrayCreation> &op,
               const std::shared_ptr<tir::Expr> type,
               const std::shared_ptr<tir::Expr> size) override;

  std::shared_ptr<tir::Expr>
  evalArrayAccess(std::shared_ptr<parsetree::ast::ArrayAccess> &op,
                  const std::shared_ptr<tir::Expr> array,
                  const std::shared_ptr<tir::Expr> index) override;

  std::shared_ptr<tir::Expr>
  evalCast(std::shared_ptr<parsetree::ast::Cast> &op,
           const std::shared_ptr<tir::Expr> type,
           const std::shared_ptr<tir::Expr> value) override;

  std::shared_ptr<tir::Expr>
  evalAssignment(std::shared_ptr<parsetree::ast::Assignment> &op,
                 const std::shared_ptr<tir::Expr> lhs,
                 const std::shared_ptr<tir::Expr> rhs) override;

  std::shared_ptr<tir::Expr>
  mapValue(std::shared_ptr<parsetree::ast::ExprValue> &value) override;

  std::shared_ptr<parsetree::ast::ASTManager> astManager;
  std::shared_ptr<CodeGenLabels> codeGenLabels;
  std::shared_ptr<parsetree::ast::ClassDecl> currentClass = nullptr;
};

} // namespace codegen
