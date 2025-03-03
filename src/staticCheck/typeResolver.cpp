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
    return refType->getResolvedDecl()->getAstNode() ==
           astManager->java_lang.String;
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

bool isWiderThan(const std::shared_ptr<parsetree::ast::BasicType> &type,
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
  if (lhs == rhs)
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
    if (auto leftClass =
            std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(leftRef)) {
      return isSuperClass(leftClass, astManager->java_lang.String);
    }
    if (auto leftInterface =
            std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(leftRef)) {
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
    if (auto rightClass =
            std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(rightRef)) {
      if (auto leftClass =
              std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(leftRef)) {
        // TODO: need such API
        return isSuperClass(leftClass, rightClass);
      }
      if (auto leftInterface =
              std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                  leftRef)) {
        // TODO: need such API
        return isSuperInterface(leftInterface, rightClass);
      }
    }
    // 3.3 Interface assignability
    if (auto rightInterface =
            std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                rightRef)) {
      if (auto leftClass =
              std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(leftRef)) {
        return leftClass == astManager->java_lang.Object;
      }
      if (auto leftInterface =
              std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                  leftRef)) {
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
      return leftRef->getResolvedDecl()->getAstNode() ==
                 astManager->java_lang.Object ||
             leftRef->getResolvedDecl()->getAstNode() ==
                 astManager->java_lang.Cloneable ||
             leftRef->getResolvedDecl()->getAstNode() ==
                 astManager->java_lang.Serializable;
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

  auto exprRef =
      std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(exprType);
  auto castRef =
      std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(castType);

  // If expr is "null", it is assignable to any reference type
  if (exprType->isNull())
    return static_cast<bool>(castRef);
  if (castType->isNull())
    return static_cast<bool>(exprRef);

  auto exprArr = std::dynamic_pointer_cast<parsetree::ast::ArrayType>(exprType);
  auto castArr = std::dynamic_pointer_cast<parsetree::ast::ArrayType>(castType);

  // Primitive type casting: only numeric conversions are valid
  if (exprType->isPrimitive() && castType->isPrimitive()) {
    return exprType->isNumeric() && castType->isNumeric();
  }

  if (exprRef) {
    if (castArr)
      return exprRef->getResolvedDecl()->getAstNode() ==
             astManager->java_lang.Object;
    if (!castRef)
      return false;

    auto leftInterface =
        std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(exprRef);
    auto rightInterface =
        std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(castRef);
    auto leftClass =
        std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(exprRef);
    auto rightClass =
        std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(castRef);

    if (leftInterface && rightInterface)
      return true;
    if (leftInterface && rightClass && !rightClass->getModifiers()->isFinal())
      return true;
    if (rightInterface && leftClass && !leftClass->getModifiers()->isFinal())
      return true;

    return isAssignableTo(exprRef, castRef) || isAssignableTo(castRef, exprRef);
  }

  if (exprArr) {
    if (castArr) {
      auto leftElem = std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(
          exprArr->getElementType());
      auto rightElem = std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(
          castArr->getElementType());
      return leftElem && rightElem &&
             isValidCast(exprArr->getElementType(), castArr->getElementType());
    }
    if (castRef) {
      if (castRef->getResolvedDecl()->getAstNode() ==
              astManager->java_lang.Object ||
          castRef->getResolvedDecl()->getAstNode() ==
              astManager->java_lang.Serializable) {
        return true;
      }
    }
  }

  throw std::runtime_error("invalid cast from " + exprType->toString() +
                           " to " + castType->toString());
}

