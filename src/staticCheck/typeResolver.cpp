#include "staticCheck/typeResolver.hpp"

namespace static_check {

bool TypeResolver::isReferenceOrArrType(
    std::shared_ptr<parsetree::ast::Type> type) const {
  return std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(type) ||
         std::dynamic_pointer_cast<parsetree::ast::ArrayType>(type) ||
         type->isString();
}

bool TypeResolver::isTypeString(
    std::shared_ptr<parsetree::ast::Type> type) const {
  if (type->isString())
    return true;
  if (auto refType =
          std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(type)) {
    return refType->getResolvedDecl() == astManager->java_lang.String;
  }
  return false;
}

// Taken from HierarchyCheck
bool isClass(std::shared_ptr<parsetree::ast::AstNode> decl) {
  return !!std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(decl);
}

// Taken from HierarchyCheck
bool isInterface(std::shared_ptr<parsetree::ast::AstNode> decl) {
  return !!std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(decl);
}

bool isSuperClass(std::shared_ptr<parsetree::ast::AstNode> child,
                  std::shared_ptr<parsetree::ast::AstNode> super) {
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
        superClass->getResolvedDecl());

    if (superClassDecl == superDecl)
      return true;

    if (isSuperClass(
            std::dynamic_pointer_cast<parsetree::ast::AstNode>(superClassDecl),
            super))
      return true;
  }
  return false;
}

bool isSuperInterface(std::shared_ptr<parsetree::ast::AstNode> child,
                      std::shared_ptr<parsetree::ast::AstNode> interface) {
  if (!child || !interface) {
    // std::cout << "Either child or super interface is a nullptr."
    return false;
  }
  if (!isClass(child) && !isInterface(child)) {
    // std::cout << "Child is not a class or interface!\n";
    return false;
  }
  if (!isInterface(interface)) {
    // std::cout << "Super interface is not an interface!\n";
    return false;
  }
  auto interfaceDecl =
      std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(interface);
  if (isClass(child)) {
    auto childDecl =
        std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(child);
    for (auto &superInterface : childDecl->getInterfaces()) {
      if (!superInterface || !superInterface->getResolvedDecl() ||
          !superInterface->getResolvedDecl())
        continue;
      // Cast to class
      auto superInterfaceDecl =
          std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
              superInterface->getResolvedDecl());

      if (superInterfaceDecl == interface)
        return true;
      if (isSuperInterface(std::dynamic_pointer_cast<parsetree::ast::AstNode>(
                               superInterfaceDecl),
                           interface))
        return true;
    }
    for (auto &superClass : childDecl->getSuperClasses()) {
      if (!superClass || !superClass->getResolvedDecl() ||
          !superClass->getResolvedDecl())
        continue;

      // Cast to class
      auto superClassDecl =
          std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
              superClass->getResolvedDecl());

      if (isSuperInterface(std::dynamic_pointer_cast<parsetree::ast::AstNode>(
                               superClassDecl),
                           interface))
        return true;
    }
  } else {
    auto childDecl =
        std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(child);
    for (auto &superInterface : childDecl->getInterfaces()) {
      if (!superInterface || !superInterface->getResolvedDecl() ||
          !superInterface->getResolvedDecl())
        continue;
      // Cast to interface
      auto superInterfaceDecl =
          std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
              superInterface->getResolvedDecl());

      if (superInterfaceDecl == interface)
        return true;
      if (isSuperInterface(std::dynamic_pointer_cast<parsetree::ast::AstNode>(
                               superInterfaceDecl),
                           interface))
        return true;
    }
  }
  return false;
}

