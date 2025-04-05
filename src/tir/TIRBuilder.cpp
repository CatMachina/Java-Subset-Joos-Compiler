#include "tir/TIRBuilder.hpp"
#include "codeGen/dispatchVector.hpp"

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
  return exprConverter->evaluateList(expr->getExprNodes());
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
    return std::make_shared<Seq>();
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

  std::vector<std::shared_ptr<Stmt>> stmts;

  auto lt = Label::generateName("if_true");
  auto lf = Label::generateName("if_false");
  auto le = Label::generateName("if_end");

  auto condition = buildExpr(node->getCondition());
  auto body = buildStmt(node->getIfBody());

  if (node->getElseBody() == nullptr) {
    // CJump(expr == 0, exit, true)
    stmts.push_back(
        std::make_shared<CJump>(BinOp::makeNegate(condition), le, lt));

    stmts.push_back(std::make_shared<Label>(lt));
    stmts.push_back(body);
    stmts.push_back(std::make_shared<Label>(le));

  } else {
    // CJump(expr == 0, false, true)
    stmts.push_back(
        std::make_shared<CJump>(BinOp::makeNegate(condition), lf, lt));

    // true
    stmts.push_back(std::make_shared<Label>(lt));
    stmts.push_back(body);
    // jump to end
    stmts.push_back(std::make_shared<Jump>(std::make_shared<Name>(le)));

    // false
    stmts.push_back(std::make_shared<Label>(lf));
    stmts.push_back(buildStmt(node->getElseBody()));

    // end
    stmts.push_back(std::make_shared<Label>(le));
  }

  return std::make_shared<Seq>(stmts);
}

std::shared_ptr<Stmt>
TIRBuilder::buildWhileStmt(std::shared_ptr<parsetree::ast::WhileStmt> node) {
  if (!node) {
    std::cout << "TIRBuilder::buildWhileStmt: node is null" << std::endl;
    return nullptr;
  }

  auto ls = Label::generateName("while_init");
  auto lt = Label::generateName("while_true");
  auto le = Label::generateName("while_end");
  std::vector<std::shared_ptr<Stmt>> stmts;

  // start
  stmts.push_back(std::make_shared<Label>(ls));

  // CJump(expr == 0, end, true)
  auto condition = buildExpr(node->getCondition());
  stmts.push_back(
      std::make_shared<CJump>(BinOp::makeNegate(condition), le, lt));

  // true
  stmts.push_back(std::make_shared<Label>(lt));
  auto stmt = buildStmt(node->getWhileBody());
  stmts.push_back(stmt);
  // jump to start
  stmts.push_back(std::make_shared<Jump>(std::make_shared<Name>(ls)));

  // end
  stmts.push_back(std::make_shared<Label>(le));

  return std::make_shared<Seq>(stmts);
}

std::shared_ptr<Stmt>
TIRBuilder::buildForStmt(std::shared_ptr<parsetree::ast::ForStmt> node) {
  if (!node) {
    std::cout << "TIRBuilder::buildForStmt: node is null" << std::endl;
    return nullptr;
  }

  auto ls = Label::generateName("for_init");
  auto lt = Label::generateName("for_true");
  auto le = Label::generateName("for_end");
  std::vector<std::shared_ptr<Stmt>> stmts;

  if (node->getForInit() != nullptr) {
    stmts.push_back(buildStmt(node->getForInit()));
  }

  // start
  stmts.push_back(std::make_shared<Label>(ls));

  // CJump(cond == 0, end, true)
  auto condition = buildExpr(node->getCondition());
  stmts.push_back(
      std::make_shared<CJump>(BinOp::makeNegate(condition), le, lt));

  // true
  stmts.push_back(std::make_shared<Label>(lt));
  auto body = buildStmt(node->getForBody());
  stmts.push_back(body);
  if (node->getForUpdate() != nullptr) {
    stmts.push_back(buildStmt(node->getForUpdate()));
  }
  // jump to start
  stmts.push_back(std::make_shared<Jump>(std::make_shared<Name>(ls)));

  // end
  stmts.push_back(std::make_shared<Label>(le));

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
    // native function handled by runtime.s
    if (!(methodDecl->getModifiers()->isNative())) {
      return buildMethodDecl(methodDecl);
    }
    return nullptr;

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

  auto compUnit = std::make_shared<tir::CompUnit>(unitName, nodes);
  currentProgram = compUnit;

  if (auto classDecl =
          std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(decl)) {
    auto classNodes = buildClassDecl(classDecl);
    // nodes.insert(nodes.end(), classNodes.begin(), classNodes.end());
    compUnit->insertNodes(classNodes);

    auto classDV = codegen::DispatchVectorBuilder::getDV(classDecl);

    // allocate memory for dv
    compUnit->appendField(
        exprConverter->codeGenLabels->getClassLabel(classDecl),
        tir::Call::makeMalloc(std::make_shared<tir::Const>(
            4 * (classDV->methodVector.size() + 1))));

    // put function labels in dv at correct offsets
    for (auto &method : classDV->methodVector) {
      if (!method)
        continue;
      auto methodLabel = exprConverter->codeGenLabels->getMethodLabel(method);
      compUnit->appendStartStmt(std::make_shared<tir::Move>(
          std::make_shared<tir::Mem>(std::make_shared<tir::BinOp>(
              tir::BinOp::ADD,
              std::make_shared<tir::Temp>(
                  exprConverter->codeGenLabels->getClassLabel(classDecl)),
              std::make_shared<tir::Const>(
                  4 * codegen::DispatchVectorBuilder::getAssignment(method)))),
          std::make_shared<tir::Name>(methodLabel, true)));
    }

    return compUnit;

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

  std::vector<std::shared_ptr<Stmt>> params;
  int argCount = 0;

  // this arg
  auto argName = exprConverter->codeGenLabels->kAbstractArgPrefix +
                 std::to_string(argCount++);
  params.push_back(std::make_shared<Move>(std::make_shared<Temp>("this"),
                                          std::make_shared<Name>(argName)));

  // move each value in virtual argument register from caller into parameter
  // temp
  for (const auto &param : node->getParams()) {
    auto paramName = exprConverter->codeGenLabels->getParameterLabel(param);
    auto regName = exprConverter->codeGenLabels->kAbstractArgPrefix +
                   std::to_string(argCount++);
    params.push_back(std::make_shared<Move>(std::make_shared<Temp>(paramName),
                                            std::make_shared<Name>(regName)));
  }

  auto body = buildBlock(node->getMethodBody());
  auto bodySeq = std::dynamic_pointer_cast<Seq>(body);
  if (!bodySeq) {
    throw std::runtime_error("TIRBuilder::buildMethodDecl: body is not a Seq");
  }
  for (auto &bodyStmt : bodySeq->getStmts()) {
    params.push_back(bodyStmt);
  }
  bodySeq = std::make_shared<Seq>(params);

  auto label = node->isStatic()
                   ? exprConverter->codeGenLabels->getStaticMethodLabel(node)
                   : exprConverter->codeGenLabels->getMethodLabel(node);
  std::shared_ptr<FuncDecl> funcIR = std::make_shared<FuncDecl>(
      node->getName(), bodySeq, node->getParams().size());
  currentProgram->appendFunc(label, funcIR);

  return funcIR;
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
  exprConverter->setCurrentClass(node);
  std::vector<std::shared_ptr<Node>> results;
  for (const auto &decl : node->getClassMembers()) {
    auto result = buildDecl(decl);
    if (result) {
      results.push_back(result);
    }
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
