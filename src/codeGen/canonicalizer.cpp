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
TIRCanonicalizer::canonicalize(std::shared_ptr<tir::Expr> expression) {}

std::shared_ptr<LoweredStatement>
TIRCanonicalizer::canonicalize(std::shared_ptr<tir::Stmt> statement) {}

} // namespace codegen