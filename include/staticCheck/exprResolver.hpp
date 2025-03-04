#pragma once

#include "ast/ast.hpp"
#include "ast/astManager.hpp"
#include "environment.hpp"
#include "evaluator.hpp"
#include "hierarchyCheck.hpp"
#include "typeLinker.hpp"
// #include "typeResolver.hpp"

namespace static_check {

class ExprNameLinked;

using ExprNodeList = std::vector<std::shared_ptr<parsetree::ast::ExprNode>>;
using exprResolveType =
    std::variant<std::shared_ptr<ExprNameLinked>,
                 std::shared_ptr<parsetree::ast::ExprNode>, ExprNodeList>;
using previousType =
    std::variant<std::shared_ptr<ExprNameLinked>, ExprNodeList>;

static std::shared_ptr<parsetree::ast::Decl>
GetTypeAsDecl(std::shared_ptr<parsetree::ast::Type> type,
              std::shared_ptr<parsetree::ast::ASTManager> manager) {
  if (auto refType =
          std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(type)) {
    return refType->getResolvedDecl()->getAstNode();
  } else if (type->isString()) {
    return manager->java_lang.String;
  } else if (type->isArray()) {
    return manager->java_lang.Arrays;
  } else {
    return nullptr;
  }
}

class ExprNameLinked {

public:
  enum class ValueType {
    PackageName,
    TypeName,
    ExpressionName,
    MethodName,
    SingleAmbiguousName
  };

  ExprNameLinked(ValueType type,
                 std::shared_ptr<parsetree::ast::SimpleName> node,
                 std::shared_ptr<parsetree::ast::FieldAccess> op) {
    this->valueType_ = type;
    this->node_ = node;
    this->op_ = op;
    this->prev_ = std::nullopt;
  }

  ValueType getValueType() const { return valueType_; }
  void setValueType(ValueType valueType) { this->valueType_ = valueType; }

  std::shared_ptr<parsetree::ast::SimpleName> getNode() const { return node_; }
  std::optional<previousType> getPrev() const { return prev_; }
  std::shared_ptr<parsetree::ast::FieldAccess> getOp() const { return op_; }

  void setPrev(std::optional<previousType> prev) {
    if (prev.has_value()) {
      if (auto linked =
              std::get_if<std::shared_ptr<ExprNameLinked>>(&prev.value())) {
        if (*linked == nullptr)
          throw std::runtime_error("Previous value is nullptr");
      }
    }
    prev_ = prev;
  }

  std::shared_ptr<ExprNameLinked> prevAsLinked() const {
    if (!prev_.has_value())
      throw std::runtime_error("No previous value");
    if (auto prev =
            std::get_if<std::shared_ptr<ExprNameLinked>>(&prev_.value()))
      return *prev;
    return nullptr;
  }

  // std::shared_ptr<parsetree::ast::Decl>
  // prevAsDecl(std::shared_ptr<TypeResolver> TR,
  //            std::shared_ptr<parsetree::ast::ASTManager> manager) const {
  //   if (auto p = prevAsLinked())
  //     return p->getNode()->getResolvedDecl();
  //   ExprNodeList prevList = std::get<ExprNodeList>(prev_.value());
  //   auto type = TR->EvalList(prevList);
  //   return std::dynamic_pointer_cast<parsetree::ast::Decl>(
  //       GetTypeAsDecl(type, manager));
  // }

private:
  std::shared_ptr<parsetree::ast::SimpleName> node_;
  std::shared_ptr<parsetree::ast::FieldAccess> op_;
  ValueType valueType_;
  std::optional<previousType> prev_;
};

class ExprResolver : public Evaluator<exprResolveType> {
  // class ExprResolver {

public:
  ExprResolver(std::shared_ptr<parsetree::ast::ASTManager> astManager,
               std::shared_ptr<HierarchyCheck> hierarchyChecker,
               std::shared_ptr<TypeLinker> typeLinker)
      : astManager(astManager), hierarchyChecker(hierarchyChecker),
        typeLinker(typeLinker) {}
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
  recursiveReduce(std::shared_ptr<ExprNameLinked> node);

  std::shared_ptr<ExprNameLinked>
  resolveSimpleName(std::shared_ptr<ExprNameLinked> expr);

  std::shared_ptr<parsetree::ast::Decl>
  lookupNamedDecl(std::shared_ptr<parsetree::ast::CodeBody> ctx,
                  std::string_view name);

  std::shared_ptr<parsetree::ast::Decl>
  reclassifyDecl(std::shared_ptr<parsetree::ast::CodeBody> ctx,
                 std::shared_ptr<ExprNameLinked> node);

  bool isAccessible(std::shared_ptr<parsetree::ast::Modifiers> mod,
                    std::shared_ptr<parsetree::ast::CodeBody> parent);

  std::shared_ptr<parsetree::ast::ASTManager> astManager;
  std::shared_ptr<HierarchyCheck> hierarchyChecker;
  std::shared_ptr<TypeLinker> typeLinker;
  // std::shared_ptr<TypeResolver> typeResolver;
  std::shared_ptr<parsetree::ast::ProgramDecl> currentProgram;
  std::shared_ptr<parsetree::ast::CodeBody> currentContext;
  std::shared_ptr<parsetree::ast::ScopeID> currentScope;
};

} // namespace static_check