/**
 * Type Conversion Rules:
 *
 * 1. Identity Conversion:
 *    - A type remains unchanged when assigned to a variable of the same type.
 *
 * 2. Widening Primitive Conversion:
 *    - A smaller primitive type can be implicitly converted into a larger
 * primitive type.
 *    - Example: int → long, float → double.
 *
 * 3. Widening Reference Conversion:
 *    - Converts a reference type to a broader (superclass or interface) type.
 *
 *    3.1 Null Type Conversion:
 *        - The `null` type can be assigned to any class type, interface type,
 * or array type.
 *
 *    3.2 Class Type Conversion:
 *        - A class type can be converted to any of its superclasses or any
 * interface it implements.
 *
 *    3.3 Interface Type Conversion:
 *        - An interface type can be converted to any of its superinterfaces or
 * the `Object` class.
 *
 *    3.4 Array Type Conversion:
 *        - An array type can be converted to:
 *          3.4.1 The `Object` class.
 *          3.4.2 The `Cloneable` interface.
 *          3.4.3 The `java.io.Serializable` interface.
 *          3.4.4 Another array type, provided the element type undergoes a
 * widening reference conversion.
 */
bool TypeResolver::isAssignableTo(
    const std::shared_ptr<parsetree::ast::Type> &lhs,
    const std::shared_ptr<parsetree::ast::Type> &rhs) const {
  if (lhs == rhs)
    return true;

  auto asBasicType = [](std::shared_ptr<parsetree::ast::Type> &t) {
    return std::dynamic_pointer_cast<parsetree::ast::BasicType>(t);
  };
  auto asReferenceType = [](std::shared_ptr<parsetree::ast::Type> &t) {
    return std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(t);
  };
  auto asArrayType = [](std::shared_ptr<parsetree::ast::Type> &t) {
    return std::dynamic_pointer_cast<parsetree::ast::ArrayType>(t);
  };
  auto asClassDecl = [](std::shared_ptr<parsetree::ast::ReferenceType> &ref) {
    return std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
        ref->getResolvedDecl());
  };
  auto asInterfaceDecl =
      [](std::shared_ptr<parsetree::ast::ReferenceType> &ref) {
        return std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
            ref->getResolvedDecl());
      };

  auto leftPrimitive = asBasicType(lhs);
  auto rightPrimitive = asBasicType(rhs);
  auto leftRef = asReferenceType(lhs);
  auto rightRef = asReferenceType(rhs);
  auto leftArr = asArrayType(lhs);
  auto rightArr = asArrayType(rhs);

  // Identity conversion: Java astManager->java_lang.String <-> primitive
  // astManager->java_lang.String
  if (isTypeString(lhs) && isTypeString(rhs))
    return true;

  // astManager->java_lang.String conversions
  if (rhs->isString() && leftRef) {
    // TODO: need such API of isSuperClass and isSuperInterface
    // astManager->java_lang.String is the javalang astManager->java_lang.String
    // class
    if (auto leftClass = asClassDecl(leftRef)) {
      return isSuperClass(leftClass, astManager->java_lang.String);
    }
    if (auto leftInterface = asInterfaceDecl(leftRef)) {
      return isSuperInterface(leftInterface, astManager->java_lang.String);
    }
    return false;
  }

  if (lhs->isString() && rightRef) {
    if (auto rightClass = asClassDecl(rightRef)) {
      return isSuperClass(astManager->java_lang.String, rightClass);
    }
    return false;
  }

  // 2. Widening Primitive Conversion
  if (lhs->isPrimitive() && rhs->isPrimitive()) {
    return isWiderThan(leftPrimitive, rightPrimitive);
  }

  // 3.1 Null Type Conversion
  if (rhs->isNull()) {
    return isReferenceOrArrType(lhs);
  }

  if (leftRef && rightRef) {
    // 3.2 Class assignability
    if (auto rightClass = asClassDecl(rightRef)) {
      if (auto leftClass = asClassDecl(leftRef)) {
        // TODO: need such API
        return isSuperClass(leftClass, rightClass);
      }
      if (auto leftInterface = asInterfaceDecl(leftRef)) {
        // TODO: need such API
        return isSuperInterface(leftInterface, rightClass);
      }
    }
    // 3.3 Interface assignability
    if (auto rightInterface = asInterfaceDecl(rightRef)) {
      if (auto leftClass = asClassDecl(leftRef)) {
        return leftClass == astManager->java_lang.Object;
      }
      if (auto leftInterface = asInterfaceDecl(leftRef)) {
        return isSuperInterface(leftInterface, rightInterface);
      }
    }
  }

  // 3.4 Array assignment rules
  if (rightArr) {
    if (leftArr) {
      auto leftElem = asReferenceType(leftArr->getElementType());
      auto rightElem = asReferenceType(rightArr->getElementType());
      return leftElem && rightElem && isAssignableTo(leftElem, rightElem);
    }

    if (leftRef) {
      // Serializable decl
      return leftRef->getResolvedDecl() == astManager->java_lang.Object ||
             leftRef->getResolvedDecl() == astManager->java_lang.Cloneable ||
             leftRef->getResolvedDecl() == astManager->java_lang.Serializable;
    }
  }

  return false;
}

