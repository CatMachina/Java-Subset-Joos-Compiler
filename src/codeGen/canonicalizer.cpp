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

std::shared_ptr<LoweredStatement>
TIRCanonicalizer::canonicalizeSeq(std::shared_ptr<tir::Seq> seq) {
  if (!seq) {
    std::cout << "TIRCanonicalizer::canonicalizeSeq: seq is null";
    return nullptr;
  }
  std::vector<std::shared_ptr<tir::Stmt>> loweredStmts;
  for (auto stmt : seq->getStmts()) {
    std::vector<std::shared_ptr<tir::Stmt>> stmts =
        canonicalize(stmt)->getStatements();
    loweredStmts.insert(loweredStmts.end(), stmts.begin(), stmts.end());
  }
  return std::make_shared<LoweredStatement>(loweredStmts);
}

std::shared_ptr<LoweredStatement>
TIRCanonicalizer::canonicalizeJump(std::shared_ptr<tir::Jump> jump) {
  if (!jump) {
    std::cout << "TIRCanonicalizer::canonicalizeJump: jump is null";
    return nullptr;
  }
  auto canonicalizedExpr = canonicalize(jump->getTarget());
  std::vector<std::shared_ptr<tir::Stmt>> stmts =
      canonicalizedExpr->getStatements();
  std::shared_ptr<tir::Expr> expr = canonicalizedExpr->getExpression();
  auto newJump = std::make_shared<tir::Jump>(expr);
  stmts.push_back(newJump);
  return std::make_shared<LoweredStatement>(stmts);
}

std::shared_ptr<LoweredStatement>
TIRCanonicalizer::canonicalizeCJump(std::shared_ptr<tir::CJump> cJump) {
  if (!cJump) {
    std::cout << "TIRCanonicalizer::canonicalizeCJump: cJump is null";
    return nullptr;
  }
  auto canonicalizedExpr = canonicalize(cJump->getCondition());
  std::vector<std::shared_ptr<tir::Stmt>> stmts =
      canonicalizedExpr->getStatements();
  std::shared_ptr<tir::Expr> expr = canonicalizedExpr->getExpression();
  auto newCJump = std::make_shared<tir::CJump>(expr, cJump->getTrueLabel(),
                                               cJump->getFalseLabel());
  stmts.push_back(newJump);
  return std::make_shared<LoweredStatement>(stmts);
}

std::shared_ptr<LoweredStatement>
TIRCanonicalizer::canonicalizeLabel(std::shared_ptr<tir::Label> label) {
  return std::make_shared<LoweredStatement>({label});
}

std::shared_ptr<LoweredStatement>
TIRCanonicalizer::canonicalizeMove(std::shared_ptr<tir::Move> move) {
  if (!move) {
    std::cout << "TIRCanonicalizer::canonicalizeMove: move is null";
    return nullptr;
  }
  std::shared_ptr<tir::Expr> target = move->getTarget();
  std::shared_ptr<tir::Expr> source = move->getSource();
  auto canonicalizedSource = canonicalize(source);
  std::vector<std::shared_ptr<tir::Stmt>> sourceStmts =
      canonicalizedSource->getStatements();
  std::shared_ptr<tir::Expr> sourceExpr = canonicalizedSource->getExpression();
  if (auto tempTarget = std::dynamic_pointer_cast<tir::Temp>(target)) {
    auto newMove = std::make_shared<tir::Move>(tempTarget, expr);
    sourceStmts.push_back(newMove);
    return std::make_shared<LoweredStatement>(sourceStmts);
  } else {
    auto memTarget = std::dynamic_pointer_cast<tir::Mem>(target);
    auto canonicalizedAddr = canonicalize(memTarget->getAddress());
    std::vector<std::shared_ptr<tir::Stmt>> addrStmts =
        canonicalizedAddr->getStatements();
    std::shared_ptr<tir::Expr> addrExpr = canonicalizedAddr->getExpression();
    // TODO: implement
    auto temp = getNewTemp();
    auto newMove1 = std::make_shared<tir::Move>(temp, addrExpr);
    addrStmts.push_back(newMove1);
    addrStmts.insert(addrStmts.end(), sourceStmts.begin(), sourceStmts.end());
    auto newMove2 =
        std::make_shared<tir::Move>(std::make_shared<tir::Mem>(temp), addrExpr);
    addrStmts.push_back(newMove2);
    return std::make_shared<LoweredStatement>(addrStmt);
  }
}

} // namespace codegen