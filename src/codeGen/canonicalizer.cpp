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

std::vector<std::shared_ptr<tir::Stmt>>
TIRCanonicalizer::canonicalize(std::shared_ptr<tir::Stmt> stmt) {
  if (auto seq = std::dynamic_pointer_cast<tir::Seq>(stmt)) {
    return canonicalizeSeq(seq);
  } else if (auto exp = std::dynamic_pointer_cast<tir::Exp>(stmt)) {
    return canonicalize(exp->getExpr()->getStatements());
  } else if (auto jump = std::dynamic_pointer_cast<tir::Jump>(stmt)) {
    return canonicalizeJump(jump);
  } else if (auto cJump = std::dynamic_pointer_cast<tir::CJump>(stmt)) {
    return canonicalizeCJump(cJump);
  } else if (auto label = std::dynamic_pointer_cast<tir::Label>(stmt)) {
    return {label};
  } else if (auto move = std::dynamic_pointer_cast<tir::Move>(stmt)) {
    return canonicalizeMove(move);
  } else if (auto ret = std::dynamic_pointer_cast<tir::Return>(ret)) {
    return canonicalizeReturn(ret);
  } else {
    throw std::runtime_error(
        "TIRCanonicalizer::canonicalize: Not a valid stmt");
  }
}

//////////////////// Statement Canonicalizers ////////////////////

std::vector<std::shared_ptr<tir::Stmt>>
TIRCanonicalizer::canonicalizeSeq(std::shared_ptr<tir::Seq> seq) {
  if (!seq) {
    std::cout << "TIRCanonicalizer::canonicalizeSeq: seq is null";
    return nullptr;
  }
  std::vector<std::shared_ptr<tir::Stmt>> combinedStmts;
  for (auto stmt : seq->getStmts()) {
    std::vector<std::shared_ptr<tir::Stmt>> loweredStmts = canonicalize(stmt);
    combinedStmts.insert(combinedStmts.end(), loweredStmts.begin(),
                         loweredStmts.end());
  }
  return combinedStmts;
}

std::vector<std::shared_ptr<tir::Stmt>>
TIRCanonicalizer::canonicalizeJump(std::shared_ptr<tir::Jump> jump) {
  if (!jump) {
    throw std::runtime_error(
        "TIRCanonicalizer::canonicalizeJump: jump is null");
  }
  std::shared_ptr<LoweredExpression> loweredJump =
      canonicalize(jump->getTarget());
  std::vector<std::shared_ptr<tir::Stmt>> loweredStmts =
      loweredJump->getStatements();
  std::shared_ptr<tir::Expr> loweredExpr = loweredJump->getExpression();
  auto newJump = std::make_shared<tir::Jump>(loweredExpr);
  loweredStmts.push_back(newJump);
  return loweredStmts;
}

// TODO: Use CFG and traces to eliminate two-way jumps
std::vector<std::shared_ptr<tir::Stmt>>
TIRCanonicalizer::canonicalizeCJump(std::shared_ptr<tir::CJump> cJump) {
  if (!cJump) {
    throw std::runtime_error(
        "TIRCanonicalizer::canonicalizeCJump: cJump is null");
  }
  std::shared_ptr<LoweredExpression> loweredCJump =
      canonicalize(cJump->getCondition());
  std::vector<std::shared_ptr<tir::Stmt>> loweredStmts =
      canonicalizedCJump->getStatements();
  std::shared_ptr<tir::Expr> loweredExpr = canonicalizedCJump->getExpression();
  auto newCJump = std::make_shared<tir::CJump>(
      loweredExpr, cJump->getTrueLabel(), cJump->getFalseLabel());
  stmts.push_back(newCJump);
  return loweredStmts;
}

std::vector<std::shared_ptr<tir::Stmt>>
TIRCanonicalizer::canonicalizeMove(std::shared_ptr<tir::Move> move) {
  if (!move) {
    throw std::runtime_error(
        "TIRCanonicalizer::canonicalizeMove: move is null");
  }
  std::shared_ptr<tir::Expr> target = move->getTarget();
  std::shared_ptr<tir::Expr> source = move->getSource();
  // Case 1: L[e] = s'; e'
  // Case 2: L[e2] = s2'; e2'
  std::shared_ptr<LoweredExpression> loweredSource = canonicalize(source);
  std::vector<std::shared_ptr<LoweredStatement>> loweredStmts =
      loweredSource->getStatements();
  // Case 1: target is a Temp
  if (auto tempTarget = std::dynamic_pointer_cast<tir::Temp>(target)) {
    // s'; MOVE(TEMP(x), e')
    auto newMove =
        std::make_shared<tir::Move>(tempTarget, loweredSource->getExpression());
    loweredStmts.push_back(newMove);
    return loweredStmts;
  }
  loweredStmts.clear();
  // Case 2: target is a Mem
  auto memTarget = std::dynamic_pointer_cast<tir::Mem>(target);
  // L[e1] = s1'; e1'
  std::shared_ptr<LoweredExpression> loweredAddr =
      canonicalize(memTarget->getAddress());
  // TODO: implement
  std::shared_ptr<Temp> temp = getNewTemp();
  // s1'; MOVE(TEMP(t), e1')
  loweredStmts.insert(loweredStmts.end(), loweredAddr->getStatements().begin(),
                      loweredAddr->getStatements().end());
  auto newMove1 =
      std::make_shared<tir::Move>(temp, loweredAddr->getExpression());
  loweredStmts.push_back(newMove1);
  // s2'; MOVE(MEM(TEMP(t)), e2')
  loweredStmts.insert(loweredStmts.end(),
                      loweredSource->getStatements().begin(),
                      loweredSource->getStatements().end());
  auto newMove2 = std::make_shared<tir::Move>(std::make_shared<tir::Mem>(temp),
                                              loweredSource->getExpression());
  loweredStmts.push_back(newMove2);
  return loweredStmts;
}

std::vector<std::shared_ptr<tir::Stmt>>
TIRCanonicalizer::canonicalizeReturn(std::shared_ptr<tir::Return> ret) {
  if (!ret) {
    throw std::runtime_error("TIRCanonicalizer::canonicalizeRet: ret is null");
  }
  auto loweredRet = canonicalize(ret->getRet());
  std::vector<std::shared_ptr<tir::Stmt>> loweredStmts =
      loweredRet->getStatements();
  std::shared_ptr<tir::Expr> loweredExpr = loweredRet->getExpression();
  auto newReturn = std::make_shared<tir::Return>(loweredExpr);
  loweredStmts.push_back(newReturn);
  return loweredStmts;
}

} // namespace codegen