bool TypeResolver::isValidCast(
    const std::shared_ptr<parsetree::ast::Type> &exprType,
    const std::shared_ptr<parsetree::ast::Type> &castType) const {
  if (exprType == castType)
    return true;

  // Identity conversion: Java astManager->java_lang.String <-> primitive
  // astManager->java_lang.String
  if (isTypeString(exprType) && isTypeString(castType))
    return true;

  if (isAssignableTo(exprType, castType) ||
      isAssignableTo(castType, exprType)) {
    return true;
  }

  // Helper lambdas for type casting
  auto asReferenceType = [](const std::shared_ptr<parsetree::ast::Type> &t) {
    return std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(t);
  };
  auto asArrayType = [](const std::shared_ptr<parsetree::ast::Type> &t) {
    return std::dynamic_pointer_cast<parsetree::ast::ArrayType>(t);
  };
  auto asClassDecl =
      [](const std::shared_ptr<parsetree::ast::ReferenceType> &ref) {
        return std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
            ref->getResolvedDecl());
      };
  auto asInterfaceDecl =
      [](const std::shared_ptr<parsetree::ast::ReferenceType> &ref) {
        return std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
            ref->getResolvedDecl());
      };

  auto exprRef = asReferenceType(exprType);
  auto castRef = asReferenceType(castType);

  // If expr is "null", it is assignable to any reference type
  if (exprType->isNull())
    return static_cast<bool>(castRef);
  if (castType->isNull())
    return static_cast<bool>(exprRef);

  auto exprArr = asArrayType(exprType);
  auto castArr = asArrayType(castType);

  // Primitive type casting: only numeric conversions are valid
  if (exprType->isPrimitive() && castType->isPrimitive()) {
    return exprType->isNumeric() && castType->isNumeric();
  }

  if (exprRef) {
    if (castArr)
      return exprRef->getResolvedDecl() == astManager->java_lang.Object;
    if (!castRef)
      return false;

    auto leftInterface = asInterfaceDecl(exprRef);
    auto rightInterface = asInterfaceDecl(castRef);
    auto leftClass = asClassDecl(exprRef);
    auto rightClass = asClassDecl(castRef);

    if (leftInterface && rightInterface)
      return true;
    if (leftInterface && rightClass && !rightClass->modifiers().isFinal())
      return true;
    if (rightInterface && leftClass && !leftClass->modifiers().isFinal())
      return true;

    return isAssignableTo(exprRef, castRef) || isAssignableTo(castRef, exprRef);
  }

  if (exprArr) {
    if (castArr) {
      auto leftElem = asReferenceType(exprArr->getElementType());
      auto rightElem = asReferenceType(castArr->getElementType());
      return leftElem && rightElem &&
             isValidCast(exprArr->getElementType(), castArr->getElementType());
    }
    if (castRef) {
      if (castRef->getResolvedDecl() == astManager->java_lang.Object ||
          castRef->getResolvedDecl() == astManager->java_lang.Serializable) {
        return true;
      }
    }
  }

  throw std::runtime_error("invalid cast from " + exprType->toString() +
                           " to " + castType->toString());
}