std::shared_ptr<parsetree::ast::Type> TypeResolver::mapValue(
    const std::shared_ptr<parsetree::ast::QualifiedName> &value) const {
  std::cout << "mapValue: ";
  value->print(std::cout);

  // FIXME: I am really mixing up decl and type in pseudocode... Hopefully it
  // makes sense
  Decl prevType = nullptr;

  for (int i = 0; i < qualifiedName->size(); ++i) {
    auto simpleName = qualifiedName->get(i);
    std::cout << simpleName->getName() << std::endl;
    // Find the largest i where qualifiedName[i] is disambiguated
    auto decl = simpleName->getResolveDecl();
    if (decl) {
      prevType = decl;
      continue;
    }
    // Now we found our starting point

    // TODO: Add getShouldBeStatic API (in name disambiguation)
    // static field
    if (simpleName->getShouldBeStatic() &&
        prevType.contains(simpleName->getName())) {
      auto fieldDecl =
          prevType.getField(simpleName->getName()) if (!fieldDecl->isStatic()) {
        throw std::runtime_error(
            "Should be static but is actually declared as non-static: "
            << decl->getName())
      }
      // TODO: Check that all accesses of protected fields, methods and
      // constructors are in a subtype of the type declaring the entity being
      // accessed, or in the same package as that type. Maybe something like
      if (fieldDecl->isProtected() && isClassType(fieldDecl) &&
          isSuperClass(prevType, fieldDecl->getType())) {
        // good
      } else {
        // bad
      }
      if (bad) {
        throw error
      }
      simpleName->setResolveDecl(fieldDecl);
      prevType = decl;
    }
    // instance field
    if (!simpleName->getShouldBeStatic() &&
        prevType.contains(simpleName->getName())) {
      auto fieldDecl =
          prevType.getField(simpleName->getName()) if (fieldDecl->isStatic()) {
        throw std::runtime_error(
            "Should be non-static but is actually declared as static: "
            << decl->getName());
      }
      // TODO: protected field access
      simpleName->setResolveDecl(fieldDecl);
      prevType = decl;
    }
  } // for loop

  return prevType;

  // for (int i = 0; i < value->size(); i++) {
  //   auto simpleName = value->get(i);
  //   std::cout << "mapValue: " << simpleName->getName() << std::endl;
  //   if (!simpleName->isDeclResolved())
  //     throw std::runtime_error("SimpleName at mapValue not resolved");
  //   if (auto method = std::dynamic_pointer_cast<parsetree::ast::MethodDecl>(
  //           simpleName->getResolvedDecl())) {
  //     // ToDo
  //     if (method->isConstructor())
  //       std::cout << "MapValue met MethodDecl Constructor" << std::endl;
  //   } else if (!simpleName->isTypeResolved()) {
  //     std::cout << "decl: " << simpleName->getResolvedDecl()->getName()
  //               << std::endl;
  //     throw std::runtime_error("SimpleName at mapValue not type resolved");
  //   }
  // }
  // return value->getLast()->getType();
  // if (!value->isDeclResolved())
  //   throw std::runtime_error("QualifiedName at mapValue not resolved");
  // if (auto method = std::dynamic_pointer_cast<parsetree::ast::MethodDecl>(
  //         value->getResolvedDecl())) {
  //   auto type = std::make_shared<parsetree::ast::MethodType>(method);
  //   if (method->isConstructor()) {
  //     // need double check
  //     //
  //     type->setReturnType(std::make_shared<parsetree::ast::ReferenceType>(method->getMethodBody()));
  //   }
  //   return type;
  // } else {
  //   if (!value->isTypeResolved())
  //     throw std::runtime_error("ExprValue at mapValue not typeresolved");
  //   return value->getType();
  // }
}

////////////////////////////////////////
// Main Logic
////////////////////////////////////////

