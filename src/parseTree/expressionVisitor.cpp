#include "parseTree/parseTree.hpp"
#include "parseTree/parseTreeVisitor.hpp"

namespace parsetree {

using NodeType = Node::Type;
using NodePtr = std::shared_ptr<Node>;

// Helpers
ast::UnOp::OpType ParseTreeVisitor::getUnOpType(const NodePtr &node) {
  check_node_type(node, NodeType::Operator);
  // TODO
  return ast::UnOp::OpType::Not;
}

ast::BinOp::OpType ParseTreeVisitor::getBinOpType(const NodePtr &node) {
  check_node_type(node, NodeType::Operator);
  // TODO
  return ast::BinOp::OpType::GreaterThan;
}

// All Expression visitors
std::shared_ptr<ast::Expr>
ParseTreeVisitor::visitExpression(const NodePtr &node) {
  using namespace NodeType;

  switch (node->get_node_type()) {
  case Expression:
    return std::make_shared<ast::Expr>(visitExprNode(node));
  case MethodInvocation:
    return std::make_shared<ast::Expr>(visitMethodInvocation(node));
  case ArrayAccess:
    return std::make_shared<ast::Expr>(visitArrayAccess(node));
  case FieldAccess:
    return std::make_shared<ast::Expr>(visitFieldAccess(node));
  case CastExpression:
    return std::make_shared<ast::Expr>(visitCastExpression(node));
  case ArrayCreate:
    return std::make_shared<ast::Expr>(visitArrayCreation(node));
  case ClassCreation:
    return std::make_shared<ast::Expr>(visitClassCreation(node));
  case Literal:
    return std::make_shared<ast::Expr>(visitLiteral(node));
  case Type:
    return std::make_shared<ast::Expr>(visitRegularType(node));
  case ArrayType:
    return std::make_shared<ast::Expr>(visitArrayType(node));
  case ArrayCastType:
    return std::make_shared<ast::Expr>(visitArrayType(node));
  case Identifier: {
    auto name = visitIdentifier(node);
    if (name == "this") {
      return std::make_shared<ast::ThisNode>();
    }
    return std::make_shared<ast::MemberName>(name);
  }
  case QualifiedIdentifier:
    return std::make_shared<ast::Expr>(visitQualifiedIdentifierInExpr(node));
  default:
    throw std::runtime_error("Invalid Expression");
  }
}

std::vector<std::shared_ptr<ast::ExprNode>>
ParseTreeVisitor::visitExprNode(const NodePtr &node) {
  check_node_type(node, NodeType::Expression);
  check_num_children(node, 1, 3);

  switch (node->num_children()) {
  case 1:
    return visitExpression(node->child_at(0));
  case 2: { // Unary expression
    auto right = visitExpression(node->child_at(1));

    auto op = std::dynamic_pointer_cast<Operator>(node->child_at(0));
    if (!op) {
      throw std::runtime_error(
          "Expected an operator node for unary expression");
    }

    right.push_back(getUnOpType(op));
    return right;
  }
  case 3: { // Binary expression
    auto left = visitExpression(node->child_at(0));
    auto right = visitExpression(node->child_at(2));

    auto op = std::dynamic_pointer_cast<Operator>(node->child_at(1));
    if (!op) {
      throw std::logic_error("Expected an operator node for binary expression");
    }

    left.insert(left.end(), std::make_move_iterator(right.begin()),
                std::make_move_iterator(right.end()));
    left.push_back(getBinOpType(op));
    return left;
  }
  default:
    throw std::logic_error("Unexpected number of children in expression node");
  }
}

std::vector<std::shared_ptr<ast::ExprNode>>
ParseTreeVisitor::visitMethodInvocation(const NodePtr &node) {
  check_node_type(node, NodeType::MethodInvocation);
  check_num_children(node, 2, 3);
  std::vector<std::shared_ptr<ast::ExprNode>> ops;

  if (node->num_children() == 2) {
    auto qualifiedId = visitQualifiedIdentifierInExpr(node->child_at(0));
    ops.insert(ops.end(), std::make_move_iterator(qualifiedId.begin()),
               std::make_move_iterator(qualifiedId.end()));

    std::vector<std::unique_ptr<ast::ExprNode>> args;
    visitArgumentList(node->child_at(1), args);
    ops.insert(ops.end(), std::make_move_iterator(args.begin()),
               std::make_move_iterator(args.end()));

    ops.push_back(std::make_shared<ast::MethodInvocation>(args.size() + 1));
    return ops;
  }
  if (node->num_children() == 3) {
    auto expr = visitExpression(node->child_at(0));
    ops.insert(ops.end(), std::make_move_iterator(expr.begin()),
               std::make_move_iterator(expr.end()));

    ops.push_back(
        std::make_shared<ast::MemberName>(visitIdentifier(node->child_at(1))));
    ops.push_back(std::make_shared<ast::MemberAccess>());

    std::vector<std::unique_ptr<ast::ExprNode>> args;
    visitArgumentList(node->child_at(2), args);
    ops.insert(ops.end(), std::make_move_iterator(args.begin()),
               std::make_move_iterator(args.end()));

    ops.push_back(std::make_shared<ast::MethodInvocation>(args.size() + 1));
    return ops;
  }

  throw std::runtime_error(
      "Unexpected number of children in method invocation node");
}

std::vector<std::shared_ptr<ast::ExprNode>>
ParseTreeVisitor::visitArrayAccess(const NodePtr &node) {
  check_node_type(node, NodeType::ArrayAccess);
  check_num_children(node, 2, 2);
  std::vector<std::shared_ptr<ast::ExprNode>> ops;
  auto left = visitExpression(node->child_at(0));
  auto right = visitExpression(node->child_at(1));
  ops.insert(ops.end(), std::make_move_iterator(left.begin()),
             std::make_move_iterator(left.end()));
  ops.insert(ops.end(), std::make_move_iterator(right.begin()),
             std::make_move_iterator(right.end()));
  ops.push_back(std::make_shared<ast::ArrayAccess>());
  return ops;
}

std::vector<std::shared_ptr<ast::ExprNode>>
ParseTreeVisitor::visitFieldAccess(const NodePtr &node) {
  check_node_type(node, NodeType::FieldAccess);
  check_num_children(node, 2, 2);
  std::vector<std::shared_ptr<ast::ExprNode>> ops;
  auto left = visitExpression(node->child_at(0));
  ops.insert(ops.end(), std::make_move_iterator(left.begin()),
             std::make_move_iterator(left.end()));
  ops.push_back(
      std::make_shared<ast::MemberName>(visitIdentifier(node->child_at(1))));
  ops.push_back(std::make_shared<ast::MemberAccess>());
  return ops;
}

std::vector<std::shared_ptr<ast::ExprNode>>
ParseTreeVisitor::visitCastExpression(const NodePtr &node) {
  check_node_type(node, NodeType::CastExpression);
  check_num_children(node, 2, 3);
  std::vector<std::shared_ptr<ast::ExprNode>> ops;

  if (auto basicType =
          std::dynamic_pointer_cast<BasicType>(node->child_at(0))) {
    auto type = std::make_shared<ast::BuiltInType>(basicType->get_type());
    if (node->num_children() == 3 && node->child_at(1) != nullptr) {
      ops.push_back(std::make_shared<ast::ArrayType>(type));
    } else {
      ops.push_back(std::make_shared<ast::TypeNode>(type));
    }
  } else if (node->child_at(0)->get_node_type() ==
             NodeType::QualifiedIdentifier) {
    auto type = visitQualifiedIdentifier(node->child_at(0));
    ops.push_back(std::make_shared<ast::TypeNode>(type));
  } else {
    ops.push_back(visitArrayType(node->child_at(0)));
  }

  auto expr = visitExpression(node->child_at(node->num_children() - 1));
  ops.insert(ops.end(), std::make_move_iterator(expr.begin()),
             std::make_move_iterator(expr.end()));
  ops.push_back(std::make_shared<ast::Cast>());
  return ops;
}

std::vector<std::shared_ptr<ast::ExprNode>>
ParseTreeVisitor::visitArrayCreation(const NodePtr &node) {
  check_node_type(node, NodeType::ArrayCreate);
  check_num_children(node, 2, 2);
  std::vector<std::shared_ptr<ast::ExprNode>> ops;
  ops.push_back(visitArrayTypeInExpr(node->child_at(0)));
  auto expr = visitExpression(node->child_at(1));
  ops.insert(ops.end(), std::make_move_iterator(expr.begin()),
             std::make_move_iterator(expr.end()));
  ops.push_back(std::make_shared<ast::ArrayInstanceCreation>());
  return ops;
}

std::vector<std::shared_ptr<ast::ExprNode>>
ParseTreeVisitor::visitClassCreation(const NodePtr &node) {
  check_node_type(node, NodeType::ClassCreation);
  check_num_children(node, 2, 2);
  std::vector<std::shared_ptr<ast::ExprNode>> ops;
  ops.push_back(visitQualifiedIdentifierInExpr(node->child_at(0)));
  auto args = visitArgumentList(node->child_at(1));
  ops.insert(ops.end(), std::make_move_iterator(args.begin()),
             std::make_move_iterator(args.end()));
  ops.push_back(std::make_shared<ast::ClassInstanceCreation>(expr.size() + 1));
  return ops;
}

std::vector<std::shared_ptr<ast::ExprNode>>
ParseTreeVisitor::visitQualifiedIdentifierInExpr(const NodePtr &node) {
  check_node_type(node, NodeType::QualifiedName);
  check_num_children(node, 1, 2);
  std::vector<std::shared_ptr<ast::ExprNode>> ops;

  if (node->num_children() == 1) {
    ops.push_back(
        std::make_shared<ast::MemberName>(visitIdentifier(node->child_at(0))));
  } else if (node->num_children() == 2) {
    ops = visitQualifiedIdentifierInExpr(node->child_at(0));
    ops.push_back(
        std::make_shared<ast::MemberName>(visitIdentifier(node->child_at(1))));
    ops.push_back(std::make_shared<ast::MemberAccess>());
  }
  return ops;
}

// Leaf Nodes
std::shared_ptr<ast::ExprNode>
ParseTreeVisitor::visitLiteral(const NodePtr &node) {
  check_node_type(node, NodeType::Literal);
  auto lit = std::dynamic_pointer_cast<Literal>(node);
  if (!lit) {
    throw std::runtime_error("Expected literal node");
  }
  switch (lit->get_type()) {
  case Literal::Type::Integer:
    return std::make_shared<ast::LiteralNode>(lit->get_value(),
                                              ast::LiteralNode::Type::Integer);
  case Literal::Type::Character:
    return std::make_shared<ast::LiteralNode>(
        lit->get_value(), ast::LiteralNode::Type::Character);
  case Literal::Type::String:
    return std::make_shared<ast::LiteralNode>(lit->get_value(),
                                              ast::LiteralNode::Type::String);
  case Literal::Type::Boolean:
    return std::make_shared<ast::LiteralNode>(lit->get_value(),
                                              ast::LiteralNode::Type::Boolean);
  case Literal::Type::Null:
    return std::make_shared<ast::LiteralNode>(lit->get_value(),
                                              ast::LiteralNode::Type::Null);
  default:
    throw std::runtime_error("Invalid literal type");
  }
}

std::shared_ptr<ast::ExprNode>
ParseTreeVisitor::visitRegularType(const NodePtr &node) {
  check_node_type(node, NodeType::RegularType);
  check_num_children(node, 1, 1);
  if (auto basicType =
          std::dynamic_pointer_cast<BasicType>(node->child_at(0))) {
    return std::make_shared<ast::BuiltInType>(basicType->get_type());
  } else if (node->child_at(0)->get_node_type() == NodeType::QualifiedName) {
    return visitQualifiedIdentifier(node->child_at(0));
  }
}

std::shared_ptr<ast::ExprNode>
ParseTreeVisitor::visitArrayTypeInExpr(const NodePtr &node) {
  if (auto basicType = std::dynamic_pointer_cast<BasicType>(node)) {
    return std::make_shared<ast::ArrayType>(
        std::make_shared<ast::BuiltInType>(basicType->get_type()));
  } else if (node->get_node_type() == NodeType::QualifiedName) {
    return std::make_shared<ast::ArrayType>(visitQualifiedIdentifier(node));
  } else {
    throw std::runtime_error(
        "Expected a BasicType or QualifiedName node for ArrayTypeInExpr");
  }
}

std::shared_ptr<ast::ExprNode>
ParseTreeVisitor::visitArrayType(const NodePtr &node) {
  check_num_children(node, 1, 1);
  if (node->get_node_type() != NodeType::ArrayType &&
      node->get_node_type() != NodeType::ArrayCastType) {
    throw std::runtime_error("Expected an ArrayType or ArrayCastType node");
  }
  return visitArrayTypeInExpr(node->child_at(0));
}

// std::shared_ptr<ast::StatementExpr>
// ParseTreeVisitor::visitStatementExpr(const NodePtr &node) {
//   switch (node->get_node_type()) {
//   case NodeType::Assignment:
//     return visitAssignment(node);
//   case NodeType::MethodInvocation:
//     return visitMethodInvocation(node);
//   case NodeType::ClassCreation:
//     return visitClassCreation(node);
//   default:
//     throw std::runtime_error("Invalid StatementExpr");
//   }
// }

// std::shared_ptr<ast::Assignment>
// ParseTreeVisitor::visitAssignment(const NodePtr &node) {
//   check_node_type(node, NodeType::Assignment);
//   check_num_children(node, 2, 2);
//   // TODO: Expression
//   return std::make_shared<ast::Assignment>(visitLValue(node->child_at(0)),
//                                            visitExpression(node->child_at(1)));
// }

// std::shared_ptr<ast::LValue>
// ParseTreeVisitor::visitLValue(const NodePtr &node) {
//   switch (node->get_node_type()) {
//   case NodeType::QualifiedName:
//     return visitQualifiedIdentifier(node);
//   case NodeType::FieldAccess:
//     return visitFieldAccess(node);
//   case NodeType::ArrayAccess:
//     return visitArrayAccess(node);
//   default:
//     throw std::runtime_error("Not a lvalue!");
//   }
// }

// std::shared_ptr<ast::ArrayAccess>
// ParseTreeVisitor::visitArrayAccess(const NodePtr &node) {
//   check_node_type(node, NodeType::ArrayAccess);
//   check_num_children(node, 2, 2);
//   // TODO: Expression
//   if (node->child_at(0)->get_node_type() == NodeType::QualifiedName) {
//     return std::make_shared<ast::ArrayAccess>(
//         visitQualifiedIdentifier(node->child_at(0)),
//         visitExpression(node->child_at(1)));
//   } else {
//     return std::make_shared<ast::ArrayAccess>(
//         visitExpression(node->child_at(0)),
//         visitExpression(node->child_at(1)));
//   }
// }

// std::shared_ptr<ast::MethodInvocation>
// ParseTreeVisitor::visitMethodInvocation(const NodePtr &node) {
//   check_node_type(node, NodeType::MethodInvocation);
//   check_num_children(node, 2, 3);
//   std::shared_ptr<ast::QualifiedIdentifier> qualifiedIdentifier;
//   if (node->get_num_children() == 2) {
//     // TODO: args
//     return std::make_shared<ast::MethodInvocation>(
//         visitQualifiedIdentifier(node->child_at(0)),
//         std::vector<std::shared_ptr<ast::Expr>>());
//   } else {
//     return std::make_shared<ast::MethodInvocation>(
//         visitExpression(node->child_at(0)),
//         visitIdentifier(node->child_at(1)),
//         std::vector<std::shared_ptr<ast::Expr>>());
//   }
// }

} // namespace parsetree