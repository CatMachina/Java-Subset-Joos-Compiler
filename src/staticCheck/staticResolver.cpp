#include "staticCheck/staticResolver.hpp"

namespace static_check {

// Taken from HierarchyCheck
static bool isClass(std::shared_ptr<parsetree::ast::AstNode> decl) {
  return !!std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(decl);
}

static bool isSuperClass(std::shared_ptr<parsetree::ast::AstNode> super,
                         std::shared_ptr<parsetree::ast::AstNode> child) {
  if (!child || !super) {
    // std::cout << "Either child or super is a nullptr."
    return false;
  }
  if (!isClass(child)) {
    // std::cout << "Child class is not a class!\n";
    return false;
  }
  if (!isClass(super)) {
    // std::cout << "Super class is not a class!\n";
    return false;
  }
  auto childDecl = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(child);
  auto superDecl = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(super);
  for (auto &superClass : childDecl->getSuperClasses()) {
    if (!superClass || !superClass->getResolvedDecl() ||
        !superClass->getResolvedDecl())
      continue;

    // Cast to class
    auto superClassDecl = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
        superClass->getResolvedDecl()->getAstNode());

    if (superClassDecl == superDecl)
      return true;

    if (isSuperClass(
            std::dynamic_pointer_cast<parsetree::ast::AstNode>(superClassDecl),
            super))
      return true;
  }
  return false;
}

void StaticResolver::evaluate(std::shared_ptr<parsetree::ast::Expr> expr,
                              StaticResolverState state) {
  this->state = state;
  StaticResolverData ret = this->evaluateList(expr->getExprNodes());
  this->checkInstanceVariable(ret);
  return;
}

void StaticResolver::checkInstanceVariable(StaticResolverData variable,
                                           bool checkInitOrder) const {
  if (!variable.isInstanceVariable)
    return;
  // Instance variable must not be accessed in a static context
  if (state.isStaticContext) {
    throw std::runtime_error(
        "cannot access or invoke instance members in a static context");
  }
  // Instance variable accessed in a field initializer must be lexical order
  if (state.isInstFieldInitializer && checkInitOrder) {
    auto fieldDecl =
        std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(variable.decl);
    if (!state.fieldScope->canView(fieldDecl->getScope())) {
      throw std::runtime_error("cannot access instance members in initializer "
                               "before they are defined");
    }
  }
}

void StaticResolver::isAccessible(StaticResolverData lhs,
                                  StaticResolverData variable) const {
  if (!variable.isInstanceVariable)
    return;

  auto lhsVar = std::dynamic_pointer_cast<parsetree::ast::VarDecl>(lhs.decl);
  if (!lhsVar)
    return;

  auto lhsRef = std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(
      lhsVar->getType());
  if (!lhsRef)
    return;

  auto lhsClass = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
      lhsRef->getResolvedDecl()->getAstNode());
  if (!lhsClass)
    return;

  if (auto method = std::dynamic_pointer_cast<parsetree::ast::MethodDecl>(
          variable.decl)) {
    if (method->getModifiers()->isProtected()) {
      auto currentProgram =
          std::dynamic_pointer_cast<parsetree::ast::ProgramDecl>(
              state.currentClass->getParent());
      auto lhsProgram = std::dynamic_pointer_cast<parsetree::ast::ProgramDecl>(
          lhsClass->getParent());
      if (!isSuperClass(state.currentClass, lhsClass) &&
          (currentProgram->getPackageName() != lhsProgram->getPackageName())) {
        throw std::runtime_error("cannot access protected method: " +
                                 lhsClass->getName() + "." + method->getName());
      }
    }
  } else if (auto field = std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(
                 variable.decl)) {
    if (field->getModifiers()->isProtected()) {
      auto currentProgram =
          std::dynamic_pointer_cast<parsetree::ast::ProgramDecl>(
              state.currentClass->getParent());
      auto lhsProgram = std::dynamic_pointer_cast<parsetree::ast::ProgramDecl>(
          lhsClass->getParent());
      if (!isSuperClass(state.currentClass, lhsClass) &&
          currentProgram->getPackageName() != lhsProgram->getPackageName()) {
        throw std::runtime_error("cannot access protected field: " +
                                 lhsClass->getName() + "." + field->getName());
      }
    }
  }
  return;
}

StaticResolverData
StaticResolver::mapValue(std::shared_ptr<parsetree::ast::ExprValue> &value) {
  if ((std::dynamic_pointer_cast<parsetree::ast::ThisNode>(value)) &&
      state.isStaticContext) {
    throw std::runtime_error("cannot use 'this' in a static context");
  }

  if (!((value->isTypeResolved()) ||
        (std::dynamic_pointer_cast<parsetree::ast::MethodName>(value)))) {
    value->print(std::cout);
    std::cout << "value is type resolved? " << value->isTypeResolved()
              << std::endl;
    throw std::runtime_error(
        "cannot resolve mapValue at previous type resolve stage");
  }
  if (std::dynamic_pointer_cast<parsetree::ast::Literal>(value)) {
    return StaticResolverData{nullptr, value->getType(), true, false};
  } else if (std::dynamic_pointer_cast<parsetree::ast::TypeNode>(value)) {
    return StaticResolverData{nullptr, value->getType(), false, false};
  } else {
    auto decl = value->getResolvedDecl();
    bool isInstanceVariable =
        std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
            decl->getParent()) &&
        !(decl->isStatic());
    return StaticResolverData{decl, value->getType(), true, isInstanceVariable};
  }
}

