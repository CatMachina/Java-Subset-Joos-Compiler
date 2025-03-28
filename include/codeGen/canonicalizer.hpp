#include "codeGen/codeGenLabels.hpp"
#include "tir/TIR.hpp"

namespace codegen {

class LoweredStatement {
  std::vector<tir::Stmt> statements;

public:
  LoweredStatement(std::vector<tir::Stmt> statements)
      : statements{statements} {}

  std::vector<tir::Stmt> getStatements() { return statements; }

  // template <typename... StatementIRs> LoweredStatement(StatementIRs
  // &&...args) {
  //   (statements.emplace_back(args), ...);
  // }
};

class LoweredExpression {
  std::vector<tir::Stmt> statements;
  std::shared_ptr<tir::Expr> expression;

public:
  LoweredExpression(std::vector<tir::Stmt> statements,
                    std::shared_ptr<tir::Expr> expression)
      : statements{statements}, expression{expression} {}

  std::vector<tir::Stmt> getStatements() { return statements; }
  std::shared_ptr<tir::Expr> getExpression() { return expression; }

  // template <typename... StatementIRs>
  // LoweredExpression(tir::Expr expression, StatementIRs &&...args) {
  //   this->expression = std::make_shared<tir::Expr>(expression);
  //   (statements.emplace_back(args), ...);
  // }
};

class TIRCanonicalizer {
  std::shared_ptr<LoweredStatement>
  canonicalize(std::shared_ptr<tir::Stmt> statement);
  std::shared_ptr<LoweredExpression>
  canonicalize(std::shared_ptr<tir::Expr> expression);

  std::shared_ptr<CodeGenLabels> codeGenLabels;

public:
  TIRCanonicalizer(std::shared_ptr<CodeGenLabels> codeGenLabels) {
    this->codeGenLabels = codeGenLabels;
  }

  void canonicalizeCompUnit(std::shared_ptr<tir::CompUnit> root);
};

} // namespace codegen