// check if the format of exprNode vector and this is consistent
// highly possible not
std::shared_ptr<parsetree::ast::Type> TypeResolver::evaluateList(
    const std::vector<std::shared_ptr<parsetree::ast::ExprNode>> &list) {
  while (!op_stack.empty())
    popStack();

  std::cout << "Evaluating " << list.size() << " nodes\n";
  for (int i = 0; i < list.size(); ++i) {
    auto node = list[i];
    if (auto value =
            std::dynamic_pointer_cast<parsetree::ast::QualifiedName>(node)) {
      bool nextIsFieldAccess = false;
      bool nextIsMethodCall = false;
      if (i < list.size() - 1) {
        auto fieldAccess =
            std::dynamic_pointer_cast<parsetree::ast::FieldAccess>(list[i + 1]);
        auto methodCall =
            std::dynamic_pointer_cast<parsetree::ast::MethodInvocation>(
                list[i + 1]);
        if (fieldAccess)
          nextIsFieldAccess = true;
        if (methodCall)
          nextIsMethodCall = true;
      }
      if (nextIsFieldAccess || nextIsMethodCall) {
        // Don't map value yet
        op_stack.push(value);
      } else {
        op_stack.push(mapValue(value));
      }
    } else if (auto unary =
                   std::dynamic_pointer_cast<parsetree::ast::UnOp>(node)) {
      auto rhs = popStack();
      op_stack.push(evalUnOp(unary, rhs));
    } else if (auto binary =
                   std::dynamic_pointer_cast<parsetree::ast::BinOp>(node)) {
      auto rhs = popStack();
      auto lhs = popStack();
      op_stack.push(evalBinOp(binary, lhs, rhs));
    } else if (auto fieldAccess =
                   std::dynamic_pointer_cast<parsetree::ast::FieldAccess>(
                       node)) {
      if (fieldAccess->nargs() == 1) {
        auto field = popStack();
        op_stack.push(evalFieldAccess(fieldAccess, field));
      } else if (fieldAccess->nargs() == 2) {
        auto field = popStack();
        auto lhs = popStack();
        op_stack.push(evalFieldAccess(fieldAccess, field, lhs));
      } else {
        throw std::runtime_error("Field access expects 1 or 2 args")
      }
    } else if (auto methodInvocation =
                   std::dynamic_pointer_cast<parsetree::ast::MethodInvocation>(
                       node)) {
      // Note: reverse order
      // Q: What happens if there are 3 children (primaryExpr, id, arg)?
      auto method_name = popStack();
      std::vector<std::shared_ptr<parsetree::ast::Type>> args(method->nargs() -
                                                              1);
      std::generate(args.rbegin(), args.rend(), [this] { return popStack(); });
      op_stack.push(evalMethodInvocation(method, method_name, args));

      auto methodName = popStack();
      // A better name might be hasNoLHS
      if (methodInvocation->getNeedsDisambiguation()) {
        std::vector<std::shared_ptr<parsetree::ast::Type>> args(
            method->nargs() - 1);
        std::generate(args.rbegin(), args.rend(),
                      [this] { return popStack(); });
        op_stack.push(evalMethodInvocation(methodInvocation, methodName, args));
      } else {
        auto lhs = popStack();
        std::vector<std::shared_ptr<parsetree::ast::Type>> args(
            method->nargs() - 1);
        std::generate(args.rbegin(), args.rend(),
                      [this] { return popStack(); });
        op_stack.push(
            evalMethodInvocation(methodInvocation, methodName, args, lhs));
      }
    } else if (auto newObj =
                   std::dynamic_pointer_cast<parsetree::ast::ClassCreation>(
                       node)) {
      std::cout << "ClassCreation\n";
      std::vector<std::shared_ptr<parsetree::ast::Type>> args(newObj->nargs() -
                                                              1);
      std::generate(args.rbegin(), args.rend(), [this] { return popStack(); });
      auto type = popStack();
      op_stack.push(evalNewObject(newObj, type, args));
    } else if (auto array =
                   std::dynamic_pointer_cast<parsetree::ast::ArrayCreation>(
                       node)) {
      auto size = popStack();
      auto type = popStack();
      op_stack.push(evalNewArray(array, type, size));
    } else if (auto access =
                   std::dynamic_pointer_cast<parsetree::ast::ArrayAccess>(
                       node)) {
      auto index = popStack();
      auto array = popStack();
      op_stack.push(evalArrayAccess(access, array, index));
    } else if (auto cast =
                   std::dynamic_pointer_cast<parsetree::ast::Cast>(node)) {
      auto value = popStack();
      auto type = popStack();
      op_stack.push(evalCast(cast, type, value));
    } else if (auto assignment =
                   std::dynamic_pointer_cast<parsetree::ast::Assignment>(
                       node)) {
      auto lhs = popStack();
      auto rhs = popStack();
      op_stack.push(evalAssignment(assignment, lhs, rhs));
    }
  }
  if (op_stack.size() != 1) {
    throw std::runtime_error("Stack not empty after evaluation!");
  }
  return popStack();
}