// check if the format of exprNode vector and this is consistent
// highly possible not
std::shared_ptr<parsetree::ast::Type> TypeResolver::evaluateList(
    const std::vector<std::shared_ptr<parsetree::ast::ExprNode>> &list) const {
  op_stack.clear();

  for (const auto &node : list) {
    if (auto value =
            std::dynamic_pointer_cast<parsetree::ast::ExprValue>(node)) {
      op_stack.push_back(mapValue(value));
    } else if (auto unary =
                   std::dynamic_pointer_cast<parsetree::ast::UnOp>(node)) {
      auto rhs = popStack();
      op_stack.push_back(evalUnOp(unary, rhs));
    } else if (auto binary =
                   std::dynamic_pointer_cast<parsetree::ast::BinOp>(node)) {
      auto rhs = popStack();
      auto lhs = popStack();
      op_stack.push_back(evalBinOp(binary, lhs, rhs));
    } else if (auto field =
                   std::dynamic_pointer_cast<parsetree::ast::FieldAccess>(
                       node)) {
      auto rhs = popStack();
      auto lhs = popStack();
      op_stack.push_back(evalFieldAccess(field, lhs, rhs));
    } else if (auto method =
                   std::dynamic_pointer_cast<parsetree::ast::MethodInvocation>(
                       node)) {
      std::vector<std::shared_ptr<parsetree::ast::Type>> args(method->nargs() -
                                                              1);
      std::generate(args.rbegin(), args.rend(), [this] { return popStack(); });
      auto method_name = popStack();
      op_stack.push_back(evalMethodInvocation(method, method_name, args));
    } else if (auto newObj =
                   std::dynamic_pointer_cast<parsetree::ast::ClassCreation>(
                       node)) {
      std::vector<std::shared_ptr<parsetree::ast::Type>> args(newObj->nargs() -
                                                              1);
      std::generate(args.rbegin(), args.rend(), [this] { return popStack(); });
      auto type = popStack();
      op_stack.push_back(evalNewObject(newObj, type, args));
    } else if (auto array =
                   std::dynamic_pointer_cast<parsetree::ast::ArrayCreation>(
                       node)) {
      auto size = popStack();
      auto type = popStack();
      op_stack.push_back(evalNewArray(array, type, size));
    } else if (auto access =
                   std::dynamic_pointer_cast<parsetree::ast::ArrayAccess>(
                       node)) {
      auto index = popStack();
      auto array = popStack();
      op_stack.push_back(evalArrayAccess(access, array, index));
    } else if (auto cast =
                   std::dynamic_pointer_cast<parsetree::ast::Cast>(node)) {
      auto value = popStack();
      auto type = popStack();
      op_stack.push_back(evalCast(cast, type, value));
    }
  }
  if (op_stack.size() != 1) {
    throw std::runtime_error("Stack not empty after evaluation!");
  }
  return popStack();
}

std::shared_ptr<parsetree::ast::Type> TypeResolver::evaluate(
    const std::shared_ptr<parsetree::ast::Expr> &node) const {
  return evaluateList(node->getExprNodes());
}

void TypeResolver::resolveAST(
    const std::shared_ptr<parsetree::ast::AST> &node) {
  // only check Expr
  if (auto decl = std::dynamic_pointer_cast<parsetree::ast::TypedDecl>(node)) {
    if (auto init = decl->getInitializer()) {
      std::cout << "resolving initializer for variable: " << decl->toString()
                << std::endl;
      evaluate(init);
    }
  } else if (auto stmt =
                 std::dynamic_pointer_cast<parsetree::ast::Stmt>(node)) {
    for (auto expr : stmt->getExprs()) {
      if (!expr)
        continue;
      std::cout << "resolving expression: ";
      expr->print(std::cout);
      std::cout << std::endl;
      evaluate(expr);
    }
  }
}

/////////////////////////////////////
// Evals of each op
////////////////////////////////////

std::shared_ptr<parsetree::ast::Type>
TypeResolver::mapValue(ExprValue &node) const {
  if (!node.isDeclResolved())
    throw std::runtime_error("ExprValue is not resolved");

  if (auto method = std::dynamic_pointer_cast<MethodDecl>(node.decl())) {
    auto ty = std::make_shared<MethodType>(method);
    if (method->isConstructor()) {
      ty->setReturnType(envManager.BuildReferenceType(
          std::dynamic_pointer_cast<Decl>(method->parent())));
    }
    return ty;
  }

  if (!node.isTypeResolved())
    throw std::runtime_error("ExprValue type is not resolved");
  return node.type();
}

