#include "staticCheck/typeResolver.hpp"

namespace static_check {

void TypeResolver::resolve() {
  for (auto ast : astManager->getASTs()) {
    resolveAST(ast);
  }
}

bool TypeResolver::isReferenceOrArrType(
    std::shared_ptr<parsetree::ast::Type> type) const {
  return std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(type) ||
         std::dynamic_pointer_cast<parsetree::ast::ArrayType>(type) ||
         type->isString();
}

// Check if is Java string
bool TypeResolver::isTypeString(
    std::shared_ptr<parsetree::ast::Type> type) const {
  if (type->isString())
    return true;
  if (auto refType =
          std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(type)) {
    return refType->getResolvedDecl().getAstNode() ==
           astManager->java_lang.String;
  }
  return false;
}

// Taken from HierarchyCheck
static bool isClass(std::shared_ptr<parsetree::ast::AstNode> decl) {
  return !!std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(decl);
}

// Taken from HierarchyCheck
static bool isInterface(std::shared_ptr<parsetree::ast::AstNode> decl) {
  return !!std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(decl);
}

bool TypeResolver::isSuperClass(
    std::shared_ptr<parsetree::ast::AstNode> super,
    std::shared_ptr<parsetree::ast::AstNode> child) const {
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
    if (!superClass || !superClass->getResolvedDecl().getAstNode())
      continue;
    // Cast to class
    auto superClassDecl = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
        superClass->getResolvedDecl().getAstNode());

    if (superClassDecl == superDecl)
      return true;

    if (isSuperClass(super, std::dynamic_pointer_cast<parsetree::ast::AstNode>(
                                superClassDecl)))
      return true;
  }
  return false;
}

static bool isSuperInterface(std::shared_ptr<parsetree::ast::AstNode> interface,
                             std::shared_ptr<parsetree::ast::AstNode> child) {
  if (!child || !interface) {
    return false;
  }
  if (!isClass(child) && !isInterface(child)) {
    return false;
  }
  if (!isInterface(interface)) {
    return false;
  }
  auto interfaceDecl =
      std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(interface);
  if (isClass(child)) {
    auto childDecl =
        std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(child);
    for (auto &superInterface : childDecl->getInterfaces()) {
      if (!superInterface || !superInterface->getResolvedDecl().getAstNode())
        continue;
      // Cast to class
      auto superInterfaceDecl =
          std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
              superInterface->getResolvedDecl().getAstNode());

      if (superInterfaceDecl == interface)
        return true;
      if (isSuperInterface(interface,
                           std::dynamic_pointer_cast<parsetree::ast::AstNode>(
                               superInterfaceDecl)))
        return true;
    }
    for (auto &superClass : childDecl->getSuperClasses()) {
      if (!superClass || !superClass->getResolvedDecl().getAstNode())
        continue;

      // Cast to class
      auto superClassDecl =
          std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
              superClass->getResolvedDecl().getAstNode());

      if (isSuperInterface(interface,
                           std::dynamic_pointer_cast<parsetree::ast::AstNode>(
                               superClassDecl)))
        return true;
    }
  } else {
    auto childDecl =
        std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(child);
    for (auto &superInterface : childDecl->getInterfaces()) {
      if (!superInterface || !superInterface->getResolvedDecl().getAstNode())
        continue;
      // Cast to interface
      auto superInterfaceDecl =
          std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
              superInterface->getResolvedDecl().getAstNode());

      if (superInterfaceDecl == interface)
        return true;
      if (isSuperInterface(interface,
                           std::dynamic_pointer_cast<parsetree::ast::AstNode>(
                               superInterfaceDecl)))
        return true;
    }
  }
  return false;
}

