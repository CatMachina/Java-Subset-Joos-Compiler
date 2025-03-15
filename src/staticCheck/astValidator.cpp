#include "staticCheck/astValidator.hpp"

namespace static_check {

bool ASTValidator::isAccessible(
    std::shared_ptr<parsetree::ast::Modifiers> mod,
    std::shared_ptr<parsetree::ast::CodeBody> parent) {
  // 6.6.1
  if (mod->isPublic())
    return true;
  // 6.6.2
  auto targetClass =
      std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(parent);
  if (auto curClass = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
          currentProgram->getBody())) {
    if (typeResolver->isSuperClass(targetClass, curClass))
      return true;
  }
  // same package
  if (auto otherProgram =
          std::dynamic_pointer_cast<parsetree::ast::ProgramDecl>(
              targetClass->getParent())) {
    if (currentProgram->getPackageName() == otherProgram->getPackageName())
      return true;
  }
  return false;
}

bool ASTValidator::areParameterTypesApplicable(
    std::shared_ptr<parsetree::ast::MethodDecl> decl,
    const std::vector<std::shared_ptr<parsetree::ast::Type>> &argTypes) const {
  bool valid = true;
  for (size_t i = 0; i < argTypes.size(); i++) {
    auto ty1 = argTypes[i];
    auto ty2 = decl->getParams()[i]->getType();
    // std::cout << "ty1: ";
    // ty1->print(std::cout);
    // std::cout << ", ty2: ";
    // ty2->print(std::cout);
    // std::cout << std::endl;
    valid &= typeResolver->isAssignableTo(ty1, ty2);
  }
  return valid;
}

void ASTValidator::validateProgram(
    std::shared_ptr<parsetree::ast::ProgramDecl> program) {
  currentProgram = program;
  auto bodyDecl = program->getBody()->asDecl();
  if (!bodyDecl)
    return;
  for (auto child : bodyDecl->getChildren()) {
    if (auto method =
            std::dynamic_pointer_cast<parsetree::ast::MethodDecl>(child)) {
      validateMethod(method);
    } else if (auto field =
                   std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(
                       child)) {
      validateVarDecl(field);
    }
  }
}

void ASTValidator::validateMethod(
    std::shared_ptr<parsetree::ast::MethodDecl> method) {
  if (method->isConstructor()) {
    if (!(method->getMethodBody()) || method->getMethodBody()->isEmpty()) {
      auto parent = method->getParent();
      auto parentClass =
          std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(parent);
      if (parentClass) {
        int validSuperClass = 0;
        for (auto superRef : parentClass->getSuperClasses()) {
          if (superRef)
            validSuperClass++;
        }
        bool needCheck = false;
        std::vector<std::shared_ptr<parsetree::ast::MethodDecl>> validSupers;
        for (auto superRef : parentClass->getSuperClasses()) {
          if (!superRef)
            continue;
          if (!(superRef->isResolved())) {
            throw std::runtime_error("Super class not resolved");
          }
          auto superDecl = superRef->getResolvedDecl()->getAstNode();
          if (superDecl->getName() == "Object")
            continue;

          needCheck = true;
          auto superClass =
              std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(superDecl);
          if (!superClass)
            throw std::runtime_error("Super class is not a class");
          std::vector<std::shared_ptr<parsetree::ast::Type>> argTypes;
          for (auto param : method->getParams()) {
            if (!param->getType()) {
              throw std::runtime_error("Parameter type is null");
            }
            argTypes.push_back(param->getType());
          }

          for (auto superConstructor : superClass->getConstructors()) {
            if (!isAccessible(superConstructor->getModifiers(),
                              superConstructor->getParent())) {
              continue;
            }

            // FIXME: currently does not check if is subset!
            if (superConstructor->getParams().size() < argTypes.size()) {
              validSupers.push_back(superConstructor);
              continue;
            }
            if (superConstructor->getParams().size() != argTypes.size()) {
              continue;
            }
            if (!areParameterTypesApplicable(superConstructor, argTypes)) {
              continue;
            }
            // std::cout << "for " << method->getName()
            //           << " found valid super constructor for super"
            //           << superClass->getName() << std::endl;
            validSupers.push_back(superConstructor);
          }
        }
        if (needCheck && validSupers.size() == 0) {
          throw std::runtime_error("No valid super constructor for " +
                                   method->getName());
        }
      }
    }
  }
  if (auto body = method->getMethodBody()) {
    currentMethod = method;
    validateStmt(body);
    currentMethod = nullptr;
  }
}

