#pragma once

#include "ast/astNode.hpp"

namespace parsetree::ast {

class ExprNode {
public:
  virtual ~ExprNode() = default;
};

class Expr {
  // Reverse Polish Notation
  // TODO: We use vector for now
  std::vector<std::shared_ptr<ExprNode>> exprNodes;

public:
  Expr(std::vector<std::shared_ptr<ExprNode>> exprNodes)
      : exprNodes{exprNodes} {}

  // Getter
  const std::vector<std::shared_ptr<ExprNode>> &getExprNodes() const {
    return exprNodes;
  }
};

// Expressions /////////////////////////////////////////////////////////////

class Literal : public ExprNode {
public:
  enum class Type { Integer, Character, String, Boolean, Null };

  Literal(Type type, std::string value) : type{type}, value{value} {}

  // Getters
  Type getType() const { return type; }
  std::string getValue() const { return value; }

private:
  Type type;
  std::string value;
};

class MemberName : public ExprNode {
public:
  MemberName(std::string_view name) : name{name} {}

private:
  std::string name;
};

class ThisNode : public ExprNode {};

// class LValue : public Expr {};

// class StatementExpr : public Expr {};

// class Assignment : public StatementExpr {
//   std::shared_ptr<LValue> lvalue;
//   std::shared_ptr<Expr> expr;

// public:
//   Assignment(std::shared_ptr<LValue> lvalue, std::shared_ptr<Expr> expr)
//       : lvalue{lvalue}, expr{expr} {}

//   // Getters
//   std::shared_ptr<LValue> getLValue() const { return lvalue; }
//   std::shared_ptr<Expr> getExpr() const { return expr; }
// };

/**************************************************************************************/
// ***************************        NEED DISCUSSION !
// *******************************
/**************************************************************************************/
// Edward: we don't need to check args & QID since it is already handled in env
// build methodInvocation is still a node, same as the rest statementExpr I am
// putting them under ExprOp
/**************************************************************************************/

/*
class MethodInvocation : public StatementExpr, public ExprNode {
  std::shared_ptr<Expr> expr;
  std::string id;
  std::shared_ptr<QualifiedIdentifier> qualifiedIdentifier;
  std::vector<std::shared_ptr<Expr>> args;

public:
  MethodInvocation(std::shared_ptr<QualifiedIdentifier> qualifiedIdentifier,
                   std::vector<std::shared_ptr<Expr>> args)
      : qualifiedIdentifier{qualifiedIdentifier}, args{args} {};

  MethodInvocation(std::shared_ptr<Expr> expr, std::string identifier,
                   std::vector<std::shared_ptr<Expr>> args)
      : expr{expr}, id{id}, args{args} {};

  // Getters
  std::string getIdentifier() { return id; }
  std::shared_ptr<QualifiedIdentifier> getQualifiedIdentifier() {
    return qualifiedIdentifier;
  }
};

class ClassCreation : public StatementExpr, public ExprNode {
  std::shared_ptr<QualifiedIdentifier> qualifiedIdentifer;
  std::vector<std::shared_ptr<Expr>> args;

public:
  ClassCreation(std::shared_ptr<QualifiedIdentifier> qualifiedIdentifer,
                std::vector<std::shared_ptr<Expr>> args)
      : qualifiedIdentifer{qualifiedIdentifer}, args{args} {}

  std::shared_ptr<QualifiedIdentifier> getQualifiedIdentifier() const {
    return qualifiedIdentifer;
  }
  const std::vector<std::shared_ptr<Expr>> &getArgs() const { return args; }
};

class FieldAccess : public LValue, public ExprNode {
  std::shared_ptr<Expr> expr;
  std::string fieldName;

public:
  FieldAccess(std::shared_ptr<Expr> expr, std::string fieldName)
      : expr{expr}, fieldName{fieldName} {}

  // Getters
  std::shared_ptr<Expr> getExpr() const { return expr; }
  const std::string &getFieldName() const { return fieldName; }
};

class ArrayCreation : public Expr, public ExprNode {
  std::shared_ptr<QualifiedIdentifier> name;
  std::shared_ptr<Expr> size;
  std::shared_ptr<Type> type;

public:
  ArrayCreation(std::shared_ptr<QualifiedIdentifier> name,
                std::shared_ptr<Expr> size, std::shared_ptr<Type> type)
      : name{name}, size{size}, type{type} {}

  // Getters
  std::shared_ptr<QualifiedIdentifier> getName() const { return name; }
  std::shared_ptr<Expr> getSize() const { return size; }
  std::shared_ptr<Type> getType() const { return type; }
};

class ArrayAccess : public LValue, public ExprNode {
  std::shared_ptr<Expr> expr;
  std::shared_ptr<QualifiedIdentifier> qualifiedIdentifier;
  std::shared_ptr<Expr> index;

public:
  ArrayAccess(std::shared_ptr<QualifiedIdentifier> qualifiedIdentifier,
              std::shared_ptr<Expr> index)
      : expr{nullptr}, qualifiedIdentifier{qualifiedIdentifier}, index{index} {}

  ArrayAccess(std::shared_ptr<Expr> primaryNoArrayExpr,
              std::shared_ptr<Expr> index)
      : expr{nullptr}, qualifiedIdentifier{qualifiedIdentifier}, index{index} {}

  // Getters
  std::shared_ptr<Expr> getExpr() const { return expr; }
  std::shared_ptr<QualifiedIdentifier> getqualifiedIdentifier() const {
    return qualifiedIdentifier;
  }
  std::shared_ptr<Expr> getIndex() const { return index; }
};

class Cast : public ExprNode {};
*/

class TypeNode : public ExprNode {
  std::shared_ptr<Type> type;

public:
  TypeNode(std::shared_ptr<Type> type) : type{type} {};
};

// Operators /////////////////////////////////////////////////////////////

class ExprOp : public ExprNode {
protected:
  ExprOp(int num_args) : num_args{num_args} {}

private:
  int num_args;
};

// For AST

// we don't need operand because they are already saved in Expr rpn
class UnOp : public Expr {
public:
  enum class OpType { Not, Plus, Minus };
  UnOp(OpType op) : ExprOp{1}, op{op} {}

private:
  OpType op;
};

class BinOp : public Expr {
public:
  enum class OpType {
    GreaterThan,
    GreaterThanOrEqual,
    LessThan,
    LessThanOrEqual,
    Equal,
    NotEqual,
    Assign,
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    BitWiseAnd,
    BitWiseOr,
    Plus,
    Minus,
    InstanceOf
  };
  BinOp(OpType op) : ExprOp{2}, op{op} {}

private:
  OpType op;
};

class MethodInvocation : public ExprOp {
public:
  MethodInvocation(int num_args) : ExprOp(num_args) {}
};

class ClassCreation : public ExprOp {
public:
  ClassCreation(int num_args) : ExprOp(num_args) {}
};

class FieldAccess : public ExprOp {
public:
  FieldAccess() : ExprOp(1) {}
};

class ArrayCreation : public ExprOp {
public:
  ArrayCreation() : ExprOp(2) {}
};

class ArrayAccess : public ExprOp {
public:
  ArrayAccess() : ExprOp(2) {}
};

class Cast : public ExprOp {
public:
  Cast() : ExprOp(2) {}
};

} // namespace parsetree::ast
