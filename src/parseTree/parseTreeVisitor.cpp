#include "parseTreeVisitor.hpp"
#include "parseTree.hpp"

#include <list>

namespace parsetree {

using nodeType = Node::Type;
using NodePtr = std::shared_ptr<Node>;

std::shared_ptr<ast::ProgramDecl> visitProgramDecl(const NodePtr &node) {
  // Validate the node
  check_node_type(node, nodeType::ProgramDecl);
  check_num_children(node, 3, 3);

  // Visit the package declaration
  auto package = visitPackageDecl(node->child_at(0));

  // Visit the import declarations
  std::vector<ast::ImportDecl> imports;
  visitListPattern<nodeType::ImportDeclList, ast::ImportDecl, true>(
      node->child_at(1), imports);

  // Visit the body, if it exists
  std::shared_ptr<ast::CodeBody> body_ast_node = nullptr;
  if (auto body = node->child_at(2)) {
    switch (body->get_node_type()) {
    case nodeType::ClassDecl:
      body_ast_node = visitClassDecl(body);
      break;
    case nodeType::InterfaceDecl:
      body_ast_node = visitInterfaceDecl(body);
      break;
    default:
      break;
    }
  } else {
    std::cout << "There is no code body? " << node->child_at(2) << std::endl;
  }

  // Return the constructed AST node
  return std::make_shared<ast::ProgramDecl>(package, std::move(imports),
                                            body_ast_node);
}

std::shared_ptr<ast::QualifiedIdentifier>
visitPackageDecl(const NodePtr &node) {
  if (!node)
    return nullptr;
  check_node_type(node, nodeType::PackageDecl);
  check_num_children(node, 1, 1);
  return visitQualifiedIdentifier(node->child_at(0));
}

template <>
ast::ImportDecl visit<nodeType::ImportDeclList>(const NodePtr &node) {
  check_num_children(node, 1, 1);
  auto id = visitQualifiedIdentifier(node->child_at(0));

  switch (node->get_node_type()) {
  case nodeType::SingleImportDecl:
    return ast::ImportDecl{id, false};
  case nodeType::MultiImportDecl:
    return ast::ImportDecl{id, true};
  default:
    throw std::runtime_error(
        "visit<ImportDeclList> called on an invalid node type");
  }
}

// Class Declaration

std::shared_ptr<ast::ClassDecl> visitClassDecl(const NodePtr &node) {
  check_node_type(node, nodeType::ClassDecl);
  check_num_children(node, 5, 5);

  // Visit the modifiers identifier etc
  std::shared_ptr<ast::Modifiers> modifiers = std::make_shared<ast::Modifiers>(
      visitModifierList(node->child_at(0), ast::Modifiers{}));
  auto name = visitIdentifier(node->child_at(1));
  auto super = visitSuper(node->child_at(2));

  std::vector<std::shared_ptr<ast::QualifiedIdentifier>> interfaces;
  visitListPattern<nodeType::InterfaceTypeList,
                   std::shared_ptr<ast::QualifiedIdentifier>, true>(
      node->child_at(3), interfaces);

  // visit Class Body
  std::vector<std::shared_ptr<ast::Decl>> classBodyDecls;
  visitListPattern<nodeType::ClassBodyDeclList, std::shared_ptr<ast::Decl>,
                   true>(node->child_at(4), classBodyDecls);

  // Return the constructed AST node
  return std::make_shared<ast::ClassDecl>(modifiers, name, super, interfaces,
                                          classBodyDecls);
}

std::shared_ptr<ast::QualifiedIdentifier> visitSuper(const NodePtr &node) {
  if (!node)
    return nullptr;
  check_node_type(node, nodeType::SuperClass);
  check_num_children(node, 1, 1);
  return visitQualifiedIdentifier(node->child_at(0));
}

template <>
std::shared_ptr<ast::QualifiedIdentifier>
visit<nodeType::InterfaceTypeList>(const NodePtr &node) {
  return visitQualifiedIdentifier(node);
}

template <>
std::shared_ptr<ast::Decl>
visit<nodeType::ClassBodyDeclList>(const NodePtr &node) {
  switch (node->get_node_type()) {
  case nodeType::FieldDecl:
    return visitFieldDecl(node);
  case nodeType::MethodDecl:
    return visitMethodDecl(node);
  case nodeType::ConstructorDecl:
    return visitConstructorDecl(node);
  default:
    throw std::runtime_error("Unexpected node type in ClassBodyDeclList");
  }
}

// Field Declaration

std::shared_ptr<ast::FieldDecl> visitFieldDecl(const NodePtr &node) {
  check_node_type(node, nodeType::FieldDecl);
  check_num_children(node, 3, 3);

  std::shared_ptr<ast::Modifiers> modifiers =
      std::make_shared<ast::Modifiers>(visitModifierList(node->child_at(0)));
  auto type = visitType(node->child_at(1));

  auto decl = visitVariableDeclarator(node->child_at(1), node->child_at(2));
  return std::make_shared<ast::FieldDecl>(modifiers, decl.type, decl.name);
}

// Method Declaration

std::shared_ptr<ast::MethodDecl> visitMethodDecl(const NodePtr &node) {
  check_node_type(node, nodeType::MethodDecl);
  check_num_children(node, 4, 5);

  // Visit the modifiers
  std::shared_ptr<ast::Modifiers> modifiers =
      std::make_shared<ast::Modifiers>(visitModifierList(node->child_at(0)));
  // type could be void
  std::shared_ptr<ast::Type> type =
      (node->get_num_children() == 5) ? visitType(node->child_at(1)) : nullptr;
  std::string name = visitIdentifier(node->child_at(type ? 2 : 1));

  std::vector<std::shared_ptr<ast::VarDecl>> params;
  // visit params
  visitListPattern<nodeType::ParameterList, std::shared_ptr<ast::VarDecl>,
                   true>(node->child_at(type ? 3 : 2), params);

  // visit body
  std::shared_ptr<ast::Block> body =
      node->child_at(type ? 4 : 3) ? visitBlock(node->child_at(type ? 4 : 3))
                                   : nullptr;

  return std::make_shared<ast::MethodDecl>(modifiers, name, type, params, false,
                                           body);
}

std::shared_ptr<ast::MethodDecl> visitConstructorDecl(const NodePtr &node) {
  check_node_type(node, nodeType::ConstructorDecl);
  check_num_children(node, 4, 4);

  // need to visit modifier, identifier and parameters
  std::shared_ptr<ast::Modifiers> modifiers =
      std::make_shared<ast::Modifiers>(visitModifierList(node->child_at(0)));
  auto name = visitIdentifier(node->child_at(1));

  std::vector<std::shared_ptr<ast::VarDecl>> params;
  visitListPattern<nodeType::ParameterList, std::shared_ptr<ast::VarDecl>,
                   true>(node->child_at(2), params);

  // visit body if have one
  std::shared_ptr<ast::Block> body =
      node->child_at(3) ? visitBlock(node->child_at(3)) : nullptr;

  return std::make_shared<ast::MethodDecl>(modifiers, name, nullptr, params,
                                           true, body);
}

template <>
std::shared_ptr<ast::VarDecl>
visit<nodeType::ParameterList>(const NodePtr &node) {
  check_node_type(node, nodeType::Parameter);
  check_num_children(node, 2, 2);

  auto type = visitType(node->child_at(0));
  auto name = visitIdentifier(node->child_at(1));

  return std::make_shared<ast::VarDecl>(type, name);
}

// Interface Declaration

std::shared_ptr<ast::InterfaceDecl> visitInterfaceDecl(const NodePtr &node) {
  check_node_type(node, nodeType::InterfaceDecl);
  check_num_children(node, 4, 4);

  std::shared_ptr<ast::Modifiers> modifiers = std::make_shared<ast::Modifiers>(
      visitModifierList(node->child_at(0), ast::Modifiers{}));
  auto name = visitIdentifier(node->child_at(1));

  std::vector<std::shared_ptr<ast::QualifiedIdentifier>> extends;
  visitListPattern<nodeType::InterfaceTypeList,
                   std::shared_ptr<ast::QualifiedIdentifier>, true>(
      node->child_at(2), extends);

  std::vector<std::shared_ptr<ast::Decl>> interfaceBodyDecls;
  visitListPattern<nodeType::InterfaceBodyDeclList, std::shared_ptr<ast::Decl>,
                   true>(node->child_at(3), interfaceBodyDecls);

  return std::make_shared<ast::InterfaceDecl>(modifiers, name, extends,
                                              interfaceBodyDecls);
}

// Abstract MethodDeclaration

std::shared_ptr<ast::MethodDecl> visitAbstractMethodDecl(const NodePtr &node) {
  check_node_type(node, nodeType::AbstractMethodDecl);
  check_num_children(node, 3, 4);

  std::shared_ptr<ast::Modifiers> modifiers =
      std::make_shared<ast::Modifiers>(visitModifierList(node->child_at(0)));
  // type could be void
  auto type =
      (node->get_num_children() == 4) ? visitType(node->child_at(1)) : nullptr;
  auto name = visitIdentifier(node->child_at(type ? 2 : 1));

  std::vector<std::shared_ptr<ast::VarDecl>> params;
  visitListPattern<nodeType::ParameterList, std::shared_ptr<ast::VarDecl>,
                   true>(node->child_at(type ? 3 : 2), params);

  // need to set as abstract
  if (!modifiers->isAbstract()) {
    modifiers->setAbstract();
  }

  return std::make_shared<ast::MethodDecl>(modifiers, name, type, params, false,
                                           nullptr);
}

template <>
std::shared_ptr<ast::Decl>
visit<nodeType::InterfaceBodyDeclList>(const NodePtr &node) {
  return visitAbstractMethodDecl(node);
}

// Expressions
std::list<ast::ExprOp> visitExpr(const NodePtr &node) {
  if (!node) {
    return std::list<ast::ExprOp>();
  }

  // check_node_type(node, nodeType::Expression);

  // unimplemented yet
  return std::list<ast::ExprOp>();
}

std::shared_ptr<ast::StatementExpr> visitStatementExpr(const NodePtr &node) {
  switch (node->get_node_type()) {
  case nodeType::Assignment:
    // TODO
    return std::make_shared<ast::Assignment>();
  case nodeType::MethodInvocation:
    return visitMethodInvocation(node);
  case nodeType::ClassCreation:
    // TODO
    return std::make_shared<ast::ClassCreation>();
  default:
    throw std::runtime_error("Not a statementExpr!");
  }
}

std::shared_ptr<ast::MethodInvocation>
visitMethodInvocation(const NodePtr &node) {
  check_node_type(node, nodeType::MethodInvocation);
  check_num_children(node, 2, 3);
  std::shared_ptr<ast::QualifiedIdentifier> qid;
  if (node->get_num_children() == 2) {
    qid = visitQualifiedIdentifier(node->child_at(0));
    // TODO: args
  } else {
    qid = visitQualifiedIdentifier(node->child_at(1));
    // TODO: expr, args
  }
  return std::make_shared<ast::MethodInvocation>(
      qid, std::vector<std::shared_ptr<ast::Expr>>());
}

// Statements

void visitStatementList(const NodePtr &node,
                        std::vector<std::shared_ptr<ast::Stmt>> &statements) {
  check_node_type(node, nodeType::StatementList);
  check_num_children(node, 1, 2);
  auto child = node->child_at(0);
  if (!child) {
    // TODO
  }
  std::shared_ptr<ast::Stmt> astNode;
  switch (child->get_node_type()) {
  case nodeType::Block:
    astNode = visitBlock(child);
    break;
  case nodeType::ReturnStatement:
    astNode = visitReturnStatement(child);
    break;
  case nodeType::IfStatement:
    astNode = visitIfStatement(child);
    break;
  case nodeType::WhileStatement:
    astNode = visitWhileStatement(child);
    break;
  case nodeType::ForStatement:
    astNode = visitForStatement(child);
    break;
  case nodeType::ExprStatement:
    astNode = visitExprStatement(child);
    break;
  default:
    astNode = visitStatement(child);
  }
  statements.push_back(astNode);
  if (node->get_num_children() == 2) {
    visitStatementList(node->child_at(1), statements);
  }
}

std::shared_ptr<ast::Stmt> visitStatement(const NodePtr &node) {
  check_node_type(node, nodeType::Statement);
  // TODO
  return std::make_shared<ast::Stmt>();
}

std::shared_ptr<ast::Block> visitBlock(const NodePtr &node) {
  check_node_type(node, nodeType::Block);
  check_num_children(node, 1, 1);
  // TODO: check implementation
  if (node->child_at(0) == nullptr) {
    // this case is when public abstract int foo() {}
    // so it should consider an empty block
    return std::make_shared<ast::Block>();
  }
  std::vector<std::shared_ptr<ast::Stmt>> statements;
  visitStatementList(node->child_at(0), statements);
  return std::make_shared<ast::Block>(statements);
}

std::shared_ptr<ast::ReturnStmt> visitReturnStatement(const NodePtr &node) {
  check_node_type(node, nodeType::ReturnStatement);
  check_num_children(node, 0, 1);
  // TODO
  return std::make_shared<ast::ReturnStmt>();
}

std::shared_ptr<ast::IfStmt> visitIfStatement(const NodePtr &node) {
  check_node_type(node, nodeType::IfStatement);
  // TODO
  return std::make_shared<ast::IfStmt>();
}

std::shared_ptr<ast::WhileStmt> visitWhileStatement(const NodePtr &node) {
  check_node_type(node, nodeType::WhileStatement);
  // TODO
  return std::make_shared<ast::WhileStmt>();
}

std::shared_ptr<ast::ForStmt> visitForStatement(const NodePtr &node) {
  check_node_type(node, nodeType::ForStatement);
  // TODO
  return std::make_shared<ast::ForStmt>();
}

std::shared_ptr<ast::ExpressionStmt> visitExprStatement(const NodePtr &node) {
  check_node_type(node, nodeType::ExprStatement);
  check_num_children(node, 1, 1);
  if (!node->child_at(0)) {
    throw std::runtime_error("Invalid statmementExpr");
  }
  return std::make_shared<ast::ExpressionStmt>(
      visitStatementExpr(node->child_at(0)));
}

// Leaf Nodes!!

std::shared_ptr<ast::QualifiedIdentifier>
visitQualifiedIdentifier(const NodePtr &node,
                         std::shared_ptr<ast::QualifiedIdentifier> ast_node) {
  check_node_type(node, nodeType::QualifiedName);
  check_num_children(node, 1, 2);

  if (!ast_node) {
    ast_node = std::make_shared<ast::QualifiedIdentifier>();
  }

  if (node->get_num_children() == 1) {
    ast_node->addIdentifier(visitIdentifier(node->child_at(0)));
  } else {
    ast_node = visitQualifiedIdentifier(node->child_at(0), ast_node);
    ast_node->addIdentifier(visitIdentifier(node->child_at(1)));
  }
  return ast_node;
}

std::string visitIdentifier(const NodePtr &node) {
  check_node_type(node, nodeType::Identifier);
  return std::dynamic_pointer_cast<Identifier>(node)->get_name();
}

ast::Modifiers visitModifierList(const NodePtr &node,
                                 ast::Modifiers modifiers) {
  if (!node) {
    return modifiers;
  }
  check_node_type(node, nodeType::ModifierList);
  check_num_children(node, 1, 2);

  if (node->get_num_children() == 1) {
    modifiers.set(visitModifier(node->child_at(0)));
  } else if (node->get_num_children() == 2) {
    modifiers = visitModifierList(node->child_at(0), modifiers);
    modifiers.set(visitModifier(node->child_at(1)));
  }
  return modifiers;
}

Modifier visitModifier(const NodePtr &node) {
  check_node_type(node, nodeType::Modifier);
  return *std::dynamic_pointer_cast<Modifier>(node);
}

std::shared_ptr<ast::Type> visitType(const NodePtr &node) {
  check_num_children(node, 1, 1);

  auto innerType = node->child_at(0);
  std::shared_ptr<ast::Type> elemType;

  if (innerType->get_node_type() == nodeType::BasicType) {
    elemType = std::make_shared<ast::BuiltInType>(
        std::dynamic_pointer_cast<BasicType>(innerType)->get_type());
  } else if (innerType->get_node_type() == nodeType::QualifiedName) {
    elemType = std::make_shared<ast::ReferenceType>(
        visitQualifiedIdentifier(innerType));
  } else {
    throw std::runtime_error(
        "Expected a BasicType or QualifiedIdentifier node but got " +
        innerType->type_string());
  }

  if (node->get_node_type() == nodeType::ArrayType) {
    return std::make_shared<ast::ArrayType>(elemType);
  }
  return elemType;
}

VariableDecl visitVariableDeclarator(const NodePtr &typeNode,
                                     const NodePtr &declNode) {
  check_node_type(declNode, nodeType::Variable);
  check_num_children(declNode, 1, 2);
  auto type = visitType(typeNode);

  auto nameNode = declNode->child_at(0);
  auto name = visitIdentifier(nameNode);

  std::list<ast::ExprOp> init;
  if (declNode->get_num_children() == 2) {
    init = visitExpr(declNode->child_at(1));
  }
  return VariableDecl{type, name, init};
}

} // namespace parsetree
