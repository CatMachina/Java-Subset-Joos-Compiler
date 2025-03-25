#include "TIRBuilder.hpp"

namespace tir {

//////////////////// Helpers ////////////////////

std::string TIRBuilder::getNextLabelName() {
  return "label_" + std::to_string(labelCounter++);
}

std::shared_ptr<Label> TIRBuilder::newLabel() {
  return std::make_shared<Label>(getNextLabelName());
}

//////////////////// Statement Builders ////////////////////

std::shared_ptr<Stmt>
TIRBuilder::buildStmt(std::shared_ptr<parsetree::ast::Stmt> node) {
  if (!node) {
    throw std::runtime_error("TIRBuilder::buildStmt: node is null");
  }
  std::shared_ptr<Node> irNode;
  if (auto block = std::dynamic_pointer_cast<parsetree::ast::Block>(node)) {
    irNode = buildBlock(block);
  } else if (auto ifStmt =
                 std::dynamic_pointer_cast<parsetree::ast::IfStmt>(node)) {
    irNode = buildIfStmt(ifStmt);
  } else if (auto whileStmt =
                 std::dynamic_pointer_cast<parsetree::ast::WhileStmt>(node)) {
    irNode = buildWhileStmt(whileStmt);
  } else if (auto forStmt =
                 std::dynamic_pointer_cast<parsetree::ast::ForStmt>(node)) {
    irNode = buildForStmt(forStmt);
  } else if (auto returnStmt =
                 std::dynamic_pointer_cast<parsetree::ast::ReturnStmt>(node)) {
    irNode = buildReturnStmt(returnStmt);
  } else if (auto expressionStmt =
                 std::dynamic_pointer_cast<parsetree::ast::ExpressionStmt>(
                     node)) {
    irNode = buildExpressionStmt(expressionStmt);
  } else if (auto declStmt =
                 std::dynamic_pointer_cast<parsetree::ast::DeclStmt>(node)) {
    irNode = buildDeclStmt(declStmt)
  } else {
    throw std::runtime_error("TIRBuilder::buildStmt: not an AST Stmt");
  }
  return irNode;
}

std::shared_ptr<Stmt>
TIRBuilder::buildBlock(std::shared_ptr<parsetree::ast::Block> node) {
  if (!node) {
    throw std::runtime_error("TIRBuilder::buildBlock: node is null");
  }
  std::vector<std::shared_ptr<Stmt>> irStmts;
  for (auto astStmt : node->getStmts()) {
    irStmts.push_back(buildStmt(astStmt));
  }
  return std::make_shared<Seq>(irStmts);
}

std::shared_ptr<Stmt>
TIRBuilder::buildIfStmt(std::shared_ptr<parsetree::ast::IfStmt> node) {
  if (!node) {
    throw std::runtime_error("TIRBuilder::buildIfStmt: node is null");
  }
  auto lt = newLabel();
  auto lf = newLabel();
  // TODO: buildExpr
  auto condition = buildExpr(node->getCondition());
  auto stmt = buildStmt(node->getIfBody());
  auto cJump = std::make_shared<CJump>(condition, lt, lf);
  return std::make_shared<Seq>({cJump, lt, stmt, lf});
}

std::shared_ptr<Stmt>
TIRBuilder::buildWhileStmt(std::shared_ptr<parsetree::ast::WhileStmt> node) {
  if (!node) {
    throw std::runtime_error("TIRBuilder::buildWhileStmt: node is null");
  }
  auto lh = newLabel();
  auto lt = newLabel();
  auto lf = newLabel();
  // TODO: buildExpr
  auto condition = buildExpr(node->getCondition());
  auto stmt = buildStmt(node->getWhileBody());
  auto cJump = std::make_shared<CJump>(condition, lt, lf);
  auto name = std::make_shared<Name>(lh->getName());
  auto jump = std::make_shared<Jump>(name);
  return std::make_shared<Seq>({lh, cJump, lt, stmt, jump, lf});
}

std::shared_ptr<Stmt>
TIRBuilder::buildForStmt(std::shared_ptr<parsetree::ast::ForStmt> node) {
  if (!node) {
    throw std::runtime_error("TIRBuilder::buildForStmt: node is null");
  }
  auto lh = newLabel();
  auto lt = newLabel();
  auto lf = newLabel();
  auto init = buildStmt(node->getForInit());
  // TODO: buildExpr
  auto condition = buildExpr(node->getCondition());
  auto update = buildStmt(node->getForUpdate());
  auto body = buildStmt(node->getForBody());
  auto cJump = std::make_shared<CJump>(condition, lt, lf);
  auto name = std::make_shared<Name>(lh->getName());
  auto jump = std::make_shared<Jump>(name);
  return std::make_shared<Seq>({init, lh, cJump, lt, body, update, jump, lf});
}

std::shared_ptr<Stmt>
TIRBuilder::buildReturnStmt(std::shared_ptr<parsetree::ast::ReturnStmt> node) {
  if (!node) {
    throw std::runtime_error("TIRBuilder::buildReturnStmt: node is null");
  }
  // TODO: buildExpr
  auto returnExpr = buildExpr(node->getReturnExpr());
  return std::make_shared<Return>(returnExpr);
}

std::shared_ptr<Stmt> TIRBuilder::buildExpressionStmt(
    std::shared_ptr<parsetree::ast::ExpressionStmt> node) {
  if (!node) {
    throw std::runtime_error("TIRBuilder::buildExpressionStmt: node is null");
  }
  // TODO: buildExpr
  return std::make_shared<Exp>(buildExpr(node->getStatementExpr()));
}

std::shared_ptr<Stmt>
TIRBuilder::buildDeclStmt(std::shared_ptr<parsetree::ast::DeclStmt> node) {
  if (!node) {
    throw std::runtime_error("TIRBuilder::buildDeclStmt: node is null");
  }
  return buildVarDecl(node->getDecl());
}

//////////////////// Declaration Builders ////////////////////
std::shared_ptr<Node>
TIRBuilder::buildDecl(std::shared_ptr<parsetree::ast::Decl> node) {
  if (!node) {
    throw std::runtime_error("TIRBuilder::buildDecl: node is null");
  }

  if (auto methodDecl =
          std::dynamic_pointer_cast<parsetree::ast::MethodDecl>(node)) {
    return buildMethodDecl(methodDecl);
  } else if (auto fieldDecl =
                 std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(node)) {
    return buildFieldDecl(fieldDecl);
  } else if (auto varDecl =
                 std::dynamic_pointer_cast<parsetree::ast::VarDecl>(node)) {
    return buildVarDecl(varDecl);
  } else {
    throw std::runtime_error("TIRBuilder::buildDecl: unsupported Decl type");
  }
}

std::shared_ptr<tir::CompUnit>
TIRBuilder::buildProgram(std::shared_ptr<parsetree::ast::ProgramDecl> node) {
  if (!node) {
    throw std::runtime_error("TIRBuilder::buildProgram: node is null");
  }

  auto body = node->getBody();
  if (!body) {
    throw std::runtime_error("TIRBuilder::buildProgram: body is null");
  }

  auto decl = std::dynamic_pointer_cast<parsetree::ast::Decl>(body);
  if (!decl) {
    throw std::runtime_error("TIRBuilder::buildProgram: body is not a Decl");
  }

  std::string className = decl->getName();
  std::vector<std::shared_ptr<tir::Node>> nodes;

  if (auto classDecl =
          std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(decl)) {
    auto classNodes = buildClassDecl(classDecl);
    nodes.insert(nodes.end(), classNodes.begin(), classNodes.end());
  } else {
    throw std::runtime_error(
        "TIRBuilder::buildProgram: only ClassDecl supported at top level");
  }

  return std::make_shared<tir::CompUnit>(className, nodes);
}

std::shared_ptr<FuncDecl>
TIRBuilder::buildMethodDecl(std::shared_ptr<parsetree::ast::MethodDecl> node) {
  if (!node) {
    throw std::runtime_error("TIRBuilder::buildMethodDecl: node is null");
  }

  std::vector<std::shared_ptr<Temp>> params;
  for (const auto &param : node->params) {
    auto name = param->getName();
    params.push_back(std::make_shared<Temp>(name));
  }

  auto body = buildStmt(node->methodBody);

  return std::make_shared<FuncDecl>(node->getName(), params, body);
}

std::shared_ptr<Stmt>
TIRBuilder::buildVarDecl(std::shared_ptr<parsetree::ast::VarDecl> node) {
  if (!node) {
    throw std::runtime_error("TIRBuilder::buildVarDecl: node is null");
  }

  auto temp = std::make_shared<Temp>(node->getName());
  if (node->hasInit()) {
    auto expr = buildExpr(node->getInitializer());
    return std::make_shared<Move>(temp, expr);
  } else {
    return std::make_shared<Move>(temp, std::make_shared<Const>(0));
  }
}

// NOTE: Treats field variables like local variables
std::shared_ptr<Stmt>
TIRBuilder::buildFieldDecl(std::shared_ptr<parsetree::ast::FieldDecl> node) {
  if (!node) {
    throw std::runtime_error("TIRBuilder::buildFieldDecl: node is null");
  }

  return buildVarDecl(node);
}

std::vector<std::shared_ptr<Node>>
TIRBuilder::buildClassDecl(std::shared_ptr<parsetree::ast::ClassDecl> node) {
  if (!node) {
    throw std::runtime_error("TIRBuilder::buildClassDecl: node is null");
  }

  std::vector<std::shared_ptr<Node>> results;
  for (const auto &decl : node->getClassMembers()) {
    results.push_back(buildDecl(decl));
  }

  return results;
}

} // namespace tir