/**
 * Determines if one primitive type is wider than another based on Java's
 * widening primitive conversions.
 *
 * This function follows the rules specified in section 5.1.2. It considers the
 * predefined widening conversions:
 *
 * - byte  → short, int, long, float, double
 * - short → int, long, float, double
 * - char  → int, long, float, double
 * - int   → long, float, double
 * - long  → float, double
 * - float → double
 *
 * Widening conversions do not lose information about the overall magnitude of a
 * numeric value, although conversions to floating-point types may lose
 * precision. However, they never result in a runtime exception.
 */
static bool
isWiderThan(const std::shared_ptr<parsetree::ast::BasicType> &type,
            const std::shared_ptr<parsetree::ast::BasicType> &other) {
  using Type = parsetree::ast::BasicType::Type;

  Type otherType = other->getType();
  Type typeType = type->getType();
  switch (otherType) {
  case Type::Char:
    return typeType == Type::Int;
  case Type::Short:
    return typeType == Type::Int;
  case Type::Byte:
    return typeType == Type::Short || typeType == Type::Int;
  default:
    return false;
  }
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

  if (*lhs == *rhs)
    return true;

  auto leftPrimitive =
      std::dynamic_pointer_cast<parsetree::ast::BasicType>(lhs);
  auto rightPrimitive =
      std::dynamic_pointer_cast<parsetree::ast::BasicType>(rhs);
  auto leftRef = std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(lhs);
  auto rightRef = std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(rhs);
  auto leftArr = std::dynamic_pointer_cast<parsetree::ast::ArrayType>(lhs);
  auto rightArr = std::dynamic_pointer_cast<parsetree::ast::ArrayType>(rhs);

  // Identity conversion: Java astManager->java_lang.String <-> primitive
  // astManager->java_lang.String
  if (isTypeString(lhs) && isTypeString(rhs))
    return true;

  // astManager->java_lang.String conversions
  if (rhs->isString() && leftRef) {
    if (auto leftClass = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
            leftRef->getResolvedDecl().getAstNode())) {
      return isSuperClass(leftClass, astManager->java_lang.String);
    }
    if (auto leftInterface =
            std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                leftRef->getResolvedDecl().getAstNode())) {
      return isSuperInterface(leftInterface, astManager->java_lang.String);
    }
    return false;
  }

  if (lhs->isString() && rightRef) {
    if (auto rightClass =
            std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(rightRef)) {
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
    if (auto rightClass = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
            rightRef->getResolvedDecl().getAstNode())) {
      if (auto leftClass = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
              leftRef->getResolvedDecl().getAstNode())) {
        return isSuperClass(leftClass, rightClass);
      }
      if (auto leftInterface =
              std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                  leftRef->getResolvedDecl().getAstNode())) {
        return isSuperInterface(leftInterface, rightClass);
      }
    }
    // 3.3 Interface assignability
    if (auto rightInterface =
            std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                rightRef->getResolvedDecl().getAstNode())) {
      if (auto leftClass = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
              leftRef->getResolvedDecl().getAstNode())) {
        return leftClass == astManager->java_lang.Object;
      }
      if (auto leftInterface =
              std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                  leftRef->getResolvedDecl().getAstNode())) {
        return isSuperInterface(leftInterface, rightInterface);
      }
    }
  }

  // 3.4 Array assignment rules
  if (rightArr) {
    if (leftArr) {
      auto leftElem = std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(
          leftArr->getElementType());
      auto rightElem = std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(
          rightArr->getElementType());
      return leftElem && rightElem && isAssignableTo(leftElem, rightElem);
    }

    if (leftRef) {
      // Serializable decl
      return leftRef->getResolvedDecl().getAstNode() ==
                 astManager->java_lang.Object ||
             leftRef->getResolvedDecl().getAstNode() ==
                 astManager->java_lang.Cloneable ||
             leftRef->getResolvedDecl().getAstNode() ==
                 astManager->java_lang.Serializable;
    }
  }

  return false;
}

