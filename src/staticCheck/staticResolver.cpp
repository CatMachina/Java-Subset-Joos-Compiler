#include "staticCheck/staticResolver.hpp"

namespace static_check {

// Taken from HierarchyCheck
static bool isClass(std::shared_ptr<parsetree::ast::AstNode> decl) {
  return !!std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(decl);
}

static bool isSuperClass(std::shared_ptr<parsetree::ast::AstNode> super,
                         std::shared_ptr<parsetree::ast::AstNode> child) {
  if (!child || !super) {
    return false;
  }
  if (!isClass(child)) {
    return false;
  }
  if (!isClass(super)) {
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

    if (isSuperClass(super, std::dynamic_pointer_cast<parsetree::ast::AstNode>(
                                superClassDecl)))
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

/**
 * Check if the given instance variable is valid to access in the current
 * context.
 *
 * An instance variable can only be accessed in an instance context, not in a
 * static context. If the instance variable is accessed in the initializer of
 * another field, the instance variable must also be declared before the
 * current field.
 *
 * @param variable the variable to check
 * @param checkInitOrder whether to check the lexical order of the instance
 *        variables
 */
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

/**
 * Checks if the given variable is accessible from the given LHS.
 *
 * Checks if the given variable is accessible from the given LHS. If the
 * variable is an instance variable, it must be accessible from the given LHS.
 * If the variable is protected, it must be accessed from a subclass of the
 * same package or from the same package.
 *
 * @param lhs the LHS variable
 * @param variable the variable to check
 */
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

/**
 * Maps an expression value to a StaticResolverData object.
 *
 * Checks if the given expression value is a literal or a type node. If it is a
 * literal, sets the type of the StaticResolverData object to the type of the
 * literal. If it is a type node, sets the type of the StaticResolverData object
 * to the type of the type node. If it is neither a literal nor a type node,
 * sets the type of the StaticResolverData object to the type of the given
 * expression value.
 *
 * Checks if the given expression value is an instance variable. If it is, sets
 * the isInstanceVariable field of the StaticResolverData object to true.
 *
 * @param value the expression value to map
 * @return the mapped StaticResolverData object
 */
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

/**
 * Evaluates a binary operation and returns the resulting StaticResolverData.
 *
 * This function checks the validity of the instance variables involved in the
 * binary operation, ensuring they can be accessed in the current context. It
 * then returns a StaticResolverData object with the result type of the
 * operation.
 *
 * @param op a shared pointer to the binary operation AST node.
 * @param lhs the left-hand side operand as StaticResolverData.
 * @param rhs the right-hand side operand as StaticResolverData.
 * @return a StaticResolverData object with the result type of the binary
 * operation.
 * @throws std::runtime_error if the result type of the operation cannot be
 * resolved.
 */

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

/**
 * Evaluates an unary operation and returns the resulting StaticResolverData.
 *
 * This function checks the validity of the instance variables involved in the
 * unary operation, ensuring they can be accessed in the current context. It
 * then returns a StaticResolverData object with the result type of the
 * operation.
 *
 * @param op a shared pointer to the unary operation AST node.
 * @param rhs the right-hand side operand as StaticResolverData.
 * @return a StaticResolverData object with the result type of the unary
 * operation.
 * @throws std::runtime_error if the result type of the operation cannot be
 * resolved.
 */
StaticResolverData
StaticResolver::evalUnOp(std::shared_ptr<parsetree::ast::UnOp> &op,
                         const StaticResolverData rhs) {
  if (!(op->getResultType()))
    throw std::runtime_error(
        "cannot resolve unOp at previous type resolve stage");

  this->checkInstanceVariable(rhs);
  return StaticResolverData{nullptr, op->getResultType(), true, false};
}

/**
 * Evaluates a field access operation and returns the resulting
 * StaticResolverData.
 *
 * This function ensures that the left-hand side (LHS) is a value and the field
 * being accessed has a resolved declaration. It checks the validity of instance
 * variables involved, ensuring they can be accessed in the current context.
 * Accessing a static field through an instance variable will throw an error.
 *
 * @param op a shared pointer to the field access AST node.
 * @param lhs the left-hand side operand as StaticResolverData.
 * @param field the field to access as StaticResolverData.
 * @return a StaticResolverData object with the result type of the field access.
 * @throws std::runtime_error if the field access cannot be resolved or if
 *         the constraints on LHS or RHS are not met.
 */
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

/**
 * Evaluates a method invocation and returns the resulting StaticResolverData.
 *
 * This function checks the validity of the method and its arguments, ensuring
 * they can be accessed in the current context. It verifies that the method is
 * a value and has a resolved declaration. Additionally, it ensures all provided
 * arguments are values and checks their validity. The function then returns a
 * StaticResolverData object with the result type of the method invocation.
 *
 * @param op a shared pointer to the method invocation AST node.
 * @param method the method being invoked as StaticResolverData.
 * @param args a vector of StaticResolverData representing the arguments to the
 * method.
 * @return a StaticResolverData object with the result type of the method
 * invocation.
 * @throws std::runtime_error if the method or its arguments do not meet the
 * required constraints.
 */
StaticResolverData StaticResolver::evalMethodInvocation(
    std::shared_ptr<parsetree::ast::MethodInvocation> &op,
    const StaticResolverData method,
    const std::vector<StaticResolverData> &args) {
  if (!(method.isValue && method.decl))
    throw std::runtime_error(
        "method must be a value and have a resolved declaration");
  this->checkInstanceVariable(method);
  for (auto arg : args) {
    if (!(arg.isValue))
      throw std::runtime_error("all arguments must be values");
    checkInstanceVariable(arg);
  }
  return StaticResolverData{nullptr, op->getResultType(), true, false};
}

/**
 * Evaluates a class creation (new object) and returns the resulting
 * StaticResolverData.
 *
 * This function checks the validity of the object and its arguments, ensuring
 * they can be accessed in the current context. It verifies that the object is
 * not a value and has a resolved type. Additionally, it ensures all provided
 * arguments are values and checks their validity. The function then returns a
 * StaticResolverData object with the result type of the class creation.
 *
 * @param op a shared pointer to the class creation AST node.
 * @param object the object being created as StaticResolverData.
 * @param args a vector of StaticResolverData representing the arguments to the
 * constructor.
 * @return a StaticResolverData object with the result type of the class
 * creation.
 * @throws std::runtime_error if the object or its arguments do not meet the
 * required constraints.
 */
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

/**
 * Evaluates a new array creation and returns the resulting StaticResolverData.
 *
 * This function checks the validity of the array and its arguments, ensuring
 * they can be accessed in the current context. It verifies that the array is
 * not a value and has a resolved type. Additionally, it ensures the size is a
 * value and checks its validity. The function then returns a StaticResolverData
 * object with the result type of the array creation.
 *
 * @param op a shared pointer to the array creation AST node.
 * @param type the type of the array elements as StaticResolverData.
 * @param size the size of the array as StaticResolverData.
 * @return a StaticResolverData object with the result type of the array
 * creation.
 * @throws std::runtime_error if the object or its arguments do not meet the
 * required constraints.
 */
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

/**
 * Evaluates an array access operation and returns the resulting
 * StaticResolverData.
 *
 * This function checks the validity of the array and its index, ensuring
 * they can be accessed in the current context. It verifies that the array and
 * index are values and checks their validity. The function then returns a
 * StaticResolverData object with the result type of the array access.
 *
 * @param op a shared pointer to the array access AST node.
 * @param array the array being accessed as StaticResolverData.
 * @param index the index of the element being accessed as StaticResolverData.
 * @return a StaticResolverData object with the result type of the array access.
 * @throws std::runtime_error if the array or its index do not meet the required
 * constraints.
 */
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

/**
 * Evaluates a cast operation and returns the resulting StaticResolverData.
 *
 * This function checks the validity of the cast, ensuring it can be accessed in
 * the current context. It verifies that the type and value are values and
 * checks their validity. The function then returns a StaticResolverData object
 * with the result type of the cast.
 *
 * @param op a shared pointer to the cast AST node.
 * @param type the type being cast to as StaticResolverData.
 * @param value the value being cast as StaticResolverData.
 * @return a StaticResolverData object with the result type of the cast.
 * @throws std::runtime_error if the cast or its arguments do not meet the
 * required constraints.
 */
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

/**
 * Evaluates an assignment operation and returns the resulting
 * StaticResolverData.
 *
 * This function checks the validity of the assignment, ensuring it can be
 * accessed in the current context. It verifies that the LHS and RHS are values
 * and checks their validity. The function then returns a StaticResolverData
 * object with the result type of the assignment.
 *
 * @param op a shared pointer to the assignment AST node.
 * @param lhs the left-hand side of the assignment as StaticResolverData.
 * @param rhs the right-hand side of the assignment as StaticResolverData.
 * @return a StaticResolverData object with the result type of the assignment.
 * @throws std::runtime_error if the assignment or its arguments do not meet the
 *         required constraints.
 */
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
