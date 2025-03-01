#pragma once

#include "ast/astNode.hpp"

namespace parsetree::ast {

// Expressions /////////////////////////////////////////////////////////////

class Literal : public ExprNode {
public:
  enum class Type { Integer, Character, String, Boolean, Null };

  Literal(Type type, std::string value) : type{type}, value{value} {}

  std::ostream &print(std::ostream &os) const override {
    os << "(Literal " << magic_enum::enum_name(type) << ", " << value << ")";
    return os;
  }

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

  std::string getName() const { return name; }

  std::shared_ptr<parsetree::ast::Decl> getResolvedDecl() {
    return resolvedDecl;
  }
  void setResolvedDecl(const std::shared_ptr<Decl> resolvedDecl) {
    this->resolvedDecl = resolvedDecl;
  }

  std::ostream &print(std::ostream &os) const override {
    os << "(Member name: " << name << ")";
    return os;
  }

private:
  std::string name;
  std::shared_ptr<Decl> resolvedDecl;
};

class MethodName : public MemberName {
public:
  MethodName(std::string_view name) : MemberName{name} {}

  std::ostream &print(std::ostream &os) const override {
    os << "(Method name: " << getName() << ")";
    return os;
  }
};

class UnresolvedTypeExpr : public ExprNode, public UnresolvedType {
public:
  UnresolvedTypeExpr() : UnresolvedType() {}
  std::ostream &print(std::ostream &os) const override {
    os << "(UnresolvedTypeExpr: ";
    for (const auto &id : getIdentifiers()) {
      os << id << ".";
    }
    os << ")";
    return os;
  }
};

class ThisNode : public MemberName {
public:
  ThisNode() : MemberName{"this"} {}

  std::ostream &print(std::ostream &os) const override {
    os << "(This)";
    return os;
  }
};

class TypeNode : public ExprNode {
  std::shared_ptr<Type> type;

public:
  TypeNode(std::shared_ptr<Type> type) : type{type} {};

  std::ostream &print(std::ostream &os) const {
    return os << "(Type: " << type->toString() << ")";
  }
  std::shared_ptr<Type> getType() const { return type; }

  std::vector<std::shared_ptr<AstNode>> getChildren() const override {
    std::vector<std::shared_ptr<AstNode>> children;
    children.push_back(std::dynamic_pointer_cast<AstNode>(type));
    return children;
  }
};

class Separator : public ExprNode {
public:
  std::ostream &print(std::ostream &os) const { return os << " (|) "; }
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
class UnOp : public ExprOp {
public:
  enum class OpType { Not, Plus, Minus };
  UnOp(OpType op) : ExprOp{1}, op{op} {}

  std::ostream &print(std::ostream &os) const override {
    os << "(UnOp " << magic_enum::enum_name(op) << ")";
    return os;
  }

private:
  OpType op;
};

class BinOp : public ExprOp {
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
    InstanceOf,
    And,
    Or
  };
  BinOp(OpType op) : ExprOp{2}, op{op} {}

  std::ostream &print(std::ostream &os) const override {
    os << "(BinOp " << magic_enum::enum_name(op) << ")";
    return os;
  }

private:
  OpType op;
};

class Assignment : public ExprOp {
public:
  Assignment() : ExprOp(2){};

  std::ostream &print(std::ostream &os) const override {
    os << "(Assignment)";
    return os;
  }
};

class MethodInvocation : public ExprOp {
  std::vector<std::shared_ptr<ast::ExprNode>> qualifiedIdentifier;

public:
  MethodInvocation(int num_args,
                   std::vector<std::shared_ptr<ast::ExprNode>> &qid)
      : ExprOp(num_args), qualifiedIdentifier{qid} {}
  std::vector<std::shared_ptr<ast::ExprNode>> &getQualifiedIdentifier() {
    return qualifiedIdentifier;
  }

  std::ostream &print(std::ostream &os) const override {
    os << "(MethodInvocation: ";
    for (const auto &qid : qualifiedIdentifier) {
      if (!qid) {
        os << "null";
      } else {
        qid->print(os);
      }
      os << " ";
    }
    os << ")";
    return os;
  }
};

class ClassCreation : public ExprOp {
public:
  ClassCreation(int num_args) : ExprOp(num_args) {}

  std::ostream &print(std::ostream &os) const override {
    os << "(ClassCreation)";
    return os;
  }
};

class FieldAccess : public ExprOp {
public:
  // Question: Why 1?
  FieldAccess() : ExprOp(1) {}

  std::ostream &print(std::ostream &os) const override {
    os << "(FieldAccess)";
    return os;
  }
};

class ArrayCreation : public ExprOp {
public:
  ArrayCreation() : ExprOp(2) {}

  std::ostream &print(std::ostream &os) const override {
    os << "(ArrayCreation)";
    return os;
  }
};

class ArrayAccess : public ExprOp {
public:
  ArrayAccess() : ExprOp(2) {}

  std::ostream &print(std::ostream &os) const override {
    os << "(ArrayAccess)";
    return os;
  }
};

class Cast : public ExprOp {
public:
  Cast() : ExprOp(2) {}

  std::ostream &print(std::ostream &os) const override {
    os << "(Cast)";
    return os;
  }
};

} // namespace parsetree::ast