bool TypeResolver::isValidCast(
    const std::shared_ptr<parsetree::ast::Type> &exprType,
    const std::shared_ptr<parsetree::ast::Type> &castType) const {
  // If both types are the same, the cast is trivially valid
  if (exprType == castType)
    return true;

  // Identity conversion: Java's String type is always castable to itself.
  // Also, null can be cast to any reference type.
  if (isTypeString(exprType) && isTypeString(castType) || exprType->isNull())
    return true;

  // Check if the types are assignable to each other (widening or narrowing
  // reference conversions)
  if (isAssignableTo(exprType, castType) ||
      isAssignableTo(castType, exprType)) {
    return true;
  }

  auto exprRef =
      std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(exprType);
  auto castRef =
      std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(castType);

  // null can be cast to any reference type
  if (exprType->isNull())
    return static_cast<bool>(castRef);
  if (castType->isNull())
    return static_cast<bool>(exprRef);

  auto exprArr = std::dynamic_pointer_cast<parsetree::ast::ArrayType>(exprType);
  auto castArr = std::dynamic_pointer_cast<parsetree::ast::ArrayType>(castType);

  // If both types are primitive, only numeric type conversions are valid
  if (exprType->isPrimitive() && castType->isPrimitive()) {
    return exprType->isNumeric() && castType->isNumeric();
  }

  // Handle casting between reference types
  if (exprRef) {
    // If casting to an array, it must be a cast to `Object`, which is valid
    if (castArr)
      return exprRef->getResolvedDecl().getAstNode() ==
             astManager->java_lang.Object;
    // If the cast type is not a reference type, the cast is invalid
    if (!castRef)
      return false;

    auto leftInterface =
        std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
            exprRef->getResolvedDecl().getAstNode());
    auto rightInterface =
        std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
            castRef->getResolvedDecl().getAstNode());
    auto leftClass = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
        exprRef->getResolvedDecl().getAstNode());
    auto rightClass = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
        castRef->getResolvedDecl().getAstNode());

    // Interface-to-interface casts are always valid
    if (leftInterface && rightInterface)
      return true;

    // Class-to-interface or interface-to-class casts are valid if the class is
    // not final
    if (leftInterface && rightClass && !rightClass->getModifiers()->isFinal())
      return true;
    if (rightInterface && leftClass && !leftClass->getModifiers()->isFinal())
      return true;

    // Otherwise, use assignability rules for reference types
    return isAssignableTo(exprRef, castRef) || isAssignableTo(castRef, exprRef);
  }

  // Handle array type casting
  if (exprArr) {
    if (castArr) {
      // If both are arrays, check if their element types are castable
      auto leftElem = std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(
          exprArr->getElementType());
      auto rightElem = std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(
          castArr->getElementType());
      return leftElem && rightElem &&
             isValidCast(exprArr->getElementType(), castArr->getElementType());
    }
    // Arrays can be cast to Object or Serializable
    if (castRef) {
      if (castRef->getResolvedDecl().getAstNode() ==
              astManager->java_lang.Object ||
          castRef->getResolvedDecl().getAstNode() ==
              astManager->java_lang.Serializable) {
        return true;
      }
    }
  }

  // Otherwise, the cast is invalid
  throw std::runtime_error("invalid cast from " + exprType->toString() +
                           " to " + castType->toString());
}

std::shared_ptr<parsetree::ast::Type>
TypeResolver::mapValue(std::shared_ptr<parsetree::ast::ExprValue> &value) {

  if (!(value->isDeclResolved()))
    throw std::runtime_error("ExprValue at mapValue not decl resolved");

  if (auto method = std::dynamic_pointer_cast<parsetree::ast::MethodDecl>(
          value->getResolvedDecl())) {
    auto type = std::make_shared<parsetree::ast::MethodType>(method);
    // If the method is a constructor, its return type should be the class it
    // belongs to.
    if (method->isConstructor()) {
      auto retType = std::make_shared<parsetree::ast::ReferenceType>(
          method->getParent()->asDecl());
      retType->setResolvedDecl(Decl{method->getParent()->asDecl()});
      type->setReturnType(retType);
    }
    // Return the resolved method type.
    return type;
  } else {
    if (!(value->isTypeResolved()))
      throw std::runtime_error("ExprValue at mapValue not type resolved");

    // Return the resolved type of the expression value.
    return value->getType();
  }
}

