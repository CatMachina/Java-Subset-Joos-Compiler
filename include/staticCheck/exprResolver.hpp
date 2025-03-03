#pragma once

#include "ast/ast.hpp"
#include "ast/astManager.hpp"
#include "environment.hpp"
#include "evaluator.hpp"
#include "hierarchyCheck.hpp"
#include "typeLinker.hpp"

namespace static_check {

using exprResolveType =
    std::variant<std::shared_ptr<parsetree::ast::ExprNode>,
                 std::vector<std::shared_ptr<parsetree::ast::ExprNode>>>;

using previousType =
    std::variant<std::shared_ptr<parsetree::ast::ExprNode>,
                 std::vector<std::shared_ptr<parsetree::ast::ExprNode>>>;

class ExprResolver : public Evaluator<exprResolveType> {
  // class ExprResolver {

public:
  ExprResolver(std::shared_ptr<parsetree::ast::ASTManager> astManager)
      : astManager(astManager) {}
  void BeginProgram(std::shared_ptr<parsetree::ast::ProgramDecl> programDecl) {
    currentProgram = programDecl;
  }
  void BeginContext(std::shared_ptr<parsetree::ast::CodeBody> context) {
    currentContext = context;
  }

  void resolve();

private:
  void resolveAST(std::shared_ptr<parsetree::ast::AstNode> ast);
  std::vector<std::shared_ptr<parsetree::ast::ExprNode>>
  resolveExprNode(const exprResolveType node);
  void evaluate(std::shared_ptr<parsetree::ast::Expr> expr);
  exprResolveType
  evaluateList(std::vector<std::shared_ptr<parsetree::ast::ExprNode>> &list);

  exprResolveType mapValue(std::shared_ptr<parsetree::ast::ExprValue> &node);
  exprResolveType
  evalQualifiedName(std::shared_ptr<parsetree::ast::QualifiedName> &node);
  exprResolveType evalBinOp(std::shared_ptr<parsetree::ast::BinOp> &op,
                            const exprResolveType lhs,
                            const exprResolveType rhs);
  exprResolveType evalUnOp(std::shared_ptr<parsetree::ast::UnOp> &op,
                           const exprResolveType rhs);
  exprResolveType
  evalFieldAccess(std::shared_ptr<parsetree::ast::FieldAccess> &op,
                  const exprResolveType lhs, const exprResolveType field);
  exprResolveType
  evalMethodInvocation(std::shared_ptr<parsetree::ast::MethodInvocation> &op,
                       const exprResolveType method, const op_array &args);
  exprResolveType
  evalNewObject(std::shared_ptr<parsetree::ast::ClassCreation> &op,
                const exprResolveType object, const op_array &args);
  exprResolveType
  evalNewArray(std::shared_ptr<parsetree::ast::ArrayCreation> &op,
               const exprResolveType type, const exprResolveType size);
  exprResolveType
  evalArrayAccess(std::shared_ptr<parsetree::ast::ArrayAccess> &op,
                  const exprResolveType array, const exprResolveType index);
  exprResolveType evalCast(std::shared_ptr<parsetree::ast::Cast> &op,
                           const exprResolveType type,
                           const exprResolveType value);

  std::vector<std::shared_ptr<parsetree::ast::ExprNode>>
  recursiveReduce(std::shared_ptr<parsetree::ast::ExprValue> node);

  std::shared_ptr<parsetree::ast::SimpleName>
  resolveSimpleName(std::shared_ptr<parsetree::ast::SimpleName> expr);

  std::shared_ptr<parsetree::ast::Decl>
  lookupNamedDecl(std::shared_ptr<parsetree::ast::CodeBody> ctx,
                  std::string_view name);

  std::shared_ptr<parsetree::ast::Decl>
  reclassifyDecl(std::shared_ptr<parsetree::ast::CodeBody> ctx,
                 std::shared_ptr<parsetree::ast::SimpleName> node);

  bool isAccessible(std::shared_ptr<parsetree::ast::Modifiers> mod,
                    std::shared_ptr<parsetree::ast::CodeBody> parent);

  std::shared_ptr<parsetree::ast::ASTManager> astManager;
  std::shared_ptr<HierarchyCheck> hierarchyChecker;
  std::shared_ptr<TypeLinker> typeLinker;
  std::shared_ptr<parsetree::ast::ProgramDecl> currentProgram;
  std::shared_ptr<parsetree::ast::CodeBody> currentContext;
  std::shared_ptr<parsetree::ast::ScopeID> currentScope;
};

} // namespace static_check