StaticResolverData
StaticResolver::evalBinOp(std::shared_ptr<parsetree::ast::BinOp> &op,
                          const StaticResolverData lhs,
                          const StaticResolverData rhs) {
  if (!(op->getResultType()))
    throw std::runtime_error(
        "cannot resolve binOp at previous type resolve stage");

  this->checkInstanceVariable(lhs);
  this->checkInstanceVariable(rhs);
  return StaticResolverData{nullptr, op->getResultType(), true, false};
}

StaticResolverData
StaticResolver::evalUnOp(std::shared_ptr<parsetree::ast::UnOp> &op,
                         const StaticResolverData rhs) {
  if (!(op->getResultType()))
    throw std::runtime_error(
        "cannot resolve unOp at previous type resolve stage");

  this->checkInstanceVariable(rhs);
  return StaticResolverData{nullptr, op->getResultType(), true, false};
}

StaticResolverData StaticResolver::evalFieldAccess(
    std::shared_ptr<parsetree::ast::FieldAccess> &op,
    const StaticResolverData lhs, const StaticResolverData field) {
  if (!(op->getResultType()))
    throw std::runtime_error(
        "cannot resolve field access at previous type resolve stage");
  if (!(lhs.isValue))
    throw std::runtime_error("LHS must be a value for field access");
  if (!((field.isValue) && (field.decl)))
    throw std::runtime_error(
        "RHS must be a field and have a resolved declaration");

  this->checkInstanceVariable(lhs);
  this->isAccessible(lhs, field);
  if (field.decl->isStatic())
    throw std::runtime_error(
        "cannot access a static field through an instance variable");
  return StaticResolverData{field.decl, op->getResultType(), true, false};
}

StaticResolverData StaticResolver::evalMethodInvocation(
    std::shared_ptr<parsetree::ast::MethodInvocation> &op,
    const StaticResolverData method,
    const std::vector<StaticResolverData> &args) {
  if (!(method.isValue && method.decl))
    throw std::runtime_error(
        "method must be a value and have a resolved declaration");
  // method.print(std::cout);
  this->checkInstanceVariable(method);
  for (auto arg : args) {
    if (!(arg.isValue))
      throw std::runtime_error("all arguments must be values");
    checkInstanceVariable(arg);
  }
  return StaticResolverData{nullptr, op->getResultType(), true, false};
}

StaticResolverData StaticResolver::evalNewObject(
    std::shared_ptr<parsetree::ast::ClassCreation> &op,
    const StaticResolverData object,
    const std::vector<StaticResolverData> &args) {
  if (!(op->getResultType()))
    throw std::runtime_error(
        "cannot resolve new object at previous type resolve stage");
  if (object.isValue || !(object.type))
    throw std::runtime_error("object must not be a value and have a type");
  for (auto arg : args) {
    if (!(arg.isValue))
      throw std::runtime_error("all arguments must be values");
    this->checkInstanceVariable(arg);
  }
  return StaticResolverData{nullptr, op->getResultType(), true, false};
}

StaticResolverData
StaticResolver::evalNewArray(std::shared_ptr<parsetree::ast::ArrayCreation> &op,
                             const StaticResolverData type,
                             const StaticResolverData size) {
  if (!(op->getResultType()))
    throw std::runtime_error(
        "cannot resolve new array at previous type resolve stage");
  if (type.isValue || !type.type)
    throw std::runtime_error("type must be a type");
  if (!(size.isValue))
    throw std::runtime_error("size must be a value");
  this->checkInstanceVariable(size);
  return StaticResolverData{nullptr, op->getResultType(), true, false};
}

StaticResolverData StaticResolver::evalArrayAccess(
    std::shared_ptr<parsetree::ast::ArrayAccess> &op,
    const StaticResolverData array, const StaticResolverData index) {
  if (!(op->getResultType()))
    throw std::runtime_error(
        "cannot resolve array access at previous type resolve stage");
  if (!(array.isValue))
    throw std::runtime_error("array must be a value");
  if (!(index.isValue))
    throw std::runtime_error("index must be a value");
  this->checkInstanceVariable(array);
  this->checkInstanceVariable(index);
  return StaticResolverData{nullptr, op->getResultType(), true, false};
}

StaticResolverData
StaticResolver::evalCast(std::shared_ptr<parsetree::ast::Cast> &op,
                         const StaticResolverData type,
                         const StaticResolverData value) {
  if (!(op->getResultType()))
    throw std::runtime_error(
        "cannot resolve cast at previous type resolve stage");
  if (type.isValue || !type.type)
    throw std::runtime_error("cast type must be a type");
  if (!(value.isValue))
    throw std::runtime_error("cast value must be a value");
  this->checkInstanceVariable(value);
  return StaticResolverData{nullptr, op->getResultType(), true, false};
}

StaticResolverData
StaticResolver::evalAssignment(std::shared_ptr<parsetree::ast::Assignment> &op,
                               const StaticResolverData lhs,
                               const StaticResolverData rhs) {
  if (!(op->getResultType()))
    throw std::runtime_error(
        "cannot resolve assignment at previous type resolve stage");

  // We can safely ignore LHS field access
  this->checkInstanceVariable(lhs, false);
  // If the LHS is a field, it must not be final
  auto decl = lhs.decl;
  if (auto field = std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(decl)) {
    if (field->getModifiers()->isFinal()) {
      throw std::runtime_error("cannot assign to a final field");
    }
  } else if (auto var =
                 std::dynamic_pointer_cast<parsetree::ast::VarDecl>(decl)) {
    op->setAssignedVariable(var);
  }
  this->checkInstanceVariable(rhs);
  return StaticResolverData{nullptr, op->getResultType(), true, false};
}

} // namespace static_check
