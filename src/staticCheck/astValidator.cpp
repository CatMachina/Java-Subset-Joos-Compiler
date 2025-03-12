#include "staticCheck/astValidator.hpp"

namespace static_check {

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
