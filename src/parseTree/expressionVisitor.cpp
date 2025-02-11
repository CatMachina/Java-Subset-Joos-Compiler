#include "parseTree/parseTree.hpp"
#include "parseTree/parseTreeVisitor.hpp"

#include <unordered_map>

namespace parsetree {

using NodeType = Node::Type;
using NodePtr = std::shared_ptr<Node>;
using PtOp = Operator::Type;
using AstUnOp = ast::UnOp::OpType;
using AstBinOp = ast::BinOp::OpType;

// Lookup tables
static std::unordered_map<PtOp, AstUnOp> UnOpTable = {
    {PtOp::Not, AstUnOp::Not},
    {PtOp::Plus, AstUnOp::Plus},
    {PtOp::Minus, AstUnOp::Minus}};

static std::unordered_map<PtOp, AstBinOp> BinOpTable = {
    {PtOp::Assign, AstBinOp::Assign},
    {PtOp::GreaterThan, AstBinOp::GreaterThan},
    {PtOp::GreaterThanOrEqual, AstBinOp::GreaterThanOrEqual},
    {PtOp::LessThan, AstBinOp::LessThan},
    {PtOp::LessThanOrEqual, AstBinOp::LessThanOrEqual},
    {PtOp::Equal, AstBinOp::Equal},
    {PtOp::NotEqual, AstBinOp::NotEqual},
    {PtOp::Add, AstBinOp::Add},
    {PtOp::Subtract, AstBinOp::Subtract},
    {PtOp::Multiply, AstBinOp::Multiply},
    {PtOp::Divide, AstBinOp::Divide},
    {PtOp::Modulo, AstBinOp::Modulo},
    {PtOp::BitwiseAnd, AstBinOp::BitWiseAnd},
    {PtOp::BitwiseOr, AstBinOp::BitWiseOr},
    {PtOp::Plus, AstBinOp::Plus},
    {PtOp::Minus, AstBinOp::Minus},
    {PtOp::InstanceOf, AstBinOp::InstanceOf}};

// Helpers
AstUnOp ParseTreeVisitor::getUnOpType(const std::shared_ptr<Operator> &node) {
  PtOp type = node->getType();
  auto result = UnOpTable.find(type);
  if (result == UnOpTable.end()) {
    throw std::runtime_error("Unary Op not found");
  }
  return result->second;
}

AstBinOp ParseTreeVisitor::getBinOpType(const std::shared_ptr<Operator> &node) {
  PtOp type = node->getType();
  auto result = BinOpTable.find(type);
  if (result == BinOpTable.end()) {
    throw std::runtime_error("Binary Op not found");
  }
  return result->second;
}

// All Expression visitors
std::shared_ptr<ast::Expr>
ParseTreeVisitor::visitExpression(const NodePtr &node) {
  // TODO: I don't think this works?
  // using namespace NodeType;

  switch (node->get_node_type()) {
  case NodeType::Expression:
    return std::make_shared<ast::Expr>(visitExprNode(node));
  case NodeType::MethodInvocation:
    return std::make_shared<ast::Expr>(visitMethodInvocation(node));
  case NodeType::ArrayAccess:
    return std::make_shared<ast::Expr>(visitArrayAccess(node));
  case NodeType::FieldAccess:
    return std::make_shared<ast::Expr>(visitFieldAccess(node));
  case NodeType::Casting:
    return std::make_shared<ast::Expr>(visitCasting(node));
  case NodeType::ArrayCreation:
    return std::make_shared<ast::Expr>(visitArrayCreation(node));
  case NodeType::ClassCreation:
    return std::make_shared<ast::Expr>(visitClassCreation(node));
  case NodeType::Literal:
    return std::make_shared<ast::Expr>(visitLiteral(node));
  case NodeType::Type:
    return std::make_shared<ast::Expr>(visitBasicType(node));
  case NodeType::ArrayType:
    return std::make_shared<ast::Expr>(visitArrayType(node));
  case NodeType::ArrayCastType:
    return std::make_shared<ast::Expr>(visitArrayType(node));
  case NodeType::Identifier: {
    std::vector<std::shared_ptr<ast::ExprNode>> exprNodes;
    auto name = visitIdentifier(node);
    if (name == "this") {
      exprNodes.push_back(std::make_shared<ast::ThisNode>());
    } else {
      exprNodes.push_back(std::make_shared<ast::MemberName>());
    }
    return std::make_shared<ast::Expr>(exprNodes);
  }
  case NodeType::QualifiedName:
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

    std::vector<std::shared_ptr<ast::ExprNode>> args;
    visitArgumentList(node->child_at(1), args);
    ops.insert(ops.end(), std::make_move_iterator(args.begin()),
               std::make_move_iterator(args.end()));

    ops.push_back(std::make_shared<ast::MethodInvocation>(args.size() + 1));
    return ops;
  }
  if (node->num_children() == 3) {
    auto expr = visitExpression(node->child_at(0));
    ops.insert(ops.end(), std::make_move_iterator(expr->getExprNodes().begin()),
               std::make_move_iterator(expr->getExprNodes().end()));

    ops.push_back(
        std::make_shared<ast::MemberName>(visitIdentifier(node->child_at(1))));
    // TODO: MemberAccess?
    ops.push_back(std::make_shared<ast::MemberAccess>());

    std::vector<std::shared_ptr<ast::ExprNode>> args;
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
  ops.insert(ops.end(), std::make_move_iterator(left->getExprNodes().begin()),
             std::make_move_iterator(left->getExprNodes().end()));
  ops.insert(ops.end(), std::make_move_iterator(right->getExprNodes().begin()),
             std::make_move_iterator(right->getExprNodes().end()));
  ops.push_back(std::make_shared<ast::ArrayAccess>());
  return ops;
}

std::vector<std::shared_ptr<ast::ExprNode>>
ParseTreeVisitor::visitFieldAccess(const NodePtr &node) {
  check_node_type(node, NodeType::FieldAccess);
  check_num_children(node, 2, 2);
  std::vector<std::shared_ptr<ast::ExprNode>> ops;
  auto left = visitExpression(node->child_at(0));
  ops.insert(ops.end(), std::make_move_iterator(left->getExprNodes().begin()),
             std::make_move_iterator(left->getExprNodes().end()));
  ops.push_back(
      std::make_shared<ast::MemberName>(visitIdentifier(node->child_at(1))));
  ops.push_back(std::make_shared<ast::FieldAccess>());
  return ops;
}

std::vector<std::shared_ptr<ast::ExprNode>>
ParseTreeVisitor::visitCasting(const NodePtr &node) {
  check_node_type(node, NodeType::Casting);
  check_num_children(node, 2, 3);
  std::vector<std::shared_ptr<ast::ExprNode>> ops;

  if (auto basicType =
          std::dynamic_pointer_cast<BasicType>(node->child_at(0))) {
    auto type = std::make_shared<ast::BasicType>(basicType->getType());
    if (node->num_children() == 3 && node->child_at(1) != nullptr) {
      ops.push_back(std::make_shared<ast::ArrayType>(type));
    } else {
      ops.push_back(std::make_shared<ast::TypeNode>(type));
    }
  } else if (node->child_at(0)->get_node_type() == NodeType::QualifiedName) {
    auto type = visitQualifiedIdentifier(node->child_at(0));
    ops.push_back(std::make_shared<ast::TypeNode>(type));
  } else {
    ops.push_back(visitArrayType(node->child_at(0)));
  }

  auto expr = visitExpression(node->child_at(node->num_children() - 1));
  ops.insert(ops.end(), std::make_move_iterator(expr->getExprNodes().begin()),
             std::make_move_iterator(expr->getExprNodes().end()));
  ops.push_back(std::make_shared<ast::Cast>());
  return ops;
}

std::vector<std::shared_ptr<ast::ExprNode>>
ParseTreeVisitor::visitArrayCreation(const NodePtr &node) {
  check_node_type(node, NodeType::ArrayCreation);
  check_num_children(node, 2, 2);
  std::vector<std::shared_ptr<ast::ExprNode>> ops;
  ops.push_back(visitArrayTypeInExpr(node->child_at(0)));
  auto expr = visitExpression(node->child_at(1));
  ops.insert(ops.end(), std::make_move_iterator(expr->getExprNodes().begin()),
             std::make_move_iterator(expr->getExprNodes().end()));
  ops.push_back(std::make_shared<ast::ArrayCreation>());
  return ops;
}

std::vector<std::shared_ptr<ast::ExprNode>>
ParseTreeVisitor::visitClassCreation(const NodePtr &node) {
  check_node_type(node, NodeType::ClassCreation);
  check_num_children(node, 2, 2);
  std::vector<std::shared_ptr<ast::ExprNode>> ops;
  auto exprNodes = visitQualifiedIdentifierInExpr(node->child_at(0));
  ops.insert(ops.end(), std::make_move_iterator(exprNodes.begin()),
             std::make_move_iterator(exprNodes.end()));

  std::vector<std::shared_ptr<ast::ExprNode>> args;
  visitArgumentList(node->child_at(1), args);
  ops.insert(ops.end(), std::make_move_iterator(args.begin()),
             std::make_move_iterator(args.end()));
  ops.push_back(std::make_shared<ast::ClassCreation>());
  return ops;
}

void ParseTreeVisitor::visitArgumentList(
    const NodePtr &node, std::vector<std::shared_ptr<ast::ExprNode>> &ops) {
  if (node == nullptr)
    return;
  check_node_type(node, NodeType::ArgumentList);
  check_num_children(node, 1, 2);
  if (node->num_children() == 1) {
    auto expr = visitExpression(node->child_at(0));
    ops.insert(ops.end(), std::make_move_iterator(expr->getExprNodes().begin()),
               std::make_move_iterator(expr->getExprNodes().end()));
  } else if (node->num_children() == 2) {
    visitArgumentList(node->child_at(0), ops);
    auto expr = visitExpression(node->child_at(1));
    ops.insert(ops.end(), std::make_move_iterator(expr->getExprNodes().begin()),
               std::make_move_iterator(expr->getExprNodes().end()));
  }
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
    // TODO: MemberAccess?
    ops.push_back(std::make_shared<ast::MemberAccess>());
  }
  return ops;
}

// Leaf Nodes
std::vector<std::shared_ptr<ast::ExprNode>>
ParseTreeVisitor::visitLiteral(const NodePtr &node) {
  check_node_type(node, NodeType::Literal);
  auto lit = std::dynamic_pointer_cast<Literal>(node);
  if (!lit) {
    throw std::runtime_error("Expected literal node");
  }
  ast::Literal::Type type;
  switch (lit->getType()) {
  case Literal::Type::Integer:
    type = ast::Literal::Type::Integer;
  case Literal::Type::Character:
    type = ast::Literal::Type::Character;
  case Literal::Type::String:
    type = ast::Literal::Type::String;
  case Literal::Type::Boolean:
    type = ast::Literal::Type::Boolean;
  case Literal::Type::Null:
    type = ast::Literal::Type::Null;
  default:
    throw std::runtime_error("Invalid literal type");
  }
  std::vector<std::shared_ptr<ast::ExprNode>> exprNodes;
  exprNodes.push_back(std::make_shared<ast::Literal>(type, lit->getValue()));
  return exprNodes;
}

std::shared_ptr<ast::ExprNode>
ParseTreeVisitor::visitBasicType(const NodePtr &node) {
  check_node_type(node, NodeType::Type);
  check_num_children(node, 1, 1);
  if (auto basicType =
          std::dynamic_pointer_cast<BasicType>(node->child_at(0))) {
    return std::make_shared<ast::BasicType>(basicType->getType());
  } else if (node->child_at(0)->get_node_type() == NodeType::QualifiedName) {
    return visitQualifiedIdentifier(node->child_at(0));
  }
}

std::shared_ptr<ast::ExprNode>
ParseTreeVisitor::visitArrayTypeInExpr(const NodePtr &node) {
  if (auto basicType = std::dynamic_pointer_cast<BasicType>(node)) {
    return std::make_shared<ast::ArrayType>(
        std::make_shared<ast::BasicType>(basicType->getType()));
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

} // namespace parsetree