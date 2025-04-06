#include "codeGen/canonicalizer.hpp"

namespace codegen {

void TIRCanonicalizer::canonicalizeCompUnit(
    std::shared_ptr<tir::CompUnit> &root) {

  // start statements
  for (auto &stmt : root->getStartStmts()) {
    stmt = std::make_shared<tir::Seq>(canonicalize(stmt));
  }

  // functions
  for (auto &func : root->getFunctionList()) {
    // func->print(std::cout);
    func->getBody() = std::make_shared<tir::Seq>(canonicalize(func->getBody()));
  }

  // child canonical static fields
  std::vector<std::pair<std::string, std::shared_ptr<tir::Stmt>>>
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

    auto newExpression = std::make_shared<tir::BinOp>(
        binOp->op, std::make_shared<tir::Temp>(tempName),
        loweredRight->getExpression());

    statements.push_back(std::make_shared<tir::Move>(
        std::make_shared<tir::Temp>(tempName), loweredLeft->getExpression()));

    auto loweredRightStmts = loweredRight->getStatements();
    statements.insert(statements.end(), loweredRightStmts.begin(),
                      loweredRightStmts.end());

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
      auto loweredArgStmts = loweredArg->getStatements();
      statements.insert(statements.end(), loweredArgStmts.begin(),
                        loweredArgStmts.end());
      statements.push_back(
          std::make_shared<tir::Move>(tempArg, loweredArg->getExpression()));
    }

    if (std::dynamic_pointer_cast<tir::Name>(call->getTarget())) {
      // target is label
      statements.push_back(std::make_shared<tir::CallStmt>(
          std::make_shared<tir::Call>(call->getTarget(), tempArgs)));
    } else {
      // target is other expression
      auto loweredTarget = canonicalize(call->getTarget());
      auto loweredTargetStmts = loweredTarget->getStatements();
      statements.insert(statements.end(), loweredTargetStmts.begin(),
                        loweredTargetStmts.end());
      statements.push_back(
          std::make_shared<tir::CallStmt>(std::make_shared<tir::Call>(
              loweredTarget->getExpression(), tempArgs)));
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
    auto loweredStmts = canonicalize(eseq->getStmt());

    auto loweredExprStmts = loweredExpr->getStatements();
    loweredStmts.insert(loweredStmts.end(), loweredExprStmts.begin(),
                        loweredExprStmts.end());
    return std::make_shared<LoweredExpression>(loweredStmts,
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
    return canonicalize(exp->getExpr())->getStatements();
  } else if (auto jump = std::dynamic_pointer_cast<tir::Jump>(stmt)) {
    return canonicalizeJump(jump);
  } else if (auto cjump = std::dynamic_pointer_cast<tir::CJump>(stmt)) {
    return canonicalizeCJump(cjump);
  } else if (auto label = std::dynamic_pointer_cast<tir::Label>(stmt)) {
    return {label};
  } else if (auto move = std::dynamic_pointer_cast<tir::Move>(stmt)) {
    return canonicalizeMove(move);
  } else if (auto ret = std::dynamic_pointer_cast<tir::Return>(stmt)) {
    return canonicalizeReturn(ret);
  } else if (auto comment = std::dynamic_pointer_cast<tir::Comment>(stmt)) {
    return {comment};
  } else {
    std::cout << "not a valid stmt" << std::endl;
    stmt->print(std::cout);
    throw std::runtime_error(
        "TIRCanonicalizer::canonicalize: Not a valid stmt");
  }
}

//////////////////// Statement Canonicalizers ////////////////////

std::vector<std::shared_ptr<tir::Stmt>>
TIRCanonicalizer::canonicalizeSeq(std::shared_ptr<tir::Seq> seq) {
  if (!seq) {
    throw std::runtime_error("TIRCanonicalizer::canonicalizeSeq: seq is null");
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
  std::shared_ptr<LoweredExpression> loweredTarget =
      canonicalize(jump->getTarget());
  std::vector<std::shared_ptr<tir::Stmt>> loweredStmts =
      loweredTarget->getStatements();
  auto newJump = std::make_shared<tir::Jump>(loweredTarget->getExpression());
  loweredStmts.push_back(newJump);
  return loweredStmts;
}

std::vector<std::shared_ptr<tir::Stmt>>
TIRCanonicalizer::canonicalizeCJump(std::shared_ptr<tir::CJump> cjump) {
  if (!cjump) {
    throw std::runtime_error(
        "TIRCanonicalizer::canonicalizeCJump: cjump is null");
  }
  std::shared_ptr<LoweredExpression> loweredCondition =
      canonicalize(cjump->getCondition());
  std::vector<std::shared_ptr<tir::Stmt>> loweredStmts =
      loweredCondition->getStatements();
  std::string newLabelName = cjump->getFalseLabel() + "_cpy";
  // CJUMP(e, l1, l2')
  auto newCJump = std::make_shared<tir::CJump>(
      loweredCondition->getExpression(), cjump->getTrueLabel(), newLabelName);
  loweredStmts.push_back(newCJump);
  // LABEL(l2')
  auto label = std::make_shared<tir::Label>(newLabelName);
  loweredStmts.push_back(label);
  // JUMP(NAME(l2))
  auto jump = std::make_shared<tir::Jump>(
      std::make_shared<tir::Name>(cjump->getFalseLabel()));
  loweredStmts.push_back(jump);
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
  std::vector<std::shared_ptr<tir::Stmt>> loweredStmts =
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
  // s1'; MOVE(TEMP(t), e1')
  auto lowerAddrStmts = loweredAddr->getStatements();
  loweredStmts.insert(loweredStmts.end(), lowerAddrStmts.begin(),
                      lowerAddrStmts.end());
  std::string tempName = tir::Temp::generateName("move_target");
  auto temp = std::make_shared<tir::Temp>(tempName);
  auto newMove1 =
      std::make_shared<tir::Move>(temp, loweredAddr->getExpression());
  loweredStmts.push_back(newMove1);
  // s2'; MOVE(MEM(TEMP(t)), e2')
  auto loweredSourceStmts = loweredSource->getStatements();
  loweredStmts.insert(loweredStmts.end(), loweredSourceStmts.begin(),
                      loweredSourceStmts.end());
  auto newMove2 = std::make_shared<tir::Move>(std::make_shared<tir::Mem>(temp),
                                              loweredSource->getExpression());
  loweredStmts.push_back(newMove2);
  return loweredStmts;
}

std::vector<std::shared_ptr<tir::Stmt>>
TIRCanonicalizer::canonicalizeReturn(std::shared_ptr<tir::Return> ret) {
  if (!ret) {
    throw std::runtime_error(
        "TIRCanonicalizer::canonicalizeReturn: ret is null");
  }

  if (!(ret->getRet())) {
    std::vector<std::shared_ptr<tir::Stmt>> loweredStmts;
    auto newReturn = std::make_shared<tir::Return>(nullptr);
    loweredStmts.push_back(newReturn);
    return loweredStmts;
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