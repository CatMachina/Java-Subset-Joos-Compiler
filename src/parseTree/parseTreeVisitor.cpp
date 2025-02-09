#include "parseTree/parseTreeVisitor.hpp"
#include "parseTree/parseTree.hpp"

#include <list>

namespace parsetree {

using NodeType = Node::Type;
using NodePtr = std::shared_ptr<Node>;

// Program Declaration visitors

std::shared_ptr<ast::ProgramDecl>
ParseTreeVisitor::visitProgramDecl(const NodePtr &node) {
  // Validate the node
  check_node_type(node, NodeType::ProgramDecl);
  check_num_children(node, 3, 3);

  // Visit the package declaration
  auto package = visitPackageDecl(node->child_at(0));

  // Visit the import declarations
  std::vector<ast::ImportDecl> imports;
  visitListPattern<NodeType::ImportDeclList, ast::ImportDecl, true>(
      node->child_at(1), imports);

  // Visit the body, if it exists
  std::shared_ptr<ast::CodeBody> body_ast_node = nullptr;
  if (auto body = node->child_at(2)) {
    switch (body->get_node_type()) {
    case NodeType::ClassDecl:
      body_ast_node = visitClassDecl(body);
      break;
    case NodeType::InterfaceDecl:
      body_ast_node = visitInterfaceDecl(body);
      break;
    default:
      break;
    }
  } else {
    std::cout << "There is no code body? " << node->child_at(2) << std::endl;
  }

  // Return the constructed AST node
  return envManager.BuildProgramDecl(package, imports, body_ast_node);
}

std::shared_ptr<ast::QualifiedIdentifier>
ParseTreeVisitor::visitPackageDecl(const NodePtr &node) {
  if (!node)
    return nullptr;
  check_node_type(node, NodeType::PackageDecl);
  check_num_children(node, 1, 1);
  return visitQualifiedIdentifier(node->child_at(0));
}

template <>
ast::ImportDecl
ParseTreeVisitor::visit<NodeType::ImportDeclList>(const NodePtr &node) {
  check_num_children(node, 1, 1);
  auto id = visitQualifiedIdentifier(node->child_at(0));

  switch (node->get_node_type()) {
  case NodeType::SingleImportDecl:
    return ast::ImportDecl{id, /* hasStar */ false};
  case NodeType::MultiImportDecl:
    return ast::ImportDecl{id, /* hasStar */ true};
  default:
    throw std::runtime_error(
        "visit<ImportDeclList> called on an invalid node type");
  }
}

// Class Declaration visitors

std::shared_ptr<ast::ClassDecl>
ParseTreeVisitor::visitClassDecl(const NodePtr &node) {
  check_node_type(node, NodeType::ClassDecl);
  check_num_children(node, 5, 5);

  // Visit the modifiers identifier etc
  std::shared_ptr<ast::Modifiers> modifiers = std::make_shared<ast::Modifiers>(
      visitModifierList(node->child_at(0), ast::Modifiers{}));
  auto name = visitIdentifier(node->child_at(1));
  auto super = visitSuper(node->child_at(2));

  std::vector<std::shared_ptr<ast::QualifiedIdentifier>> interfaces;
  visitListPattern<NodeType::InterfaceTypeList,
                   std::shared_ptr<ast::QualifiedIdentifier>, true>(
      node->child_at(3), interfaces);

  // Visit Class Body
  std::vector<std::shared_ptr<ast::Decl>> classBodyDecls;
  visitListPattern<NodeType::ClassBodyDeclList, std::shared_ptr<ast::Decl>,
                   true>(node->child_at(4), classBodyDecls);

  // Return the constructed AST node
  return envManager.BuildClassDecl(modifiers, name, super, interfaces,
                                   classBodyDecls);
}

std::shared_ptr<ast::QualifiedIdentifier>
ParseTreeVisitor::visitSuper(const NodePtr &node) {
  if (!node)
    return nullptr;
  check_node_type(node, NodeType::SuperClass);
  check_num_children(node, 1, 1);
  return visitQualifiedIdentifier(node->child_at(0));
}

template <>
std::shared_ptr<ast::QualifiedIdentifier>
ParseTreeVisitor::visit<NodeType::InterfaceTypeList>(const NodePtr &node) {
  return visitQualifiedIdentifier(node);
}

template <>
std::shared_ptr<ast::Decl>
ParseTreeVisitor::visit<NodeType::ClassBodyDeclList>(const NodePtr &node) {
  switch (node->get_node_type()) {
  case NodeType::FieldDecl:
    return visitFieldDecl(node);
  case NodeType::MethodDecl:
    return visitMethodDecl(node);
  case NodeType::ConstructorDecl:
    return visitConstructorDecl(node);
  default:
    throw std::runtime_error("Unexpected node type in ClassBodyDeclList");
  }
}

// Field Declaration visitors

std::shared_ptr<ast::FieldDecl>
ParseTreeVisitor::visitFieldDecl(const NodePtr &node) {
  check_node_type(node, NodeType::FieldDecl);
  check_num_children(node, 3, 3);

  std::shared_ptr<ast::Modifiers> modifiers =
      std::make_shared<ast::Modifiers>(visitModifierList(node->child_at(0)));

  auto decl = visitVariableDeclarator(node->child_at(1), node->child_at(2));
  return envManager.BuildFieldDecl(modifiers, decl.type, decl.name, decl.init);
}

// Method Declaration

std::shared_ptr<ast::MethodDecl>
ParseTreeVisitor::visitMethodDecl(const NodePtr &node) {
  check_node_type(node, NodeType::MethodDecl);
  check_num_children(node, 4, 5);

  envManager.ClearLocalScope();

  // Visit the modifiers
  std::shared_ptr<ast::Modifiers> modifiers =
      std::make_shared<ast::Modifiers>(visitModifierList(node->child_at(0)));
  // type could be void
  std::shared_ptr<ast::Type> type =
      (node->num_children() == 5) ? visitType(node->child_at(1)) : nullptr;
  std::string name = visitIdentifier(node->child_at(type ? 2 : 1));

  std::vector<std::shared_ptr<ast::VarDecl>> params;
  // visit params
  visitListPattern<NodeType::ParameterList, std::shared_ptr<ast::VarDecl>,
                   true>(node->child_at(type ? 3 : 2), params);

  // visit body
  std::shared_ptr<ast::Block> body =
      node->child_at(type ? 4 : 3) ? visitBlock(node->child_at(type ? 4 : 3))
                                   : nullptr;

  return envManager.BuildMethodDecl(modifiers, name, type, params, false, body);
}

std::shared_ptr<ast::MethodDecl>
ParseTreeVisitor::visitConstructorDecl(const NodePtr &node) {
  check_node_type(node, NodeType::ConstructorDecl);
  check_num_children(node, 4, 4);

  envManager.ClearLocalScope();

  // need to visit modifier, identifier and parameters
  std::shared_ptr<ast::Modifiers> modifiers =
      std::make_shared<ast::Modifiers>(visitModifierList(node->child_at(0)));
  auto name = visitIdentifier(node->child_at(1));

  std::vector<std::shared_ptr<ast::VarDecl>> params;
  visitListPattern<NodeType::ParameterList, std::shared_ptr<ast::VarDecl>,
                   true>(node->child_at(2), params);

  // visit body if have one
  std::shared_ptr<ast::Block> body =
      node->child_at(3) ? visitBlock(node->child_at(3)) : nullptr;

  return envManager.BuildMethodDecl(modifiers, name, nullptr, params, true,
                                    body);
}

template <>
std::shared_ptr<ast::VarDecl>
ParseTreeVisitor::visit<NodeType::ParameterList>(const NodePtr &node) {
  check_node_type(node, NodeType::Parameter);
  check_num_children(node, 2, 2);

  auto type = visitType(node->child_at(0));
  auto name = visitIdentifier(node->child_at(1));

  return envManager.BuildVarDecl(type, name);
}

// Interface Declaration

std::shared_ptr<ast::InterfaceDecl>
ParseTreeVisitor::visitInterfaceDecl(const NodePtr &node) {
  check_node_type(node, NodeType::InterfaceDecl);
  check_num_children(node, 4, 4);

  std::shared_ptr<ast::Modifiers> modifiers = std::make_shared<ast::Modifiers>(
      visitModifierList(node->child_at(0), ast::Modifiers{}));
  auto name = visitIdentifier(node->child_at(1));

  std::vector<std::shared_ptr<ast::QualifiedIdentifier>> extends;
  visitListPattern<NodeType::InterfaceTypeList,
                   std::shared_ptr<ast::QualifiedIdentifier>, true>(
      node->child_at(2), extends);

  std::vector<std::shared_ptr<ast::Decl>> interfaceBodyDecls;
  visitListPattern<NodeType::InterfaceBodyDeclList, std::shared_ptr<ast::Decl>,
                   true>(node->child_at(3), interfaceBodyDecls);

  return envManager.BuildInterfaceDecl(modifiers, name, extends,
                                       interfaceBodyDecls);
}

// Abstract Method Declaration

std::shared_ptr<ast::MethodDecl>
ParseTreeVisitor::visitAbstractMethodDecl(const NodePtr &node) {
  check_node_type(node, NodeType::AbstractMethodDecl);
  check_num_children(node, 3, 4);

  std::shared_ptr<ast::Modifiers> modifiers =
      std::make_shared<ast::Modifiers>(visitModifierList(node->child_at(0)));
  // type could be void
  auto type =
      (node->num_children() == 4) ? visitType(node->child_at(1)) : nullptr;
  auto name = visitIdentifier(node->child_at(type ? 2 : 1));

  std::vector<std::shared_ptr<ast::VarDecl>> params;
  visitListPattern<NodeType::ParameterList, std::shared_ptr<ast::VarDecl>,
                   true>(node->child_at(type ? 3 : 2), params);

  // need to set as abstract
  if (!modifiers->isAbstract()) {
    modifiers->setAbstract();
  }

  return envManager.BuildMethodDecl(modifiers, name, type, params, false,
                                    nullptr);
}

template <>
std::shared_ptr<ast::Decl>
ParseTreeVisitor::visit<NodeType::InterfaceBodyDeclList>(const NodePtr &node) {
  return visitAbstractMethodDecl(node);
}

// Statements

void ParseTreeVisitor::visitStatementList(
    const NodePtr &node, std::vector<std::shared_ptr<ast::Stmt>> &statements) {
  check_node_type(node, NodeType::StatementList);
  check_num_children(node, 1, 2);
  auto child = node->child_at(0);
  if (!child) {
    // TODO
  }
  std::shared_ptr<ast::Stmt> astNode = visitStatement(node);
  statements.push_back(astNode);
  if (node->num_children() == 2) {
    visitStatementList(node->child_at(1), statements);
  }
}

std::shared_ptr<ast::Stmt>
ParseTreeVisitor::visitStatement(const NodePtr &node) {
  check_node_type(node, NodeType::Statement);
  std::shared_ptr<ast::Stmt> astNode;
  switch (node->get_node_type()) {
  case NodeType::Block:
    return visitBlock(node);
  case NodeType::ReturnStatement:
    return visitReturnStatement(node);
  case NodeType::IfStatement:
    return visitIfStatement(node);
  case NodeType::WhileStatement:
    return visitWhileStatement(node);
  case NodeType::ForStatement:
    return visitForStatement(node);
  case NodeType::ExprStatement:
    return visitExprStatement(node);
  default:
    throw std::runtime_error("Not a statement!");
  }
}

std::shared_ptr<ast::Block> ParseTreeVisitor::visitBlock(const NodePtr &node) {
  check_node_type(node, NodeType::Block);
  check_num_children(node, 1, 1);
  // TODO: check implementation
  if (node->child_at(0) == nullptr) {
    // this case is when public abstract int foo() {}
    // so it should consider an empty block
    return std::make_shared<ast::Block>();
  }
  std::vector<std::shared_ptr<ast::Stmt>> statements;

  // going into new scope
  size_t scope = envManager.EnterNewScope();
  visitStatementList(node->child_at(0), statements);
  envManager.ExitScope(scope);
  return std::make_shared<ast::Block>(statements);
}

std::shared_ptr<ast::ReturnStmt>
ParseTreeVisitor::visitReturnStatement(const NodePtr &node) {
  check_node_type(node, NodeType::ReturnStatement);
  check_num_children(node, 0, 1);
  if (node->num_children() == 0) {
    return std::make_shared<ast::ReturnStmt>();
  } else {
    return std::make_shared<ast::ReturnStmt>(
        visitExpression(node->child_at(0)));
  }
}

std::shared_ptr<ast::IfStmt>
ParseTreeVisitor::visitIfStatement(const NodePtr &node) {
  check_node_type(node, NodeType::IfStatement);
  check_num_children(node, 2, 3);

  auto scope = envManager.EnterNewScope();
  auto stmt = visitStatement(node->child_at(1));
  envManager.ExitScope(scope);

  return std::make_shared<ast::IfStmt>(
      visitExpression(node->child_at(0)), stmt,
      node->num_children() == 3 ? visitStatement(node->child_at(2)) : nullptr);
}

std::shared_ptr<ast::WhileStmt>
ParseTreeVisitor::visitWhileStatement(const NodePtr &node) {
  check_node_type(node, NodeType::WhileStatement);
  check_num_children(node, 2, 2);

  auto scope = envManager.EnterNewScope();
  auto stmt = visitStatement(node->child_at(1));
  envManager.ExitScope(scope);

  return std::make_shared<ast::WhileStmt>(visitExpression(node->child_at(0)),
                                          stmt);
}

std::shared_ptr<ast::ForStmt>
ParseTreeVisitor::visitForStatement(const NodePtr &node) {
  check_node_type(node, NodeType::ForStatement);
  check_num_children(node, 4, 4);

  std::shared_ptr<ast::Stmt> init = nullptr;
  std::shared_ptr<ast::Expr> condition = nullptr;
  std::shared_ptr<ast::Stmt> update = nullptr;
  std::shared_ptr<ast::Stmt> body = nullptr;

  auto scope = envManager.EnterNewScope();
  if (auto initNode = node->child_at(0)) {
    init = visitStatement(initNode);
  }
  if (auto conditionNode = node->child_at(1)) {
    condition = visitExpression(conditionNode);
  }
  if (auto updateNode = node->child_at(2)) {
    update = visitStatement(updateNode);
  }
  // body is always not null
  body = visitStatement(node->child_at(3));
  envManager.ExitScope(scope);

  return std::make_shared<ast::ForStmt>(init, condition, update, body);
}

std::shared_ptr<ast::ExpressionStmt>
ParseTreeVisitor::visitExprStatement(const NodePtr &node) {
  check_node_type(node, NodeType::ExprStatement);
  check_num_children(node, 1, 1);
  if (!node->child_at(0)) {
    throw std::runtime_error("Invalid StatementExpr");
  }
  return std::make_shared<ast::ExpressionStmt>(
      visitStatementExpr(node->child_at(0)));
}

// Leaf Nodes!!

std::shared_ptr<ast::QualifiedIdentifier>
ParseTreeVisitor::visitQualifiedIdentifier(
    const NodePtr &node, std::shared_ptr<ast::QualifiedIdentifier> ast_node) {
  check_node_type(node, NodeType::QualifiedName);
  check_num_children(node, 1, 2);

  if (!ast_node) {
    ast_node = envManager.BuildQualifiedIdentifier(std::vector<std::string>());
  }

  if (node->num_children() == 1) {
    ast_node->addIdentifier(visitIdentifier(node->child_at(0)));
  } else {
    ast_node = visitQualifiedIdentifier(node->child_at(0), ast_node);
    ast_node->addIdentifier(visitIdentifier(node->child_at(1)));
  }
  return ast_node;
}

std::string ParseTreeVisitor::visitIdentifier(const NodePtr &node) {
  check_node_type(node, NodeType::Identifier);
  return std::dynamic_pointer_cast<Identifier>(node)->get_name();
}

ast::Modifiers ParseTreeVisitor::visitModifierList(const NodePtr &node,
                                                   ast::Modifiers modifiers) {
  if (!node) {
    return modifiers;
  }
  check_node_type(node, NodeType::ModifierList);
  check_num_children(node, 1, 2);

  if (node->num_children() == 1) {
    modifiers.set(visitModifier(node->child_at(0)));
  } else if (node->num_children() == 2) {
    modifiers = visitModifierList(node->child_at(0), modifiers);
    modifiers.set(visitModifier(node->child_at(1)));
  }
  return modifiers;
}

Modifier ParseTreeVisitor::visitModifier(const NodePtr &node) {
  check_node_type(node, NodeType::Modifier);
  return *std::dynamic_pointer_cast<Modifier>(node);
}

std::shared_ptr<ast::Type> ParseTreeVisitor::visitType(const NodePtr &node) {
  check_num_children(node, 1, 1);

  auto innerType = node->child_at(0);
  std::shared_ptr<ast::Type> elemType;

  if (innerType->get_node_type() == NodeType::BasicType) {
    elemType = envManager.BuildBasicType(
        std::dynamic_pointer_cast<ast::BasicType>(innerType)->getType());
  } else if (innerType->get_node_type() == NodeType::QualifiedName) {
    elemType = std::make_shared<ast::ReferenceType>(
        visitQualifiedIdentifier(innerType));
  } else {
    throw std::runtime_error(
        "Expected a BasicType or QualifiedIdentifier node but got " +
        innerType->type_string());
  }

  if (!elemType) {
    throw std::runtime_error(
        "Expected a BasicType or QualifiedIdentifier node");
  }

  if (node->get_node_type() == NodeType::ArrayType) {
    return envManager.BuildArrayType(elemType);
  } else if (node->get_node_type() == NodeType::Type) {
    return elemType;
  }
  throw std::runtime_error("Expected a Type or ArrayType node");
}

ParseTreeVisitor::VariableDecl
ParseTreeVisitor::visitVariableDeclarator(const NodePtr &typeNode,
                                          const NodePtr &declNode) {
  check_node_type(declNode, NodeType::Variable);
  check_num_children(declNode, 1, 2);
  auto type = visitType(typeNode);

  auto nameNode = declNode->child_at(0);
  auto name = visitIdentifier(nameNode);

  std::shared_ptr<ast::Expr> init;
  if (declNode->num_children() == 2) {
    init = visitExpression(declNode->child_at(1));
  }
  return VariableDecl{type, name, init};
}

} // namespace parsetree
