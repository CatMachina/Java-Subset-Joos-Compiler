#pragma once

#include "ast/ast.hpp"
#include "parsetree/parseTree.hpp"

namespace parsetree {

using nodeType = Node::Type;

// General visitor template
template <parsetree::Node::Type N, typename T>
T visit(Node* node) {
    if (!node) {
        throw std::runtime_error("Visited a null node!");
    }
    throw std::runtime_error("No visitor for node type " + node->type_string());
}

// List pattern visitor template
template <parsetree::Node::Type N, typename T, bool nullable = false>
void visitListPattern(Node* node, std::vector<T>& list) {
    if (nullable && node == nullptr) return;
    if (!nullable && node == nullptr) {
        throw std::runtime_error("Visited a null node!");
    }
    check_node_type(node, N);
    check_num_children(node, 1, 2);

    if (node->num_children() == 1) {
        list.push_back(visit<N, T>(node->child(0)));
    } else if (node->num_children() == 2) {
        visitListPattern<N, T, nullable>(node->child(0), list);
        list.push_back(visit<N, T>(node->child(1)));
    }
}

// Compilation unit visitors ///////////////////////////////////////////////////

std::unique_ptr<ast::ProgramDecl> visitProgramDecl(Node* node);
std::unique_ptr<ast::QualifiedIdentifier> visitPackageDeclaration(Node* node);

template <>
ast::ImportDeclaration visit<pty::ImportDeclarationList>(Node* node);

// Classes & interfaces visitors ///////////////////////////////////////////////

std::unique_ptr<ast::ClassDecl> visitClassDeclaration(Node* node);
std::unique_ptr<ast::InterfaceDecl> visitInterfaceDeclaration(Node* node);
std::unique_ptr<ast::QualifiedIdentifier> visitSuperOpt(Node* node);
std::unique_ptr<ast::FieldDecl> visitFieldDeclaration(Node* node);
std::unique_ptr<ast::MethodDecl> visitMethodDeclaration(Node* node);
std::unique_ptr<ast::MethodDecl> visitConstructorDeclaration(Node* node);
std::unique_ptr<ast::MethodDecl> visitAbstractMethodDeclaration(Node* node);

template <>
ast::Decl* visit<pty::ClassBodyDeclarationList>(Node* node);
template <>
ast::VarDecl* visit<pty::VariableDeclaratorList>(Node* node);
template <>
ast::VarDecl* visit<pty::FormalParameterList>(Node* node);
template <>
ast::Decl* visit<pty::InterfaceMemberDeclarationList>(Node* node);

// Statements visitors /////////////////////////////////////////////////////////

std::unique_ptr<ast::Stmt> visitBlock(Node* node);

// Leaf node visitors //////////////////////////////////////////////////////////

std::unique_ptr<ast::QualifiedIdentifier> visitQualifiedIdentifier(
    Node* node, std::unique_ptr<ast::QualifiedIdentifier> ast_node = nullptr);
std::string visitIdentifier(Node* node);
ast::Modifiers visitModifierList(Node* node, ast::Modifiers modifiers = ast::Modifiers{});
Modifier visitModifier(Node* node);
std::unique_ptr<ast::Type> visitType(Node* node);

} // namespace parsetree
