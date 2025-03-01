#pragma once

#include "ast/ast.hpp"
#include "parseTree/parseTree.hpp"
#include "staticCheck/envManager.hpp"
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace parsetree {

using nodeType = Node::Type;
using NodePtr = std::shared_ptr<Node>;

class ParseTreeVisitor {

public:
  ParseTreeVisitor(static_check::EnvManager &envManager)
      : envManager{envManager} {}

private:
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
    if (!node || node->num_children() < min || node->num_children() > max) {
      throw std::runtime_error(
          "Node has incorrect number of children!"
          " Type: " +
          (node ? node->type_string() : "null") +
          " Expected: " + std::to_string(min) + " to " + std::to_string(max) +
          " Actual: " + std::to_string(node ? node->num_children() : 0));
    }
  }

  [[noreturn]] static void unreachable() {
    throw std::runtime_error("Unreachable code reached!");
  }

  static ast::BasicType::Type getAstBasicType(BasicType::Type type);

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

    if (node->num_children() == 1) {
      list.push_back(visit<N, T>(node->child_at(0)));
    } else {
      visitListPattern<N, T, nullable>(node->child_at(0), list);
      list.push_back(visit<N, T>(node->child_at(1)));
    }
  }

public:
  // Program Decl visitors

  [[nodiscard]] std::shared_ptr<ast::ProgramDecl>
  visitProgramDecl(const NodePtr &node);
  [[nodiscard]] std::shared_ptr<ast::ReferenceType>
  visitPackageDecl(const NodePtr &node);
  // template <>
  // [[nodiscard]] ast::ImportDecl
  // visit<nodeType::ImportDeclList>(const NodePtr &node);

  // Classes and Interfaces visitors

  [[nodiscard]] std::shared_ptr<ast::ClassDecl>
  visitClassDecl(const NodePtr &node);
  [[nodiscard]] std::shared_ptr<ast::ReferenceType>
  visitSuper(const NodePtr &node);
  [[nodiscard]] std::shared_ptr<ast::InterfaceDecl>
  visitInterfaceDecl(const NodePtr &node);
  [[nodiscard]] std::shared_ptr<ast::FieldDecl>
  visitFieldDecl(const NodePtr &node);
  [[nodiscard]] std::shared_ptr<ast::MethodDecl>
  visitMethodDecl(const NodePtr &node);
  [[nodiscard]] std::shared_ptr<ast::MethodDecl>
  visitConstructorDecl(const NodePtr &node);
  [[nodiscard]] std::shared_ptr<ast::MethodDecl>
  visitAbstractMethodDecl(const NodePtr &node);

  // Statement visitors
  struct VariableDecl {
    std::shared_ptr<ast::Type> type;
    std::string name;
    std::shared_ptr<ast::Expr> init;
  };
  [[nodiscard]] VariableDecl visitLocalDecl(const NodePtr &type,
                                            const NodePtr &node);
  [[nodiscard]] std::shared_ptr<ast::DeclStmt>
  visitLocalDeclStatement(const NodePtr &node);

  [[nodiscard]] std::shared_ptr<ast::Block> visitBlock(const NodePtr &node);

  [[nodiscard]] std::shared_ptr<ast::Stmt> visitStatement(const NodePtr &node);

  [[nodiscard]] std::shared_ptr<ast::ReturnStmt>
  visitReturnStatement(const NodePtr &node);

  [[nodiscard]] std::shared_ptr<ast::IfStmt>
  visitIfStatement(const NodePtr &node);

  [[nodiscard]] std::shared_ptr<ast::WhileStmt>
  visitWhileStatement(const NodePtr &node);

  [[nodiscard]] std::shared_ptr<ast::ForStmt>
  visitForStatement(const NodePtr &node);

  [[nodiscard]] std::shared_ptr<ast::ExpressionStmt>
  visitExprStatement(const NodePtr &node);

  void visitStatementList(const NodePtr &node,
                          std::vector<std::shared_ptr<ast::Stmt>> &statements);

  // Expression visitors
  // [[nodiscard]] std::list<ast::ExprOp> visitExprOp(const NodePtr &node);

  [[nodiscard]] std::vector<std::shared_ptr<ast::ExprNode>>
  visitExprNode(const NodePtr &node);

  [[nodiscard]] std::vector<std::shared_ptr<ast::ExprNode>>
  visitCast(const NodePtr &node);

  [[nodiscard]] std::vector<std::shared_ptr<ast::ExprNode>>
  visitLiteral(const NodePtr &node);

  [[nodiscard]] std::vector<std::shared_ptr<ast::ExprNode>>
  visitRegularType(const NodePtr &node);

  [[nodiscard]] std::shared_ptr<ast::ExprNode>
  visitArrayType(const NodePtr &node);

  [[nodiscard]] std::shared_ptr<ast::ExprNode>
  visitArrayTypeInExpr(const NodePtr &node);

  [[nodiscard]] std::shared_ptr<ast::QualifiedName>
  visitQualifiedIdentifierInExpr(const NodePtr &node);

  [[nodiscard]] std::vector<std::string>
  visitUnresolvedTypeExpr(const NodePtr &node);

  std::shared_ptr<ast::ExprNode> visitBasicType(const NodePtr &node);

  [[nodiscard]] std::shared_ptr<ast::Expr> visitExpression(const NodePtr &node);

  [[nodiscard]] std::vector<std::shared_ptr<ast::ExprNode>>
  visitFieldAccess(const NodePtr &node);

  [[nodiscard]] std::vector<std::shared_ptr<ast::ExprNode>>
  visitArrayAccess(const NodePtr &node);

  [[nodiscard]] std::vector<std::shared_ptr<ast::ExprNode>>
  visitMethodInvocation(const NodePtr &node);

  [[nodiscard]] std::vector<std::shared_ptr<ast::ExprNode>>
  visitAssignment(const NodePtr &node);

  [[nodiscard]] std::vector<std::shared_ptr<ast::ExprNode>>
  visitArrayCreation(const NodePtr &node);

  [[nodiscard]] std::vector<std::shared_ptr<ast::ExprNode>>
  visitClassCreation(const NodePtr &node);

  void visitArgumentList(const NodePtr &node,
                         std::vector<std::shared_ptr<ast::ExprNode>> &args);

  // Leaf node visitors

  [[nodiscard]] std::shared_ptr<ast::UnresolvedType>
  visitReferenceType(const NodePtr &node,
                     std::shared_ptr<ast::UnresolvedType> ast_node = nullptr);
  [[nodiscard]] std::string visitIdentifier(const NodePtr &node);
  [[nodiscard]] ast::Modifiers
  visitModifierList(const NodePtr &node,
                    ast::Modifiers modifiers = ast::Modifiers{});
  [[nodiscard]] Modifier visitModifier(const NodePtr &node);
  [[nodiscard]] std::shared_ptr<ast::Type> visitType(const NodePtr &node);

private:
  static_check::EnvManager envManager;

  // Helpers

  [[nodiscard]] ast::UnOp::OpType
  getUnOpType(const std::shared_ptr<Operator> &node);

  [[nodiscard]] ast::BinOp::OpType
  getBinOpType(const std::shared_ptr<Operator> &node);

  void insertSeparator(std::vector<std::shared_ptr<ast::ExprNode>> &exprNodes);
};
} // namespace parsetree
