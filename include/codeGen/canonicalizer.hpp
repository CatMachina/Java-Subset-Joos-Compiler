

// draft

class LoweredStatement {
  std::vector<tir::Stmt> statements;

public:
  template <typename... StatementIRs> LoweredStatement(StatementIRs &&...args) {
    (statements.emplace_back(args), ...);
  }
};

class LoweredExpression {
  std::vector<tir::Stmt> statements;
  std::shared_ptr<tir::Expr> expression;

public:
  template <typename... StatementIRs>
  LoweredExpression(tir::Expr expression, StatementIRs &&...args) {
    this->expression = std::make_shared<tir::Expr>(expression);
    (statements.emplace_back(args), ...);
  }
};