std::shared_ptr<parsetree::ast::Type>
TypeResolver::evaluate(const std::shared_ptr<parsetree::ast::Expr> &node) {
  std::cout << "Evaluating ";
  node->print(std::cout);
  std::cout << std::endl;
  return evaluateList(node->getExprNodes());
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

std::shared_ptr<parsetree::ast::Type> TypeResolver::evalBinOp(
    const std::shared_ptr<parsetree::ast::BinOp> &op,
    const std::shared_ptr<parsetree::ast::Type> &lhs,
    const std::shared_ptr<parsetree::ast::Type> &rhs) const {
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
        (lhs->isBoolean() && rhs->isBoolean())) {
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
    throw std::runtime_error("operands are not of the same type");
  }

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
    if (lhs->isBoolean() && rhs->isBoolean()) {
      return op->resolveResultType(
          envManager->BuildBasicType(parsetree::ast::BasicType::Type::Boolean));
    }
    throw std::runtime_error("logical operation requires boolean operands");

  default:
    throw std::runtime_error("Invalid binary operation");
  }
}

std::shared_ptr<parsetree::ast::Type>
TypeResolver::evalUnOp(const std::shared_ptr<parsetree::ast::UnOp> &op,
                       const std::shared_ptr<parsetree::ast::Type> &rhs) const {
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
    const std::shared_ptr<parsetree::ast::FieldAccess> &op,
    const std::shared_ptr<parsetree::ast::QualifiedName> &qualifiedName,
    const std::shared_ptr<parsetree::ast::Type> &lhs) const {
  if (op->nargs() != 1 && op->nargs() != 2) {
    throw std::runtime_error("Field access expects 1 or 2 args");
  }

  std::cout << "Evaluating Field Access: " << std::endl;

  if (auto result = op->getResultType(); result) {
    return result;
  }

  // Case 1: (QualifiedName a.b.c.d) (FieldAccess 1)
  // Start type checking from a

  if (op->nargs() == 1)
    return mapValue(qualifiedName);

  // Case 2: (Type of LHS) (QualifiedName a.b.c.d) (FieldAccess 2)
  // LHS is already type resolved, continue checking field access

  Decl prevType = lhs;

  for (int i = 0; i < qualifiedName->size(); ++i) {
    auto simpleName = qualifiedName->get(i);
    std::cout << simpleName->getName() << std::endl;
    // TODO: getShouldBeStatic API (in name disambiguation)
    // static field
    if (simpleName->getShouldBeStatic() && prevType.contains(simpleName)) {
      auto fieldDecl =
          prevType
              .getField(simpleName->getName()) if (
                  !fieldDecl->isStatic()){throw std::runtime_error(
                  "Should be static but is actually declared as non-static: "
                  << decl->getName())} // TODO: protected field access
          simpleName->setResolveDecl(fieldDecl);
      prevType = decl;
    }
    // instance field
    if (!simpleName->getShouldBeStatic() && prevType.contains(simpleName)) {
      auto fieldDecl =
          prevType.getField(simpleName->getName()) if (fieldDecl->isStatic()) {
        throw std::runtime_error(
            "Should be non-static but is actually declared as static: "
            << decl->getName());
      }
      // TODO: protected field access
      simpleName->setResolveDecl(fieldDecl);
      prevType = decl;
    }
  } // for loop

  return prevType;
}

