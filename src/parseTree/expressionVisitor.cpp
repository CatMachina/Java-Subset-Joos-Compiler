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
    {PtOp::InstanceOf, AstBinOp::InstanceOf},
    {PtOp::And, AstBinOp::And},
    {PtOp::Or, AstBinOp::Or}};

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
  // TODO: Code looks repetitive. Will fix later
  switch (node->get_node_type()) {
  case NodeType::Expression:
    return std::make_shared<ast::Expr>(visitExprNode(node));
  case NodeType::Assignment:
    return std::make_shared<ast::Expr>(visitAssignment(node));
  case NodeType::MethodInvocation:
    return std::make_shared<ast::Expr>(visitMethodInvocation(node));
  case NodeType::ArrayAccess:
    return std::make_shared<ast::Expr>(visitArrayAccess(node));
  case NodeType::FieldAccess:
    return std::make_shared<ast::Expr>(visitFieldAccess(node));
  case NodeType::Cast:
    return std::make_shared<ast::Expr>(visitCast(node));
  case NodeType::ArrayCreation:
    return std::make_shared<ast::Expr>(visitArrayCreation(node));
  case NodeType::ClassCreation:
    return std::make_shared<ast::Expr>(visitClassCreation(node));
  case NodeType::Literal:
    return std::make_shared<ast::Expr>(
        std::vector<std::shared_ptr<ast::ExprNode>>{visitLiteral(node)});
  case NodeType::Type:
    return std::make_shared<ast::Expr>(
        std::vector<std::shared_ptr<ast::ExprNode>>{visitBasicType(node)});
  case NodeType::ArrayType:
    return std::make_shared<ast::Expr>(
        std::vector<std::shared_ptr<ast::ExprNode>>{visitArrayType(node)});
  case NodeType::ArrayCastType:
    return std::make_shared<ast::Expr>(
        std::vector<std::shared_ptr<ast::ExprNode>>{visitArrayType(node)});
  case NodeType::Identifier: {
    std::vector<std::shared_ptr<ast::ExprNode>> exprNodes;
    auto name = visitIdentifier(node);
    if (name == "this") {
      exprNodes.push_back(std::make_shared<ast::ThisNode>());
    } else {
      exprNodes.push_back(std::make_shared<ast::MemberName>(name));
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
    return visitExpression(node->child_at(0))->getExprNodes();
  case 2: { // Unary expression
    auto right = visitExpression(node->child_at(1))->getExprNodes();

    auto op = std::dynamic_pointer_cast<Operator>(node->child_at(0));
    if (!op) {
      throw std::runtime_error(
          "Expected an operator node for unary expression");
    }

    right.push_back(std::make_shared<ast::UnOp>(getUnOpType(op)));
    return right;
  }
  case 3: { // Binary expression
    auto left = visitExpression(node->child_at(0))->getExprNodes();
    auto right = visitExpression(node->child_at(2))->getExprNodes();

    auto op = std::dynamic_pointer_cast<Operator>(node->child_at(1));
    if (!op) {
      throw std::logic_error("Expected an operator node for binary expression");
    }

    left.insert(left.end(), std::make_move_iterator(right.begin()),
                std::make_move_iterator(right.end()));
    left.push_back(std::make_shared<ast::BinOp>(getBinOpType(op)));
    return left;
  }
  default:
    throw std::logic_error("Unexpected number of children in expression node");
  }
}

std::vector<std::shared_ptr<ast::ExprNode>>
ParseTreeVisitor::visitAssignment(const NodePtr &node) {
  check_node_type(node, NodeType::Assignment);
  check_num_children(node, 3, 3);
  std::vector<std::shared_ptr<ast::ExprNode>> ops;

  auto lvalue = visitExpression(node->child_at(0))->getExprNodes();
  ops.insert(ops.end(), std::make_move_iterator(lvalue.begin()),
    std::make_move_iterator(lvalue.end()));

  auto exprNodes = visitExpression(node->child_at(2))->getExprNodes();
  ops.insert(ops.end(), std::make_move_iterator(exprNodes.begin()),
    std::make_move_iterator(exprNodes.end()));
  
  ops.push_back(std::make_shared<ast::Assignment>());

  return ops;
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

    ops.push_back(
        std::make_shared<ast::MethodInvocation>(args.size() + 1, qualifiedId));
    return ops;
  }
  if (node->num_children() == 3) {
    auto exprNodes = visitExpression(node->child_at(0))->getExprNodes();
    ops.insert(ops.end(), std::make_move_iterator(exprNodes.begin()),
               std::make_move_iterator(exprNodes.end()));

    auto id =
        std::make_shared<ast::MemberName>(visitIdentifier(node->child_at(1)));
    ops.push_back(id);
    ops.push_back(std::make_shared<ast::FieldAccess>());

    std::vector<std::shared_ptr<ast::ExprNode>> args;
    visitArgumentList(node->child_at(2), args);
    ops.insert(ops.end(), std::make_move_iterator(args.begin()),
               std::make_move_iterator(args.end()));

    auto qid = std::vector<std::shared_ptr<ast::ExprNode>>{id};
    ops.push_back(
        std::make_shared<ast::MethodInvocation>(args.size() + 1, qid));
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
  auto left = visitExpression(node->child_at(0))->getExprNodes();
  auto right = visitExpression(node->child_at(1))->getExprNodes();
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
  auto left = visitExpression(node->child_at(0))->getExprNodes();
  ops.insert(ops.end(), std::make_move_iterator(left.begin()),
             std::make_move_iterator(left.end()));
  ops.push_back(
      std::make_shared<ast::MemberName>(visitIdentifier(node->child_at(1))));
  ops.push_back(std::make_shared<ast::FieldAccess>());
  return ops;
}

std::vector<std::shared_ptr<ast::ExprNode>>
ParseTreeVisitor::visitCast(const NodePtr &node) {
  check_node_type(node, NodeType::Cast);
  check_num_children(node, 2, 2);
  std::vector<std::shared_ptr<ast::ExprNode>> ops;

  auto child = node->child_at(0);
  if (child->get_node_type() == NodeType::Type) {
    ops.push_back(visitBasicType(child));
  } else if (child->get_node_type() == NodeType::QualifiedName) {
    auto type = visitReferenceType(child);
    ops.push_back(std::make_shared<ast::TypeNode>(type));
  } else if (child->get_node_type() == NodeType::ArrayType ||
             child->get_node_type() == NodeType::ArrayCastType) {
    ops.push_back(visitArrayType(child));
  } else {
    throw std::runtime_error("Invalid Cast Expression");
  }
  auto exprNodes = visitExpression(node->child_at(1))->getExprNodes();
  ops.insert(ops.end(), std::make_move_iterator(exprNodes.begin()),
             std::make_move_iterator(exprNodes.end()));
  return ops;
}

std::vector<std::shared_ptr<ast::ExprNode>>
ParseTreeVisitor::visitArrayCreation(const NodePtr &node) {
  check_node_type(node, NodeType::ArrayCreation);
  check_num_children(node, 2, 2);
  std::vector<std::shared_ptr<ast::ExprNode>> ops;
  ops.push_back(visitArrayTypeInExpr(node->child_at(0)->child_at(0)));
  auto exprNodes = visitExpression(node->child_at(1))->getExprNodes();
  ops.insert(ops.end(), std::make_move_iterator(exprNodes.begin()),
             std::make_move_iterator(exprNodes.end()));
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
  ops.push_back(std::make_shared<ast::ClassCreation>(args.size() + 1));
  return ops;
}

void ParseTreeVisitor::visitArgumentList(
    const NodePtr &node, std::vector<std::shared_ptr<ast::ExprNode>> &ops) {
  if (node == nullptr)
    return;
  check_node_type(node, NodeType::ArgumentList);
  check_num_children(node, 1, 2);
  if (node->num_children() == 1) {
    auto exprNodes = visitExpression(node->child_at(0))->getExprNodes();
    ops.insert(ops.end(), std::make_move_iterator(exprNodes.begin()),
               std::make_move_iterator(exprNodes.end()));
  } else if (node->num_children() == 2) {
    visitArgumentList(node->child_at(0), ops);
    auto exprNodes = visitExpression(node->child_at(1))->getExprNodes();
    ops.insert(ops.end(), std::make_move_iterator(exprNodes.begin()),
               std::make_move_iterator(exprNodes.end()));
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
    ops.push_back(std::make_shared<ast::FieldAccess>());
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
    break;
  case Literal::Type::Character:
    type = ast::Literal::Type::Character;
    break;
  case Literal::Type::String:
    type = ast::Literal::Type::String;
    break;
  case Literal::Type::Boolean:
    type = ast::Literal::Type::Boolean;
    break;
  case Literal::Type::Null:
    type = ast::Literal::Type::Null;
    break;
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
    return std::make_shared<ast::TypeNode>(
        visitReferenceType(node->child_at(0)));
  }
  throw std::runtime_error("Expected a BasicType or QualifiedName node");
}

std::shared_ptr<ast::ExprNode>
ParseTreeVisitor::visitArrayTypeInExpr(const NodePtr &node) {
  if (auto basicType = std::dynamic_pointer_cast<BasicType>(node)) {
    return std::make_shared<ast::ArrayType>(
        std::make_shared<ast::BasicType>(basicType->getType()));
  } else if (node->get_node_type() == NodeType::QualifiedName) {
    return std::make_shared<ast::ArrayType>(visitReferenceType(node));
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

} // namespace parsetree