std::shared_ptr<parsetree::ast::Type> TypeResolver::evalBinOp(
    BinOp &op, const std::shared_ptr<parsetree::ast::Type> &lhs,
    const std::shared_ptr<parsetree::ast::Type> &rhs) const {
  if (op.resultType())
    return op.resultType();

  switch (op.opType()) {
  case parsetree::ast::BinOp::OpType::Assignment:
    if (isAssignableTo(lhs, rhs)) {
      return op.resolveResultType(lhs);
    }
    throw std::runtime_error("assignment is not valid");

  case parsetree::ast::BinOp::OpType::GreaterThan:
  case parsetree::ast::BinOp::OpType::GreaterThanOrEqual:
  case parsetree::ast::BinOp::OpType::LessThan:
  case parsetree::ast::BinOp::OpType::LessThanOrEqual:
    if (lhs->isNumeric() && rhs->isNumeric()) {
      return op.resolveResultType(
          envManager.BuildBasicType(parsetree::ast::BasicType::Type::Boolean));
    }
    throw std::runtime_error("comparison operands are non-numeric");

  case parsetree::ast::BinOp::OpType::Equal:
  case parsetree::ast::BinOp::OpType::NotEqual: {
    if ((lhs->isNumeric() && rhs->isNumeric()) ||
        (lhs->isBoolean() && rhs->isBoolean())) {
      return op.resolveResultType(
          envManager.BuildBasicType(parsetree::ast::BasicType::Type::Boolean));
    }

    auto lhsType =
        std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(lhs);
    auto rhsType =
        std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(rhs);

    if ((lhs->isNull() || lhsType) && (rhs->isNull() || rhsType) &&
        (isValidCast(lhs, rhs) || isValidCast(rhs, lhs))) {
      return op.resolveResultType(
          envManager.BuildBasicType(parsetree::ast::BasicType::Type::Boolean));
    }
    throw std::runtime_error("operands are not of the same type");
  }

  case parsetree::ast::BinOp::OpType::Add:
    if (isTypeString(lhs) || isTypeString(rhs)) {
      return op.resolveResultType(
          envManager.BuildBasicType(parsetree::ast::BasicType::Type::String));
    }
    if (lhs->isNumeric() && rhs->isNumeric()) {
      return op.resolveResultType(
          envManager.BuildBasicType(parsetree::ast::BasicType::Type::Int));
    }
    throw std::runtime_error("invalid types for arithmetic operation");

  case parsetree::ast::BinOp::OpType::And:
  case parsetree::ast::BinOp::OpType::Or:
    if (lhs->isBoolean() && rhs->isBoolean()) {
      return op.resolveResultType(
          envManager.BuildBasicType(parsetree::ast::BasicType::Type::Boolean));
    }
    throw std::runtime_error("logical operation requires boolean operands");

  default:
    throw std::runtime_error("Invalid binary operation");
  }
}

std::shared_ptr<parsetree::ast::Type>
TypeResolver::evalUnOp(UnOp &op,
                       const std::shared_ptr<parsetree::ast::Type> &rhs) const {
  if (auto result = op.resultType(); result) {
    return result;
  }

  switch (op.opType()) {
  case parsetree::ast::UnOp::OpType::Plus:
  case parsetree::ast::UnOp::OpType::Minus:
  case parsetree::ast::UnOp::OpType::Not:
    if (rhs->isNumeric()) {
      return op.resolveResultType(
          envManager.BuildBasicType(parsetree::ast::BasicType::Type::Int));
    }
    break;
  case parsetree::ast::UnOp::OpType::Not:
    if (rhs->isBoolean()) {
      return op.resolveResultType(
          envManager.BuildBasicType(parsetree::ast::BasicType::Type::Boolean));
    }
    break;
  default:
    throw std::runtime_error("Invalid unary operation");
  }

  throw std::runtime_error("Invalid type for unary " +
                           magic_enum::enum_name(op.opType()) + ", is type " +
                           rhs->toString());
}

