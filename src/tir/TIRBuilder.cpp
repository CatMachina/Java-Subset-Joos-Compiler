#include "tir/TIRBuilder.hpp"

namespace tir {

//////////////////// Helpers ////////////////////

std::string TIRBuilder::getNextLabelName() {
  return "label_" + std::to_string(labelCounter++);
}

std::shared_ptr<Label> TIRBuilder::getNewLabel() {
  return std::make_shared<Label>(getNextLabelName());
}

//////////////////// Expression Builder ////////////////////

std::shared_ptr<Expr>
TIRBuilder::buildExpr(std::shared_ptr<parsetree::ast::Expr> expr) {
  return exprConverter.evaluateList(expr->getExprNodes());
}

//////////////////// Statement Builders ////////////////////

std::shared_ptr<Stmt>
TIRBuilder::buildStmt(std::shared_ptr<parsetree::ast::Stmt> node) {
  if (!node) {
    std::cout << "TIRBuilder::buildStmt: node is null" << std::endl;
    return nullptr;
  }
  std::shared_ptr<Stmt> irStmt;
  if (auto block = std::dynamic_pointer_cast<parsetree::ast::Block>(node)) {
    irStmt = buildBlock(block);
  } else if (auto ifStmt =
                 std::dynamic_pointer_cast<parsetree::ast::IfStmt>(node)) {
    irStmt = buildIfStmt(ifStmt);
  } else if (auto whileStmt =
                 std::dynamic_pointer_cast<parsetree::ast::WhileStmt>(node)) {
    irStmt = buildWhileStmt(whileStmt);
  } else if (auto forStmt =
                 std::dynamic_pointer_cast<parsetree::ast::ForStmt>(node)) {
    irStmt = buildForStmt(forStmt);
  } else if (auto returnStmt =
                 std::dynamic_pointer_cast<parsetree::ast::ReturnStmt>(node)) {
    irStmt = buildReturnStmt(returnStmt);
  } else if (auto expressionStmt =
                 std::dynamic_pointer_cast<parsetree::ast::ExpressionStmt>(
                     node)) {
    irStmt = buildExpressionStmt(expressionStmt);
  } else if (auto declStmt =
                 std::dynamic_pointer_cast<parsetree::ast::DeclStmt>(node)) {
    irStmt = buildDeclStmt(declStmt);
  } else if (auto nullStmt =
                 std::dynamic_pointer_cast<parsetree::ast::NullStmt>(node)) {
    irStmt = nullptr;
  } else {
    throw std::runtime_error("TIRBuilder::buildStmt: not an AST Stmt");
  }
  return irStmt;
}

std::shared_ptr<Stmt>
TIRBuilder::buildBlock(std::shared_ptr<parsetree::ast::Block> node) {
  if (!node) {
    std::cout << "TIRBuilder::buildBlock: node is null" << std::endl;
    return nullptr;
  }
  std::vector<std::shared_ptr<Stmt>> irStmts;
  for (auto astStmt : node->getStatements()) {
    irStmts.push_back(buildStmt(astStmt));
  }
  return std::make_shared<Seq>(irStmts);
}

std::shared_ptr<Stmt>
TIRBuilder::buildIfStmt(std::shared_ptr<parsetree::ast::IfStmt> node) {
  if (!node) {
    std::cout << "TIRBuilder::buildIfStmt: node is null" << std::endl;
    return nullptr;
  }
  auto ltName = getNextLabelName();
  auto lfName = getNextLabelName();
  auto lt = std::make_shared<Label>(ltName);
  auto lf = std::make_shared<Label>(lfName);

  auto condition = buildExpr(node->getCondition());
  auto stmt = buildStmt(node->getIfBody());
  auto cJump = std::make_shared<CJump>(condition, ltName, lfName);

  std::vector<std::shared_ptr<Stmt>> stmts = {cJump, lt, stmt, lf};
  return std::make_shared<Seq>(stmts);
}

std::shared_ptr<Stmt>
TIRBuilder::buildWhileStmt(std::shared_ptr<parsetree::ast::WhileStmt> node) {
  if (!node) {
    std::cout << "TIRBuilder::buildWhileStmt: node is null" << std::endl;
    return nullptr;
  }
  auto lh = getNewLabel();
  auto ltName = getNextLabelName();
  auto lfName = getNextLabelName();
  auto lt = std::make_shared<Label>(ltName);
  auto lf = std::make_shared<Label>(lfName);

  auto condition = buildExpr(node->getCondition());
  auto stmt = buildStmt(node->getWhileBody());
  auto cJump = std::make_shared<CJump>(condition, ltName, lfName);
  auto name = std::make_shared<Name>(lh->getName());
  auto jump = std::make_shared<Jump>(name);

  std::vector<std::shared_ptr<Stmt>> stmts = {lh, cJump, lt, stmt, jump, lf};
  return std::make_shared<Seq>(stmts);
}

std::shared_ptr<Stmt>
TIRBuilder::buildForStmt(std::shared_ptr<parsetree::ast::ForStmt> node) {
  if (!node) {
    std::cout << "TIRBuilder::buildForStmt: node is null" << std::endl;
    return nullptr;
  }
  auto lh = getNewLabel();
  auto ltName = getNextLabelName();
  auto lfName = getNextLabelName();
  auto lt = std::make_shared<Label>(ltName);
  auto lf = std::make_shared<Label>(lfName);
  auto init = buildStmt(node->getForInit());

  auto condition = buildExpr(node->getCondition());
  auto update = buildStmt(node->getForUpdate());
  auto body = buildStmt(node->getForBody());
  auto cJump = std::make_shared<CJump>(condition, ltName, lfName);
  auto name = std::make_shared<Name>(lh->getName());
  auto jump = std::make_shared<Jump>(name);
  std::vector<std::shared_ptr<Stmt>> stmts = {init, lh,     cJump, lt,
                                              body, update, jump,  lf};
  return std::make_shared<Seq>(stmts);
}

std::shared_ptr<Stmt>
TIRBuilder::buildReturnStmt(std::shared_ptr<parsetree::ast::ReturnStmt> node) {
  if (!node) {
    std::cout << "TIRBuilder::buildReturnStmt: node is null" << std::endl;
    return nullptr;
  }
  auto returnExpr = buildExpr(node->getReturnExpr());
  return std::make_shared<Return>(returnExpr);
}

std::shared_ptr<Stmt> TIRBuilder::buildExpressionStmt(
    std::shared_ptr<parsetree::ast::ExpressionStmt> node) {
  if (!node) {
    std::cout << "TIRBuilder::buildExpressionStmt: node is null" << std::endl;
    return nullptr;
  }
  return std::make_shared<Exp>(buildExpr(node->getStatementExpr()));
}

std::shared_ptr<Stmt>
TIRBuilder::buildDeclStmt(std::shared_ptr<parsetree::ast::DeclStmt> node) {
  if (!node) {
    std::cout << "TIRBuilder::buildDeclStmt: node is null" << std::endl;
    return nullptr;
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

  std::string unitName = decl->getName();
  std::vector<std::shared_ptr<tir::Node>> nodes;

  if (auto classDecl =
          std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(decl)) {
    auto classNodes = buildClassDecl(classDecl);
    nodes.insert(nodes.end(), classNodes.begin(), classNodes.end());
  } else if (auto interfaceDecl =
                 std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                     decl)) {
  } else {
    throw std::runtime_error("TIRBuilder::buildProgram: only ClassDecl and "
                             "InterfaceDecl supported at top level");
  }

  return std::make_shared<tir::CompUnit>(unitName, nodes);
}

std::shared_ptr<FuncDecl>
TIRBuilder::buildMethodDecl(std::shared_ptr<parsetree::ast::MethodDecl> node) {
  if (!node) {
    throw std::runtime_error("TIRBuilder::buildMethodDecl: node is null");
  }

  std::vector<std::shared_ptr<Temp>> params;
  for (const auto &param : node->getParams()) {
    auto name = param->getName();
    params.push_back(std::make_shared<Temp>(name));
  }

  auto body = buildBlock(node->getMethodBody());

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

//////////////////// Pass Runner ////////////////////

void TIRBuilder::run() {
  for (auto ast : astManager->getASTs()) {
    auto compUnit = buildProgram(ast);
    compUnit->print(std::cout);
    compUnits.push_back(compUnit);
  }
}

std::ostream &TIRBuilder::print(std::ostream &os) {
  for (auto compUnit : compUnits) {
    compUnit->print(os, 0);
  }
  return os;
}

} // namespace tir