void ASTValidator::validateStmt(std::shared_ptr<parsetree::ast::Stmt> stmt) {
  if (auto ret = std::dynamic_pointer_cast<parsetree::ast::ReturnStmt>(stmt)) {
    validateReturnStmt(ret);
  } else if (auto decl =
                 std::dynamic_pointer_cast<parsetree::ast::DeclStmt>(stmt)) {
    validateVarDecl(decl->getDecl());
  } else if (auto ifStmt =
                 std::dynamic_pointer_cast<parsetree::ast::IfStmt>(stmt)) {
    if (!(ifStmt->getCondition()))
      throw std::runtime_error("if condition is null");

    auto condType = getTypeFromExpr(ifStmt->getCondition());
    if (!condType || !condType->isBoolean()) {
      throw std::runtime_error(
          "if condition expression must be yield a boolean");
    }
  } else if (auto forStmt =
                 std::dynamic_pointer_cast<parsetree::ast::ForStmt>(stmt)) {
    if (forStmt->getCondition()) {
      auto condType = getTypeFromExpr(forStmt->getCondition());
      if (!condType || !condType->isBoolean()) {
        throw std::runtime_error(
            "for condition expression must be yield a boolean");
      }
    }
  } else if (auto whileStmt =
                 std::dynamic_pointer_cast<parsetree::ast::WhileStmt>(stmt)) {
    assert(whileStmt->getCondition());
    auto condType = getTypeFromExpr(whileStmt->getCondition());
    if (!condType || !condType->isBoolean()) {
      throw std::runtime_error(
          "while condition expression must be yield a boolean");
    }
  }
  for (auto child : stmt->getChildren()) {
    if (auto stmt = std::dynamic_pointer_cast<parsetree::ast::Stmt>(child)) {
      validateStmt(stmt);
    }
  }
}

void ASTValidator::validateReturnStmt(
    std::shared_ptr<parsetree::ast::ReturnStmt> stmt) {
  if (!currentMethod)
    throw std::runtime_error("currentMethod is null");
  auto methodReturnType = currentMethod->getReturnType();
  if (stmt->getExprs().size() != 1 || !(stmt->getExprs()[0])) {
    if (methodReturnType) {
      throw std::runtime_error(
          "there must be one and only one return expression");
    }
    // void
    return;
  }

  std::vector<std::shared_ptr<parsetree::ast::Expr>> exprs = stmt->getExprs();
  std::shared_ptr<parsetree::ast::Type> returnType = getTypeFromExpr(exprs[0]);
  if (!returnType) {
    throw std::runtime_error(
        "return expression is null but return type is not void");
  }

  if (!methodReturnType) {
    if (returnType) {
      throw std::runtime_error(
          "return type is void but return expression is not");
    }
    return;
  }

  if (!(typeResolver->isAssignableTo(methodReturnType, returnType))) {
    throw std::runtime_error(
        "return type must be assignable to method return type");
  }
}

void ASTValidator::validateVarDecl(
    std::shared_ptr<parsetree::ast::VarDecl> varDecl) {
  if (!(varDecl->hasInit()))
    return;
  auto declType = varDecl->getType();
  auto exprType = getTypeFromExpr(varDecl->getInitializer());
  if (!exprType) {
    throw std::runtime_error("initializer type cannot be void");
  }
  if (!typeResolver->isAssignableTo(declType, exprType)) {
    throw std::runtime_error(
        "initializer type must be assignable to declared type");
  }
}

std::shared_ptr<parsetree::ast::Type>
ASTValidator::getTypeFromExpr(std::shared_ptr<parsetree::ast::Expr> expr) {
  if (!expr)
    throw std::runtime_error("trying to get type from null expr");
  auto lastExpr = expr->getLastExprNode();
  if (!lastExpr)
    throw std::runtime_error("trying to get type from null last expr");
  if (auto op = std::dynamic_pointer_cast<parsetree::ast::ExprOp>(lastExpr)) {
    if (!op->getResultType())
      throw std::runtime_error("Expr op result type cannot be null");
    return op->getResultType();
  } else if (auto value = std::dynamic_pointer_cast<parsetree::ast::ExprValue>(
                 lastExpr)) {
    if (!value->getType())
      throw std::runtime_error("Expr value type cannot be null");
    return value->getType();
  }
  throw std::runtime_error(
      "Expr node list contain node other than op or value after type resolve!");
}

} // namespace static_check
