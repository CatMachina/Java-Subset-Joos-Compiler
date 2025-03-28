#include "codegen/canonicalizer.hpp"

namespace codegen {

void TIRCanonicalizer::canonicalizeCompUnit(
    std::shared_ptr<tir::CompUnit> root) {

  // start statements
  for (auto &stmt : root->start_statements) {
    stmt = std::make_shared<tir::Seq>(canonicalize(stmt)->getStatements());
  }

  // functions
  for (auto &func : root->getFunctionList()) {
    func->getBody() = std::make_shared<tir::Seq>(
        canonicalize(func->getBody())->getStatements());
  }

  // child canonical static fields
  std::vector<std::pair<std::string, std::shared_ptr<Stmt>>>
      child_canonical_static_fields;
  for (auto &fieldPair : root->getFieldList()) {
    auto name = fieldPair.first;
    auto initializer = fieldPair.second;

    auto loweredInitializer = canonicalize(initializer);
    auto initializerStmts = loweredInitializer->getStatements();
    initializerStmts.push_back(std::make_shared<tir::Move>(
        std::make_shared<tir::Temp>(name, nullptr, true),
        loweredInitializer->getExpression()));

    child_canonical_static_fields.emplace_back(
        name, std::make_shared<tir::Seq>(initializerStmts));
  }
  root->canonicalizeStaticFields(child_canonical_static_fields);
}

std::shared_ptr<LoweredExpression>
TIRCanonicalizer::canonicalize(std::shared_ptr<tir::Expr> expression) {
  if (auto binOp = std::dynamic_pointer_cast<tir::BinOp>(expression)) {
    // assume right side effect can affect left
    // naive
    std::shared_ptr<LoweredExpression> loweredLeft =
        canonicalize(binOp->getLeft());
    std::shared_ptr<LoweredExpression> loweredRight =
        canonicalize(binOp->getRight());

    std::string tempName = tir::Temp::generateName();
    auto statements = loweredLeft->getStatements();

    auto newExpression =
        std::make_shared<tir::BinOp>(expression->op,
                                     std::make_shared<tir::Temp>(tempName),
                                     loweredRight->getExpression())

            statements.push_back(std::make_shared<tir::Move>(
                strd::make_shared<tir::Temp>(tempName),
                loweredLeft->getExpression()));
    statements.insert(statements.end(), loweredRight->getStatements().begin(),
                      loweredRight->getStatements().end());

    return std::make_shared<LoweredExpression>(statements, newExpression);

  } else if (auto call = std::dynamic_pointer_cast<tir::Call>(expression)) {
    // Call nodes must be hoisted
    std::vector<std::shared_ptr<tir::Stmt>> statements = {};
    std::vector<std::shared_ptr<tir::Expr>> tempArgs;

    for (auto &arg : call->getArgs()) {
      auto tempArg = std::make_shared<tir::Temp>(
          tir::Temp::generateName("canon_call_arg"));
      tempArgs.push_back(tempArg);

      auto loweredArg = canonicalize(arg);
      statements.insert(statements.end(), loweredArg->getStatements().begin(),
                        loweredArg->getStatements().end());
      statements.push_back(
          std::make_shared<tir::Move>(tempArg, loweredArg->getExpression()));
    }

    if (std::dynamic_pointer_cast<tir::Name>(call->getTarget())) {
      // target is label
      statements.push_back(
          std::make_shared<tir::Call>(call->getTarget(), tempArgs));
    } else {
      // target is other expression
      auto loweredTarget = canonicalize(call->getTarget());
      statements.insert(statements.end(),
                        loweredTarget->getStatements().begin(),
                        loweredTarget->getStatements().end());
      statements.push_back(std::make_shared<tir::Call>(
          loweredTarget->getExpression(), tempArgs));
    }

    return std::make_shared<LoweredExpression>(
        statements,
        std::make_shared<tir::Temp>(codeGenLabels->kAbstractReturn));

  } else if (auto constExpr =
                 std::dynamic_pointer_cast<tir::Const>(expression)) {
    // no side effects
    return std::make_shared<LoweredExpression>(
        std::vector<std::shared_ptr<tir::Stmt>>(), expression);

  } else if (auto eseq = std::dynamic_pointer_cast<tir::ESeq>(expression)) {
    // we can eliminate eseq nodes completely
    auto loweredExpr = canonicalize(eseq->getExpr());
    auto loweredStmt = canonicalize(eseq->getStmt());

    auto statements = loweredStmt->getStatements();
    statements.insert(statements.end(), loweredExpr->getStatements().begin(),
                      loweredExpr->getStatements().end());
    return std::make_shared<LoweredExpression>(statements,
                                               loweredExpr->getExpression());

  } else if (auto mem = std::dynamic_pointer_cast<tir::Mem>(expression)) {
    // hoist the statements out of subexpressions
    auto loweredAddress = canonicalize(mem->getAddress());
    return std::make_shared<LoweredExpression>(
        loweredAddress->getStatements(),
        std::make_shared<tir::Mem>(loweredAddress->getExpression()));

  } else if (auto name = std::dynamic_pointer_cast<tir::Name>(expression)) {
    // no side effects
    return std::make_shared<LoweredExpression>(
        std::vector<std::shared_ptr<tir::Stmt>>(), expression);

  } else if (auto temp = std::dynamic_pointer_cast<tir::Temp>(expression)) {
    // no side effects
    return std::make_shared<LoweredExpression>(
        std::vector<std::shared_ptr<tir::Stmt>>(), expression);
  }

  throw std::runtime_error("Expr type not found, should not happen");
}

std::shared_ptr<LoweredStatement>
TIRCanonicalizer::canonicalize(std::shared_ptr<tir::Stmt> statement) {
  if (auto exp = std::dynamic_pointer_cast<tir::Exp>(statement)) {
    std::shared_ptr<LoweredExpression> loweredExp =
        canonicalize(exp->getExpr());
    return std::make_shared<LoweredStatement>(loweredExp->getStatements());
  }
  // TODO: the rest of Stmt IR
}

} // namespace codegen