std::shared_ptr<parsetree::ast::Type> TypeResolver::evalMethodInvocation(
    const std::shared_ptr<parsetree::ast::MethodInvocation> &op,
    const std::shared_ptr<parsetree::ast::QualifiedName> &qualifiedName,
    const std::vector<std::shared_ptr<parsetree::ast::Type>> &args,
    const std::shared_ptr<parsetree::ast::Type> &lhs) const {
  if (auto result = op->getResultType(); result) {
    return result;
  }

  // Case 1: (qualified_name LPAREN args RPAREN)
  // (QualifiedName a.b.c.m) (MethodInvocation needsAmbiguation=true)
  // For a.b.c, this is just like other qualified name

  if (!lhs) {
    prevType = mapValue(qualifiedName [0:(n - 1)]);
  }

  // Case 2: (primary DOT ID LPAREN args RPAREN)
  // (QualifiedName a.b.c) (FieldAccess 1) (QualifiedName m) (MethodInvocation
  // needsAmbiguation=false)
  //    => (Type of LHS) (QualifiedName m) (MethodInvocation)
  // a.b.c should have been handled by evalFieldAccess

  else {
    prevType = lhs;
  }

  // prevType is the starting type. Now we check for method m
  std::string m = qualifiedName->getLastName();
  // is m contained in prevType?
  auto methodDecl = prevType->findMethodDecl(m);
  if (methodDecl) {
    auto params = methodDecl->getParams();
    // Check args and params assignability...
    for (size_t i = 0; i < args.size(); ++i) {
      // TODO
    }
  }
  // TODO: static / non-static access
  // TODO: protected access
  // TODO: constructor check

  // If everything is good
  return methodDecl->getMethodReturnType();

  // auto methodType =
  //     std::dynamic_pointer_cast<parsetree::ast::MethodType>(method);
  // if (!methodType) {
  //   throw std::runtime_error("Not a method type");
  // }

  // const auto &methodParams = methodType->getParamTypes();
  // if (methodParams.size() != args.size()) {
  //   throw std::runtime_error("Method params and args size mismatch");
  // }

  // for (size_t i = 0; i < args.size(); ++i) {
  //   if (!isAssignableTo(methodParams[i],
  //                       args[args.size() - 1 - i])) { // Reverse iteration
  //     throw std::runtime_error("Invalid argument type for method call");
  //   }
  // }

  // return op->resolveResultType(methodType->getReturnType());
}

std::shared_ptr<parsetree::ast::Type> TypeResolver::evalNewObject(
    const std::shared_ptr<parsetree::ast::ClassCreation> &op,
    const std::shared_ptr<parsetree::ast::Type> &object,
    const std::vector<std::shared_ptr<parsetree::ast::Type>> &args) const {

  if (auto result = op->getResultType(); result) {
    return result;
  }

  auto constructor =
      std::dynamic_pointer_cast<parsetree::ast::MethodType>(object);
  if (!constructor) {
    throw std::runtime_error("Not a method type");
  }

  const auto &constructorParams = constructor->getParamTypes();
  if (constructorParams.size() != args.size()) {
    throw std::runtime_error("Constructor params and args size mismatch");
  }

  for (size_t i = 0; i < args.size(); ++i) {
    if (!isAssignableTo(constructorParams[i], args[args.size() - 1 - i])) {
      throw std::runtime_error("Invalid argument type for constructor call");
    }
  }

  return op->resolveResultType(constructor->getReturnType());
}

std::shared_ptr<parsetree::ast::Type> TypeResolver::evalNewArray(
    const std::shared_ptr<parsetree::ast::ArrayCreation> &op,
    const std::shared_ptr<parsetree::ast::Type> &type,
    const std::shared_ptr<parsetree::ast::Type> &size) const {

  if (auto result = op->getResultType(); result) {
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

std::shared_ptr<parsetree::ast::Type> TypeResolver::evalCast(
    const std::shared_ptr<parsetree::ast::Cast> &op,
    const std::shared_ptr<parsetree::ast::Type> &type,
    const std::shared_ptr<parsetree::ast::Type> &value) const {

  if (auto result = op->getResultType(); result) {
    return result;
  }

  if (!isValidCast(value, type)) {
    throw std::runtime_error("Invalid cast from " + value->toString() + " to " +
                             type->toString());
  }

  return op->resolveResultType(type);
}

std::shared_ptr<parsetree::ast::Type> TypeResolver::evalAssignment(
    const std::shared_ptr<parsetree::ast::Assignment> &op,
    const std::shared_ptr<parsetree::ast::Type> &lhs,
    const std::shared_ptr<parsetree::ast::Type> &rhs) const {
  if (isAssignableTo(lhs, rhs)) {
    return op->resolveResultType(lhs);
  }
  throw std::runtime_error("assignment is not valid");
}

} // namespace static_check
