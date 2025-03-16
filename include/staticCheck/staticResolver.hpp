#pragma once

#include "ast/ast.hpp"
#include "evaluator.hpp"

namespace static_check {

// state for static resolution
struct StaticResolverState {
  bool isStaticContext;
  bool isInstFieldInitializer;
  std::shared_ptr<parsetree::ast::ClassDecl> currentClass;
  std::shared_ptr<parsetree::ast::ScopeID> fieldScope;
  StaticResolverState()
      : isStaticContext{false}, isInstFieldInitializer{false},
        currentClass{nullptr}, fieldScope{nullptr} {}
};

// store info in static resolution
struct StaticResolverData {
  const std::shared_ptr<parsetree::ast::Decl> decl;
  const std::shared_ptr<parsetree::ast::Type> type;
  const bool isValue;
  const bool isInstanceVariable;

  void print(std::ostream &os = std::cout) const {
    os << "StaticResolverData {\n";
    os << "  Decl: ";
    if (decl) {
      decl->print(os);
    } else {
      os << "null";
    }
    os << "\n";

    os << "  Type: ";
    if (type) {
      type->print(os);
    } else {
      os << "null";
    }
    os << "\n";

    os << "  isValue: " << (isValue ? "true" : "false") << "\n";
    os << "  isInstanceVariable: " << (isInstanceVariable ? "true" : "false")
       << "\n";
    os << "}\n";
  }
};

// Static resolver of expressions
class StaticResolver : public Evaluator<StaticResolverData> {

public:
  void evaluate(std::shared_ptr<parsetree::ast::Expr> expr,
                StaticResolverState state);

private:
  StaticResolverData
  mapValue(std::shared_ptr<parsetree::ast::ExprValue> &value) override;

  StaticResolverData evalBinOp(std::shared_ptr<parsetree::ast::BinOp> &op,
                               const StaticResolverData lhs,
                               const StaticResolverData rhs) override;

  StaticResolverData evalUnOp(std::shared_ptr<parsetree::ast::UnOp> &op,
                              const StaticResolverData rhs) override;

  StaticResolverData
  evalFieldAccess(std::shared_ptr<parsetree::ast::FieldAccess> &op,
                  const StaticResolverData lhs,
                  const StaticResolverData field) override;

  StaticResolverData
  evalMethodInvocation(std::shared_ptr<parsetree::ast::MethodInvocation> &op,
                       const StaticResolverData method,
                       const std::vector<StaticResolverData> &args) override;

  StaticResolverData
  evalNewObject(std::shared_ptr<parsetree::ast::ClassCreation> &op,
                const StaticResolverData object,
                const std::vector<StaticResolverData> &args) override;

  StaticResolverData
  evalNewArray(std::shared_ptr<parsetree::ast::ArrayCreation> &op,
               const StaticResolverData type,
               const StaticResolverData size) override;

  StaticResolverData
  evalArrayAccess(std::shared_ptr<parsetree::ast::ArrayAccess> &op,
                  const StaticResolverData array,
                  const StaticResolverData index) override;

  StaticResolverData evalCast(std::shared_ptr<parsetree::ast::Cast> &op,
                              const StaticResolverData type,
                              const StaticResolverData value) override;

  StaticResolverData
  evalAssignment(std::shared_ptr<parsetree::ast::Assignment> &op,
                 const StaticResolverData lhs,
                 const StaticResolverData rhs) override;

private:
  void checkInstanceVariable(StaticResolverData var,
                             bool checkInitOrder = true) const;
  void isAccessible(StaticResolverData lhs, StaticResolverData var) const;

private:
  StaticResolverState state;
};

} // namespace static_check