void TypeResolver::resolveAST(
    const std::shared_ptr<parsetree::ast::AstNode> &node) {
  if (!node)
    throw std::runtime_error("Node is null when resolving AST");
  // only check Expr
  if (auto expr = std::dynamic_pointer_cast<parsetree::ast::Expr>(node)) {
    evaluate(expr);
  } else {
    for (const auto &child : node->getChildren()) {
      if (!child)
        continue;
      resolveAST(child);
    }
  }
}

std::shared_ptr<parsetree::ast::Type>
TypeResolver::evalBinOp(std::shared_ptr<parsetree::ast::BinOp> &op,
                        const std::shared_ptr<parsetree::ast::Type> lhs,
                        const std::shared_ptr<parsetree::ast::Type> rhs) {

  if (!lhs || !rhs)
    throw std::runtime_error("BinOp operands are null");
  if (auto result = op->getResultType(); result) {
    return result;
  }

  switch (op->getOp()) {
  case parsetree::ast::BinOp::OpType::Assign:
    if (isAssignableTo(lhs, rhs)) {
      return op->resolveResultType(lhs);
    }
    throw std::runtime_error("assignment is not valid");

  case parsetree::ast::BinOp::OpType::GreaterThan:
  case parsetree::ast::BinOp::OpType::GreaterThanOrEqual:
  case parsetree::ast::BinOp::OpType::LessThan:
  case parsetree::ast::BinOp::OpType::LessThanOrEqual:
    if (lhs->isNumeric() && rhs->isNumeric()) {
      return op->resolveResultType(
          envManager->BuildBasicType(parsetree::ast::BasicType::Type::Boolean));
    }
    throw std::runtime_error("comparison operands are non-numeric");

  case parsetree::ast::BinOp::OpType::Equal:
  case parsetree::ast::BinOp::OpType::NotEqual: {
    if ((lhs->isNumeric() && rhs->isNumeric()) ||
        (lhs->isBoolean() && rhs->isBoolean()) ||
        (lhs->isString() && rhs->isString())) {
      return op->resolveResultType(
          envManager->BuildBasicType(parsetree::ast::BasicType::Type::Boolean));
    }

    auto lhsType =
        std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(lhs);
    auto rhsType =
        std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(rhs);

    if ((lhs->isNull() || lhsType) && (rhs->isNull() || rhsType) &&
        (isValidCast(lhs, rhs) || isValidCast(rhs, lhs))) {
      return op->resolveResultType(
          envManager->BuildBasicType(parsetree::ast::BasicType::Type::Boolean));
    }
    std::cout << "lhs: ";
    lhs->print(std::cout);
    std::cout << "rhs: ";
    rhs->print(std::cout);
    throw std::runtime_error("operands are not of the same type");
  }

  // FIXME: Duplicate BinOp? didn't have time to fix
  case parsetree::ast::BinOp::OpType::Plus:
  case parsetree::ast::BinOp::OpType::Add:
    if (isTypeString(lhs) || isTypeString(rhs)) {
      return op->resolveResultType(
          envManager->BuildBasicType(parsetree::ast::BasicType::Type::String));
    }
    if (lhs->isNumeric() && rhs->isNumeric()) {
      return op->resolveResultType(
          envManager->BuildBasicType(parsetree::ast::BasicType::Type::Int));
    }
    throw std::runtime_error("invalid types for arithmetic operation");

  case parsetree::ast::BinOp::OpType::And:
  case parsetree::ast::BinOp::OpType::Or:
  case parsetree::ast::BinOp::OpType::BitWiseAnd:
  case parsetree::ast::BinOp::OpType::BitWiseOr:
    if (lhs->isBoolean() && rhs->isBoolean()) {
      return op->resolveResultType(
          envManager->BuildBasicType(parsetree::ast::BasicType::Type::Boolean));
    }
    throw std::runtime_error("logical operation requires boolean operands");

  // FIXME: Duplicate BinOp? didn't have time to fix
  case parsetree::ast::BinOp::OpType::Minus:
  case parsetree::ast::BinOp::OpType::Subtract:
  case parsetree::ast::BinOp::OpType::Multiply:
  case parsetree::ast::BinOp::OpType::Divide:
  case parsetree::ast::BinOp::OpType::Modulo: {
    if (lhs->isNumeric() && rhs->isNumeric()) {
      return op->resolveResultType(
          envManager->BuildBasicType(parsetree::ast::BasicType::Type::Int));
    }
    throw std::runtime_error("invalid types for arithmetic operation");
  }

  case parsetree::ast::BinOp::OpType::InstanceOf: {
    if ((lhs->isNull() || isReferenceOrArrType(lhs)) &&
        (isReferenceOrArrType(rhs)) && isValidCast(rhs, lhs)) {
      op->setLhsType(lhs);
      op->setRhsType(rhs);
      return op->resolveResultType(
          envManager->BuildBasicType(parsetree::ast::BasicType::Type::Boolean));
    }
  }

  default:
    throw std::runtime_error("Invalid binary operation");
  }
}

