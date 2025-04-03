#include "codeGen/codeGenLabels.hpp"
#include "tir/TIR.hpp"

namespace codegen {

/*
class LoweredStatement {
  std::vector<std::shared_ptr<tir::Stmt>> statements;

public:
  LoweredStatement(std::vector<std::shared_ptr<tir::Stmt>> statements)
      : statements{statements} {}

  std::vector<std::shared_ptr<tir::Stmt>> getStatements() { return statements; }

  // template <typename... StatementIRs> LoweredStatement(StatementIRs
  // &&...args) {
  //   (statements.emplace_back(args), ...);
  // }
};
*/

class LoweredExpression {
  std::vector<std::shared_ptr<tir::Stmt>> statements;
  std::shared_ptr<tir::Expr> expression;

public:
  LoweredExpression(std::vector<std::shared_ptr<tir::Stmt>> statements,
                    std::shared_ptr<tir::Expr> expression)
      : statements{statements}, expression{expression} {}

  std::vector<std::shared_ptr<tir::Stmt>> getStatements() { return statements; }
  std::shared_ptr<tir::Expr> getExpression() { return expression; }
};

class TIRCanonicalizer {
  // Angel: Looks like LoweredStatement is a wrapper for a vector?
  // I will just use vectors for now, but we can add it back if needed
  std::vector<std::shared_ptr<tir::Stmt>>
  canonicalize(std::shared_ptr<tir::Stmt> statement);

  std::shared_ptr<LoweredExpression>
  canonicalize(std::shared_ptr<tir::Expr> expression);

  std::shared_ptr<CodeGenLabels> codeGenLabels;

public:
  TIRCanonicalizer(std::shared_ptr<CodeGenLabels> codeGenLabels) {
    this->codeGenLabels = codeGenLabels;
  }

  void canonicalizeCompUnit(std::shared_ptr<tir::CompUnit> &root);

  // Statement Canonicalizers
  std::vector<std::shared_ptr<tir::Stmt>>
  canonicalizeSeq(std::shared_ptr<tir::Seq> seq);

  std::vector<std::shared_ptr<tir::Stmt>>
  canonicalizeJump(std::shared_ptr<tir::Jump> jump);

  std::vector<std::shared_ptr<tir::Stmt>>
  canonicalizeCJump(std::shared_ptr<tir::CJump> cJump);

  std::vector<std::shared_ptr<tir::Stmt>>
  canonicalizeMove(std::shared_ptr<tir::Move> move);

  std::vector<std::shared_ptr<tir::Stmt>>
  canonicalizeReturn(std::shared_ptr<tir::Return> ret);
};

} // namespace codegen
