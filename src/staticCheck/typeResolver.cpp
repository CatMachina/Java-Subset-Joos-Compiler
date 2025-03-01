#include "staticCheck/typeResolver.hpp"
#include "staticCheck/environment.hpp"

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
    return refType->decl() == String; // get the java lang String decl!
  }
  return false;
}

// Taken from HierarchyCheck
bool isClass(std::shared_ptr<parsetree::ast::AstNode> decl)
{
  return !!dynamic_pointer_cast<parsetree::ast::ClassDecl>(decl);
}

// Taken from HierarchyCheck
bool isInterface(std::shared_ptr<parsetree::ast::AstNode> decl)
{
  return !!dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(decl);
}

bool isSuperClass(std::shared_ptr<parsetree::ast::AstNode> child, std::shared_ptr<parsetree::ast::AstNode> super) {
  if (!child || !super)
  {
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
  auto childDecl = dynamic_pointer_cast<parsetree::ast::ClassDecl>(child);
  auto superDecl = dynamic_pointer_cast<parsetree::ast::ClassDecl>(super);
  for (auto &superClass : childDecl->getSuperClasses())
  {
    if (!superClass || !superClass->getResolvedDecl() || !superClass->getResolvedDecl()->getAstNode())
      continue;

    // Cast to class
    auto superClassDecl =
        dynamic_pointer_cast<parsetree::ast::ClassDecl>(
            superClass->getResolvedDecl()->getAstNode());

    if (superClassDecl == superDecl)
      return true;
        
    if(isSuperClass(dynamic_pointer_cast<parsetree::ast::AstNode>(superClassDecl), super))
      return true;
  }
  return false;
}

bool isSuperInterface(std::shared_ptr<parsetree::ast::AstNode> child, std::shared_ptr<parsetree::ast::AstNode> interface) {
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
  auto interfaceDecl = dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(interface);
  if(isClass(child)) {
    auto childDecl = dynamic_pointer_cast<parsetree::ast::ClassDecl>(child);
    for (auto &superInterface : childDecl->getInterfaces()) {
      if (!superInterface || !superInterface->getResolvedDecl() || !superInterface->getResolvedDecl()->getAstNode())
        continue;
      // Cast to class
      auto superInterfaceDecl =
          dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
              superInterface->getResolvedDecl()->getAstNode());
  
      if (superInterfaceDecl == interface)
        return true;
      if (isSuperInterface(dynamic_pointer_cast<parsetree::ast::AstNode>(superInterfaceDecl), interface))
        return true;
    }
    for (auto &superClass : childDecl->getSuperClasses())
    {
      if (!superClass || !superClass->getResolvedDecl() || !superClass->getResolvedDecl()->getAstNode())
        continue;
  
      // Cast to class
      auto superClassDecl =
          dynamic_pointer_cast<parsetree::ast::ClassDecl>(
              superClass->getResolvedDecl()->getAstNode());
          
      if(isSuperInterface(dynamic_pointer_cast<parsetree::ast::AstNode>(superClassDecl), interface))
        return true;
    }
  } else {
    auto childDecl = dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(child);
    for (auto &superInterface : childDecl->getInterfaces())
    {
      if (!superInterface || !superInterface->getResolvedDecl() || !superInterface->getResolvedDecl()->getAstNode())
        continue;
      // Cast to interface
      auto superInterfaceDecl =
          dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
              superInterface->getResolvedDecl()->getAstNode());

      if (superInterfaceDecl == interface)
        return true;
      if (isSuperInterface(dynamic_pointer_cast<parsetree::ast::AstNode>(superInterfaceDecl), interface))
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
bool TypeResolver::isAssignableTo(const std::shared_ptr<Type> &lhs,
                                  const std::shared_ptr<Type> &rhs) const {
  if (*lhs == *rhs)
    return true;

  auto asBuiltInType = [](std::shared_ptr<Type> &t) {
    return std::dynamic_pointer_cast<ast::BuiltInType>(t);
  };
  auto asReferenceType = [](std::shared_ptr<Type> &t) {
    return std::dynamic_pointer_cast<ast::ReferenceType>(t);
  };
  auto asArrayType = [](std::shared_ptr<Type> &t) {
    return std::dynamic_pointer_cast<ast::ArrayType>(t);
  };
  auto asClassDecl = [](std::shared_ptr<ast::ReferenceType> &ref) {
    return std::dynamic_pointer_cast<ClassDecl>(ref->decl());
  };
  auto asInterfaceDecl = [](std::shared_ptr<ast::ReferenceType> &ref) {
    return std::dynamic_pointer_cast<InterfaceDecl>(ref->decl());
  };

  auto leftPrimitive = asBuiltInType(lhs);
  auto rightPrimitive = asBuiltInType(rhs);
  auto leftRef = asReferenceType(lhs);
  auto rightRef = asReferenceType(rhs);
  auto leftArr = asArrayType(lhs);
  auto rightArr = asArrayType(rhs);

  // Identity conversion: Java String <-> primitive string
  if (isTypeString(lhs) && isTypeString(rhs))
    return true;

  // String conversions
  if (rhs->isString() && leftRef) {
    // TODO: need such API of isSuperClass and isSuperInterface
    // String is the javalang String class
    if (auto leftClass = asClassDecl(leftRef)) {
      return isSuperClass(leftClass, String);
    }
    if (auto leftInterface = asInterfaceDecl(leftRef)) {
      return isSuperInterface(leftInterface, String);
    }
    return false;
  }

  if (lhs->isString() && rightRef) {
    if (auto rightClass = asClassDecl(rightRef)) {
      return isSuperClass(String, rightClass);
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
        return leftClass == Object; // java lang Object decl
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
      // ToDo: we need to have a way to get java lang Object Cloneable and
      // Serializable decl
      return leftRef->decl() == Object || leftRef->decl() == Cloneable ||
             leftRef->decl() == Serializable;
    }
  }

  return false;
}

bool ExprTypeResolver::isValidCast(
    const std::shared_ptr<Type> &exprType,
    const std::shared_ptr<Type> &castType) const {
  if (*exprType == *castType)
    return true;

  // Identity conversion: Java String <-> primitive string
  if (isTypeString(exprType) && isTypeString(castType))
    return true;

  if (isAssignableTo(exprType, castType) ||
      isAssignableTo(castType, exprType)) {
    return true;
  }

  // Helper lambdas for type casting
  auto asReferenceType = [](const std::shared_ptr<Type> &t) {
    return std::dynamic_pointer_cast<ast::ReferenceType>(t);
  };
  auto asArrayType = [](const std::shared_ptr<Type> &t) {
    return std::dynamic_pointer_cast<ast::ArrayType>(t);
  };
  auto asClassDecl = [](const std::shared_ptr<ast::ReferenceType> &ref) {
    return std::dynamic_pointer_cast<ast::ClassDecl>(ref->decl());
  };
  auto asInterfaceDecl = [](const std::shared_ptr<ast::ReferenceType> &ref) {
    return std::dynamic_pointer_cast<ast::InterfaceDecl>(ref->decl());
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
      return exprRef->decl() == Object; // java lang Object
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
      // Object and Serializable are java lang decls
      if (castRef->decl() == Object || castRef->decl() == Serializable) {
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

/////////////////////////////////////
// Evals of each op
////////////////////////////////////

std::shared_ptr<Type> ExprTypeResolver::mapValue(ExprValue &node) const {
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

std::shared_ptr<Type>
ExprTypeResolver::evalBinOp(BinOp &op, const std::shared_ptr<Type> &lhs,
                            const std::shared_ptr<Type> &rhs) const {
  if (op.resultType())
    return op.resultType();

  switch (op.opType()) {
  case BinOp::OpType::Assignment:
    if (isAssignableTo(lhs, rhs)) {
      return op.resolveResultType(lhs);
    }
    throw std::runtime_error("assignment is not valid");

  case BinOp::OpType::GreaterThan:
  case BinOp::OpType::GreaterThanOrEqual:
  case BinOp::OpType::LessThan:
  case BinOp::OpType::LessThanOrEqual:
    if (lhs->isNumeric() && rhs->isNumeric()) {
      return op.resolveResultType(
          envManager.BuildBuiltInType(ast::BuiltInType::Kind::Boolean));
    }
    throw std::runtime_error("comparison operands are non-numeric");

  case BinOp::OpType::Equal:
  case BinOp::OpType::NotEqual: {
    if ((lhs->isNumeric() && rhs->isNumeric()) ||
        (lhs->isBoolean() && rhs->isBoolean())) {
      return op.resolveResultType(
          envManager.BuildBuiltInType(ast::BuiltInType::Kind::Boolean));
    }

    auto lhsType = std::dynamic_pointer_cast<ast::ReferenceType>(lhs);
    auto rhsType = std::dynamic_pointer_cast<ast::ReferenceType>(rhs);

    if ((lhs->isNull() || lhsType) && (rhs->isNull() || rhsType) &&
        (isValidCast(lhs, rhs) || isValidCast(rhs, lhs))) {
      return op.resolveResultType(
          envManager.BuildBuiltInType(ast::BuiltInType::Kind::Boolean));
    }
    throw std::runtime_error("operands are not of the same type");
  }

  case BinOp::OpType::Add:
    if (isTypeString(lhs) || isTypeString(rhs)) {
      return op.resolveResultType(
          envManager.BuildBuiltInType(ast::BuiltInType::Kind::String));
    }
    if (lhs->isNumeric() && rhs->isNumeric()) {
      return op.resolveResultType(
          envManager.BuildBuiltInType(ast::BuiltInType::Kind::Int));
    }
    throw std::runtime_error("invalid types for arithmetic operation");

  case BinOp::OpType::And:
  case BinOp::OpType::Or:
    if (lhs->isBoolean() && rhs->isBoolean()) {
      return op.resolveResultType(
          envManager.BuildBuiltInType(ast::BuiltInType::Kind::Boolean));
    }
    throw std::runtime_error("logical operation requires boolean operands");

  default:
    throw std::runtime_error("Invalid binary operation");
  }
}

} // namespace static_check