std::shared_ptr<parsetree::ast::Type> TypeResolver::evalFieldAccess(
    const std::shared_ptr<parsetree::ast::FieldAccess> &op,
    const std::shared_ptr<parsetree::ast::Type> &lhs,
    const std::shared_ptr<parsetree::ast::Type> &field) const {

  if (auto result = op->resultType(); result) {
    return result;
  }
  return op->resolveResultType(field);
}

std::shared_ptr<parsetree::ast::Type> TypeResolver::evalMethodInvocation(
    const std::shared_ptr<parsetree::ast::MethodInvocation> &op,
    const std::shared_ptr<parsetree::ast::Type> &method,
    const std::vector<std::shared_ptr<parsetree::ast::Type>> &args) const {

  if (auto result = op->resultType(); result) {
    return result;
  }

  auto methodType = std::dynamic_pointer_cast<MethodType>(method);
  if (!methodType) {
    throw std::runtime_error("Not a method type");
  }

  const auto &methodParams = methodType->paramTypes();
  if (methodParams.size() != args.size()) {
    throw std::runtime_error("Method params and args size mismatch");
  }

  for (size_t i = 0; i < args.size(); ++i) {
    if (!isAssignableTo(methodParams[i],
                        args[args.size() - 1 - i])) { // Reverse iteration
      throw std::runtime_error("Invalid argument type for method call");
    }
  }

  return op->resolveResultType(methodType->returnType());
}

std::shared_ptr<parsetree::ast::Type> TypeResolver::evalNewObject(
    const std::shared_ptr<parsetree::ast::ClassCreation> &op,
    const std::shared_ptr<parsetree::ast::Type> &object,
    const std::vector<std::shared_ptr<parsetree::ast::Type>> &args) const {

  if (auto result = op->resultType(); result) {
    return result;
  }

  auto constructor = std::dynamic_pointer_cast<MethodType>(object);
  if (!constructor) {
    throw std::runtime_error("Not a method type");
  }

  const auto &constructorParams = constructor->paramTypes();
  if (constructorParams.size() != args.size()) {
    throw std::runtime_error("Constructor params and args size mismatch");
  }

  for (size_t i = 0; i < args.size(); ++i) {
    if (!isAssignableTo(constructorParams[i], args[args.size() - 1 - i])) {
      throw std::runtime_error("Invalid argument type for constructor call");
    }
  }

  return op->resolveResultType(constructor->returnType());
}

std::shared_ptr<parsetree::ast::Type> TypeResolver::evalNewArray(
    const std::shared_ptr<parsetree::ast::ArrayCreation> &op,
    const std::shared_ptr<parsetree::ast::Type> &type,
    const std::shared_ptr<parsetree::ast::Type> &size) const {

  if (auto result = op->resultType(); result) {
    return result;
  }

  if (!size->isNumeric()) {
    throw std::runtime_error("Invalid type for array size, non-numeric");
  }

  return op->resolveResultType(type);
}

std::shared_ptr<parsetree::ast::Type> TypeResolver::evalArrayAccess(
    const std::shared_ptr<parsetree::ast::ArrayAccess> &op,
    const std::shared_ptr<parsetree::ast::Type> &array,
    const std::shared_ptr<parsetree::ast::Type> &index) const {

  if (auto result = op->resultType(); result) {
    return result;
  }

  auto arrayType = std::dynamic_pointer_cast<ArrayType>(array);
  if (!arrayType) {
    throw std::runtime_error("Not an array type");
  }

  if (!index->isNumeric()) {
    throw std::runtime_error("Invalid type for array index, non-numeric");
  }

  return op->resolveResultType(arrayType->getElementType());
}

std::shared_ptr<parsetree::ast::Type> TypeResolver::evalCast(
    const std::shared_ptr<parsetree::ast::Cast> &op,
    const std::shared_ptr<parsetree::ast::Type> &type,
    const std::shared_ptr<parsetree::ast::Type> &value) const {

  if (auto result = op->resultType(); result) {
    return result;
  }

  if (!isValidCast(value, type)) {
    throw std::runtime_error("Invalid cast from " + value->toString() + " to " +
                             type->toString());
  }

  return op->resolveResultType(type);
}

} // namespace static_check
