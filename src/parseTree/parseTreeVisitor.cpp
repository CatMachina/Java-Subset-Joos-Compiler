#include "parseTreeVisitor.hpp"
#include "parseTree.hpp"

namespace parsetree {

using nodeType = Node::Type;
using NodePtr = std::shared_ptr<Node>;

std::shared_ptr<ast::ProgramDecl> visitProgramDecl(const NodePtr &node) {
  // Validate the node
  check_node_type(node, nodeType::ProgramDecl);
  check_num_children(node, 3, 3);

  // Visit the package declaration
  auto package = visitPackageDecl(node->child(0));

  // Visit the import declarations
  std::vector<ast::ImportDecl> imports;
  visitListPattern<nodeType::ImportDeclList, ast::ImportDecl, true>(
      node->child(1), imports);

  // Visit the body, if it exists
  std::shared_ptr<ast::CodeBody> body_ast_node = nullptr;
  if (auto body = node->child(2)) {
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
  }

  // Return the constructed AST node
  return std::make_shared<ast::ProgramDecl>(package, imports, body_ast_node);
}

std::shared_ptr<ast::QualifiedIdentifier>
visitPackageDecl(const NodePtr &node) {
  if (!node)
    return nullptr;
  check_node_type(node, nodeType::PackageDecl);
  check_num_children(node, 1, 1);
  return visitQualifiedIdentifier(node->child(0));
}

template <>
ast::ImportDecl visit<nodeType::ImportDeclList>(const NodePtr &node) {
  check_num_children(node, 1, 1);
  auto id = visitQualifiedIdentifier(node->child(0));

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
  ast::Modifiers modifiers =
      visitModifierList(node->child(0), ast::Modifiers{});
  auto name = visitIdentifier(node->child(1));
  auto super = visitSuper(node->child(2));

  std::vector<std::shared_ptr<ast::QualifiedIdentifier>> interfaces;
  visitListPattern<nodeType::InterfaceTypeList,
                   std::shared_ptr<ast::QualifiedIdentifier>, true>(
      node->child(3), interfaces);

  // visit Class Body
  std::vector<std::shared_ptr<ast::Decl>> classBodyDecls;
  visitListPattern<nodeType::ClassBodyDeclList, std::shared_ptr<ast::Decl>,
                   true>(node->child(4), classBodyDecls);

  // Return the constructed AST node
  return std::make_shared<ast::ClassDecl>(modifiers, name, super, interfaces,
                                          classBodyDecls);
}

std::shared_ptr<ast::QualifiedIdentifier> visitSuper(const NodePtr &node) {
  if (!node)
    return nullptr;
  check_node_type(node, nodeType::SuperClass);
  check_num_children(node, 1, 1);
  return visitQualifiedIdentifier(node->child(0));
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

  auto modifiers = visitModifierList(node->child(0));
  auto type = visitType(node->child(1));

  auto decl = visitVariableDeclarator(node->child(1), node->child(2));
  return std::make_shared<ast::FieldDecl>(modifiers, decl.type, decl.init);
}

// Method Declaration

std::shared_ptr<ast::MethodDecl> visitMethodDecl(const NodePtr &node) {
  check_node_type(node, nodeType::MethodDecl);
  check_num_children(node, 4, 5);

  // Visit the modifiers
  ast::Modifiers modifiers = visitModifierList(node->child(0));
  // type could be void
  std::shared_ptr<ast::Type> type =
      (node->num_children() == 5) ? visitType(node->child(1)) : nullptr;
  std::string name = visitIdentifier(node->child(type ? 2 : 1));

  std::vector<std::shared_ptr<ast::VarDecl>> params;
  // visit params
  visitListPattern<nodeType::FormalParameterList, std::shared_ptr<ast::VarDecl>,
                   true>(node->child(type ? 3 : 2), params);

  // visit body
  std::shared_ptr<ast::Stmt> body = node->child(type ? 4 : 3)
                                        ? visitBlock(node->child(type ? 4 : 3))
                                        : nullptr;

  return std::make_shared<ast::MethodDecl>(modifiers, name, type, params, false,
                                           body);
}

std::shared_ptr<ast::MethodDecl> visitConstructorDecl(const NodePtr &node) {
  check_node_type(node, nodeType::ConstructorDecl);
  check_num_children(node, 4, 4);

  // need to visit modifier, identifier and parameters
  auto modifiers = visitModifierList(node->child(0));
  auto name = visitIdentifier(node->child(1));

  std::vector<std::shared_ptr<ast::VarDecl>> params;
  visitListPattern<nodeType::FormalParameterList, std::shared_ptr<ast::VarDecl>,
                   true>(node->child(2), params);

  // visit body if have one
  std::shared_ptr<ast::Stmt> body =
      node->child(3) ? visitBlock(node->child(3)) : nullptr;

  return std::make_shared<ast::MethodDecl>(modifiers, name, nullptr, params,
                                           true, body);
}

// FIXME: we need formal parameter that strictly have 2 children!
template <>
std::shared_ptr<ast::VarDecl>
visit<nodeType::FormalParameterList>(const NodePtr &node) {
  check_node_type(node, nodeType::FormalParameter);
  check_num_children(node, 2, 2);

  auto type = visitType(node->child(0));
  auto name = visitIdentifier(node->child(1));

  return std::make_shared<ast::VarDecl>(type, name);
}

// Interface Declaration

std::shared_ptr<ast::InterfaceDecl> visitInterfaceDecl(const NodePtr &node) {
  check_node_type(node, nodeType::InterfaceDecl);
  check_num_children(node, 4, 4);

  ast::Modifiers modifiers =
      visitModifierList(node->child(0), ast::Modifiers{});
  auto name = visitIdentifier(node->child(1));

  std::vector<std::shared_ptr<ast::QualifiedIdentifier>> extends;
  visitListPattern<nodeType::InterfaceTypeList,
                   std::shared_ptr<ast::QualifiedIdentifier>, true>(
      node->child(2), extends);

  std::vector<std::shared_ptr<ast::Decl>> interfaceBodyDecls;
  visitListPattern<nodeType::InterfaceBodyDeclList, std::shared_ptr<ast::Decl>,
                   true>(node->child(3), interfaceBodyDecls);

  return std::make_shared<ast::InterfaceDecl>(modifiers, name, extends,
                                              interfaceBodyDecls);
}

// Abstract MethodDeclaration

std::shared_ptr<ast::MethodDecl> visitAbstractMethodDecl(const NodePtr &node) {
  check_node_type(node, nodeType::AbstractMethodDecl);
  check_num_children(node, 3, 4);

  ast::Modifiers modifiers = visitModifierList(node->child(0));
  // type could be void
  auto type = (node->num_children() == 4) ? visitType(node->child(1)) : nullptr;
  auto name = visitIdentifier(node->child(type ? 2 : 1));

  std::vector<std::shared_ptr<ast::VarDecl>> params;
  visitListPattern<nodeType::FormalParameterList, std::shared_ptr<ast::VarDecl>,
                   true>(node->child(type ? 3 : 2), params);

  // need to set as abstract
  if (!modifiers.isAbstract()) {
    modifiers.setAbstract();
  }

  return std::make_shared<ast::MethodDecl>(modifiers, name, type, params, false,
                                           nullptr);
}

template <>
std::shared_ptr<ast::Decl>
visit<nodeType::InterfaceMemberDeclList>(const NodePtr &node) {
  return visitAbstractMethodDecl(node);
}

// Expressions
std::list<ast::ExprOp> visitExpr(const NodePtr &node) {
  if (!node) {
    return list<ast::ExprOp>();
  }

  check_node_type(node, nodeType::Expression);

  // unimplemented yet
  return list<ast::ExprOp>();
}

// Statements
std::shared_ptr<ast::Stmt> visitBlock(const NodePtr &node) {
  check_node_type(node, nodeType::Block);
  check_num_children(node, 1, 1);
  if (node->child(0) == nullptr)
    return make_shared<ast::Block>();
  // Unimplemented yet
  return make_shared<ast::Block>();
}

// Leaf Nodes!!

std::shared_ptr<ast::QualifiedIdentifier> visitQualifiedIdentifier(
    const NodePtr &node,
    std::shared_ptr<ast::QualifiedIdentifier> ast_node = nullptr) {
  check_node_type(node, nodeType::QualifiedName);
  check_num_children(node, 1, 2);

  if (!ast_node) {
    ast_node = std::make_shared<ast::QualifiedIdentifier>();
  }

  if (node->num_children() == 1) {
    ast_node->addIdentifier(visitIdentifier(node->child(0)));
  } else {
    ast_node = visitQualifiedIdentifier(node->child(0), ast_node);
    ast_node->addIdentifier(visitIdentifier(node->child(1)));
  }
  return ast_node;
}

std::string visitIdentifier(const NodePtr &node) {
  check_node_type(node, nodeType::Identifier);
  return std::dynamic_pointer_cast<Identifier>(node)->get_name();
}

ast::Modifiers visitModifierList(const NodePtr &node,
                                 ast::Modifiers modifiers = {}) {
  if (!node) {
    return modifiers;
  }
  check_node_type(node, nodeType::ModifierList);
  check_num_children(node, 1, 2);

  modifiers.set(visitModifier(node->child(0)));
  if (node->num_children() == 2) {
    modifiers = visitModifierList(node->child(1), modifiers);
  }
  return modifiers;
}

Modifier visitModifier(const NodePtr &node) {
  check_node_type(node, nodeType::Modifier);
  return *std::dynamic_pointer_cast<Modifier>(node);
}

std::shared_ptr<ast::Type> visitType(const NodePtr &node) {
  check_num_children(node, 1, 1);

  auto elemType = node->child(0);
  std::shared_ptr<ast::Type> elemType = nullptr;

  if (elemType->get_node_type() == nodeType::BasicType) {
    elemType = std::make_shared<ast::BuiltInType>(
        std::dynamic_pointer_cast<BasicType>(elemType)->get_type());
  } else if (elemType->get_node_type() == nodeType::QualifiedName) {
    elemType = std::make_shared<ast::ReferenceType>(
        visitQualifiedIdentifier(elemType));
  } else {
    throw std::runtime_error(
        "Expected a BasicType or QualifiedIdentifier node but got " +
        elemType->type_string());
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

  auto nameNode = declNode->child(0);
  auto name = visitIdentifier(nameNode);

  std::shared_ptr<ast::Expr> init = nullptr;
  if (declNode->num_children() == 2) {
    init = visitExpr(declNode->child(1));
  }
  return VariableDecl{type, name, init};
}

} // namespace parsetree
