#pragma once

#include "ast/ast.hpp"
#include "parseTree/parseTree.hpp"
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace parsetree {

  using nodeType = Node::Type;
  using NodePtr = std::shared_ptr<Node>;

  // Basic helper functions

  static void check_node_type(const NodePtr &node, Node::Type type) {
    if (!node || node->get_node_type() != type) {
      throw std::runtime_error("Called on a node that is not the correct type!"
                               " Expected: " +
                               std::string(magic_enum::enum_name(type)) +
                               " Actual: " + node->type_string());
    }
  }

  static void check_num_children(const NodePtr &node, std::size_t min,
                                 std::size_t max) {
    if (!node || node->get_num_children() < min ||
        node->get_num_children() > max) {
      throw std::runtime_error(
          "Node has incorrect number of children!"
          " Type: " +
          (node ? node->type_string() : "null") +
          " Expected: " + std::to_string(min) + " to " + std::to_string(max) +
          " Actual: " + std::to_string(node ? node->get_num_children() : 0));
    }
  }

  [[noreturn]] static void unreachable() {
    throw std::runtime_error("Unreachable code reached!");
  }

  // Templated visitor patterns

  template <nodeType N, typename T> [[nodiscard]] T visit(const NodePtr &node) {
    throw std::runtime_error("No visitor for node type " +
                             (node ? node->type_string() : "null"));
  }

  template <nodeType N, typename T, bool nullable = false>
  void visitListPattern(const NodePtr &node, std::vector<T> &list) {
    if constexpr (nullable) {
      if (!node)
        return;
    } else if (!node) {
      throw std::runtime_error("Visited a null node!");
    }

    check_node_type(node, N);
    check_num_children(node, 1, 2);

    if (node->get_num_children() == 1) {
      list.push_back(visit<N, T>(node->child_at(0)));
    } else {
      visitListPattern<N, T, nullable>(node->child_at(0), list);
      list.push_back(visit<N, T>(node->child_at(1)));
    }
  }

  // Program Decl visitors

  [[nodiscard]] std::shared_ptr<ast::ProgramDecl>
  visitProgramDecl(const NodePtr &node);
  [[nodiscard]] std::shared_ptr<ast::QualifiedIdentifier>
  visitPackageDecl(const NodePtr &node);
  template <>
  [[nodiscard]] ast::ImportDecl
  visit<nodeType::ImportDeclList>(const NodePtr &node);

  // Classes & interfaces visitors

  [[nodiscard]] std::shared_ptr<ast::ClassDecl>
  visitClassDecl(const NodePtr &node);
  [[nodiscard]] std::shared_ptr<ast::InterfaceDecl>
  visitInterfaceDecl(const NodePtr &node);
  [[nodiscard]] std::shared_ptr<ast::QualifiedIdentifier>
  visitSuper(const NodePtr &node);
  [[nodiscard]] std::shared_ptr<ast::FieldDecl>
  visitFieldDecl(const NodePtr &node);
  [[nodiscard]] std::shared_ptr<ast::MethodDecl>
  visitMethodDecl(const NodePtr &node);
  [[nodiscard]] std::shared_ptr<ast::MethodDecl>
  visitConstructorDecl(const NodePtr &node);
  [[nodiscard]] std::shared_ptr<ast::MethodDecl>
  visitAbstractMethodDecl(const NodePtr &node);

  template <>
  [[nodiscard]] std::shared_ptr<ast::Decl>
  visit<nodeType::ClassBodyDeclList>(const NodePtr &node);
  template <>
  [[nodiscard]] std::shared_ptr<ast::VarDecl>
  visit<nodeType::ParameterList>(const NodePtr &node);
  template <>
  [[nodiscard]] std::shared_ptr<ast::Decl>
  visit<nodeType::InterfaceBodyDeclList>(const NodePtr &node);

  // Statements visitors
  struct VariableDecl {
    std::shared_ptr<ast::Type> type;
    std::string_view name;
    std::list<ast::ExprOp> init;
  };
  VariableDecl visitVariableDeclarator(const NodePtr &type,
                                       const NodePtr &node);

  [[nodiscard]] std::shared_ptr<ast::Stmt> visitBlock(const NodePtr &node);

  // Expression visitors

  [[nodiscard]] std::list<ast::ExprOp> visitExpr(const NodePtr &node);

  // Leaf node visitors

  [[nodiscard]] std::shared_ptr<ast::QualifiedIdentifier>
  visitQualifiedIdentifier(
      const NodePtr &node,
      std::shared_ptr<ast::QualifiedIdentifier> ast_node = nullptr);
  [[nodiscard]] std::string visitIdentifier(const NodePtr &node);
  [[nodiscard]] ast::Modifiers
  visitModifierList(const NodePtr &node,
                    ast::Modifiers modifiers = ast::Modifiers{});
  [[nodiscard]] Modifier visitModifier(const NodePtr &node);
  [[nodiscard]] std::shared_ptr<ast::Type> visitType(const NodePtr &node);


} // namespace parsetree