std::shared_ptr<parsetree::ast::Type>
TypeResolver::evalUnOp(std::shared_ptr<parsetree::ast::UnOp> &op,
                       const std::shared_ptr<parsetree::ast::Type> rhs) {
  if (auto result = op->getResultType(); result) {
    return result;
  }

  switch (op->getOp()) {
  case parsetree::ast::UnOp::OpType::Plus:
  case parsetree::ast::UnOp::OpType::Minus:
    // case parsetree::ast::UnOp::OpType::BitWiseNot:
    if (rhs->isNumeric()) {
      return op->resolveResultType(
          envManager->BuildBasicType(parsetree::ast::BasicType::Type::Int));
    }
    break;
  case parsetree::ast::UnOp::OpType::Not:
    if (rhs->isBoolean()) {
      return op->resolveResultType(
          envManager->BuildBasicType(parsetree::ast::BasicType::Type::Boolean));
    }
    break;
  default:
    throw std::runtime_error("Invalid unary operation");
  }

  throw std::runtime_error("Invalid type for unary " +
                           std::string(magic_enum::enum_name(op->getOp())) +
                           ", is type " + rhs->toString());
}

std::shared_ptr<parsetree::ast::Type> TypeResolver::evalFieldAccess(
    std::shared_ptr<parsetree::ast::FieldAccess> &op,
    const std::shared_ptr<parsetree::ast::Type> lhs,
    const std::shared_ptr<parsetree::ast::Type> field) {

  if (auto result = op->getResultType(); result) {
    return result;
  }
  return op->resolveResultType(field);
}

std::shared_ptr<parsetree::ast::Type> TypeResolver::evalMethodInvocation(
    std::shared_ptr<parsetree::ast::MethodInvocation> &op,
    const std::shared_ptr<parsetree::ast::Type> method,
    const std::vector<std::shared_ptr<parsetree::ast::Type>> &args) {

  if (auto result = op->getResultType(); result) {
    return result;
  }

  auto methodType =
      std::dynamic_pointer_cast<parsetree::ast::MethodType>(method);
  if (!methodType) {
    throw std::runtime_error("Not a method type");
  }

  const auto &methodParams = methodType->getParamTypes();
  if (methodParams.size() != args.size()) {
    throw std::runtime_error("Method params and args size mismatch");
  }

  for (size_t i = 0; i < args.size(); ++i) {
    if (!isAssignableTo(methodParams[i],
                        args[args.size() - 1 - i])) { // Reverse iteration
      throw std::runtime_error("Invalid argument type for method call");
    }
  }

  return op->resolveResultType(methodType->getReturnType());
}

std::shared_ptr<parsetree::ast::Type> TypeResolver::evalNewObject(
    std::shared_ptr<parsetree::ast::ClassCreation> &op,
    const std::shared_ptr<parsetree::ast::Type> object,
    const std::vector<std::shared_ptr<parsetree::ast::Type>> &args) {

  // Check if the result type has already been computed and return it if
  // available.
  if (auto result = op->getResultType(); result) {
    return result;
  }

  // Ensure the provided object type is actually a method type (constructor).
  auto constructor =
      std::dynamic_pointer_cast<parsetree::ast::MethodType>(object);
  if (!constructor) {
    throw std::runtime_error("Not a method type");
  }

  const auto &constructorParams = constructor->getParamTypes();
  if (constructorParams.size() != args.size()) {
    throw std::runtime_error("Constructor params and args size mismatch");
  }

  // for (size_t i = 0; i < args.size(); ++i) {
  //   std::cout << "expected " << constructorParams[i]->toString() << " and got
  //   "
  //             << args[args.size() - 1 - i]->toString() << std::endl;
  // }

  // Validate that each argument type matches the expected parameter type.
  for (size_t i = 0; i < args.size(); ++i) {
    if (!isAssignableTo(constructorParams[i], args[args.size() - 1 - i])) {
      throw std::runtime_error("Invalid argument type for constructor call: " +
                               constructorParams[i]->toString() + " but got " +
                               args[args.size() - 1 - i]->toString());
    }
  }

  return op->resolveResultType(constructor->getReturnType());
}

std::shared_ptr<parsetree::ast::Type>
TypeResolver::evalNewArray(std::shared_ptr<parsetree::ast::ArrayCreation> &op,
                           const std::shared_ptr<parsetree::ast::Type> type,
                           const std::shared_ptr<parsetree::ast::Type> size) {

  if (auto result = op->getResultType(); result) {
    return result;
  }

  if (!size->isNumeric()) {
    throw std::runtime_error("Invalid type for array size, non-numeric");
  }

  return op->resolveResultType(type);
}

std::shared_ptr<parsetree::ast::Type> TypeResolver::evalArrayAccess(
    std::shared_ptr<parsetree::ast::ArrayAccess> &op,
    const std::shared_ptr<parsetree::ast::Type> array,
    const std::shared_ptr<parsetree::ast::Type> index) {

  if (auto result = op->getResultType(); result) {
    return result;
  }

  auto arrayType = std::dynamic_pointer_cast<parsetree::ast::ArrayType>(array);
  if (!arrayType) {
    throw std::runtime_error("Not an array type");
  }

  if (!index->isNumeric()) {
    throw std::runtime_error("Invalid type for array index, non-numeric");
  }

  return op->resolveResultType(arrayType->getElementType());
}

std::shared_ptr<parsetree::ast::Type>
TypeResolver::evalCast(std::shared_ptr<parsetree::ast::Cast> &op,
                       const std::shared_ptr<parsetree::ast::Type> type,
                       const std::shared_ptr<parsetree::ast::Type> value) {

  if (auto result = op->getResultType(); result) {
    return result;
  }

  if (!isValidCast(value, type)) {
    throw std::runtime_error("In evalCast Invalid cast from " +
                             value->toString() + " to " + type->toString());
  }

  return op->resolveResultType(type);
}

std::shared_ptr<parsetree::ast::Type>
TypeResolver::evalAssignment(std::shared_ptr<parsetree::ast::Assignment> &op,
                             const std::shared_ptr<parsetree::ast::Type> lhs,
                             const std::shared_ptr<parsetree::ast::Type> rhs) {

  if (isAssignableTo(lhs, rhs)) {
    return op->resolveResultType(lhs);
  }
  throw std::runtime_error("assignment is not valid");
}

} // namespace static_check
