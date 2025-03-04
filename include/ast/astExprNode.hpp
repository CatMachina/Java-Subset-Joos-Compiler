#pragma once

#include "ast/astNode.hpp"

namespace parsetree::ast {

// Expressions /////////////////////////////////////////////////////////////

class ExprValue : public ExprNode {
public:
  explicit ExprValue() : decl_{nullptr}, type_{nullptr} {}

  std::shared_ptr<Decl> getResolvedDecl() const { return decl_; }
  std::shared_ptr<Type> getType() const { return type_; }

  void setResolvedDecl(const std::shared_ptr<Decl> resolvedDecl) {
    this->decl_ = resolvedDecl;
  }

  virtual bool isDeclResolved() const { return decl_ != nullptr; }
  bool isTypeResolved() const { return type_ != nullptr; }

  std::ostream &print(std::ostream &os) const override {
    os << "(ExprValue " << type_->toString() << ", " << decl_->getName() << ")";
    return os;
  }

  void resolveDeclAndType(std::shared_ptr<Decl> decl,
                          std::shared_ptr<Type> type) {
    if (decl_ && decl != decl_)
      throw std::runtime_error("Decl already resolved");
    decl_ = decl;
    if (type_ && type != type_)
      throw std::runtime_error("Type already resolved with " +
                               type_->toString() + " instead of " +
                               type->toString());
    if (type && !type->isResolved())
      throw std::runtime_error("Type not resolved");
    type_ = type;
  }

private:
  std::shared_ptr<Decl> decl_;
  std::shared_ptr<Type> type_;
};

class Literal : public ExprValue {
public:
  enum class Type { Integer, Character, String, Boolean, Null };

  Literal(Type type, std::string value) : type{type}, value{value} {}

  std::ostream &print(std::ostream &os) const override {
    os << "(Literal " << magic_enum::enum_name(type) << ", " << value << ")";
    return os;
  }

  bool isDeclResolved() const override { return true; }

  bool isString() const { return type == Type::String; }

  // Getters
  Type getType() const { return type; }
  std::string getValue() const { return value; }

private:
  Type type;
  std::string value;
};

class SimpleName : public ExprValue {
public:
  SimpleName(std::string name)
      : ExprValue{}, name{name}, shouldBeStatic{false} {}

  std::string getName() const { return name; }

  // std::shared_ptr<Decl> getResolvedDecl() {
  //   return resolvedDecl;
  // }
  // void setResolvedDecl(const std::shared_ptr<Decl> resolvedDecl) {
  //   this->resolvedDecl = resolvedDecl;
  // }

  std::ostream &print(std::ostream &os) const override {
    os << "(Simple Name " << name << ", shouldBeStatic: " << shouldBeStatic
       << ")";
    return os;
  }

  bool getShouldBeStatic() const { return shouldBeStatic; }
  void setShouldBeStatic() { shouldBeStatic = true; }

private:
  std::string name;
  // std::shared_ptr<Decl> resolvedDecl;

  bool shouldBeStatic;
};

class QualifiedName : public ExprNode {
public:
  QualifiedName(){};

  int size() { return simpleNames.size(); }

  std::shared_ptr<SimpleName> get(int index) { return simpleNames[index]; }
  std::string getName(int index) { return simpleNames[index]->getName(); }

  std::shared_ptr<SimpleName> getLast() { return simpleNames[size() - 1]; }
  std::string getLastName() { return simpleNames[size() - 1]->getName(); }

  void add(std::string name) {
    simpleNames.push_back(std::make_shared<SimpleName>(name));
  }
  void add(std::shared_ptr<SimpleName> simpleName) {
    simpleNames.push_back(simpleName);
  }

  std::ostream &print(std::ostream &os) const override {
    os << "(QualifiedName: ";
    bool first = true;
    for (const auto &simpleName : simpleNames) {
      if (first) {
        os << simpleName->getName();
        first = false;
      } else {
        os << "." << simpleName->getName();
      }
    }
    os << ")";
    return os;
  }

  std::vector<std::shared_ptr<AstNode>> getChildren() const override {
    std::vector<std::shared_ptr<AstNode>> children;
    for (const auto &simpleName : simpleNames) {
      children.push_back(std::dynamic_pointer_cast<AstNode>(simpleName));
    }
    return children;
  }

private:
  std::vector<std::shared_ptr<SimpleName>> simpleNames;
};

class MemberName : public ExprValue {
public:
  MemberName(std::string_view name) : ExprValue{}, name{name} {}

  std::ostream &print(std::ostream &os) const override {
    os << "(Member name: " << name << ")";
    return os;
  }

  // Getters
  std::string getName() const { return name; }

private:
  std::string name;
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

class TypeNode : public ExprValue {
  std::shared_ptr<Type> type;

public:
  TypeNode(std::shared_ptr<Type> type) : ExprValue{}, type{type} {};

  std::ostream &print(std::ostream &os) const {
    return os << "(Type: " << type->toString() << ")";
  }
  std::shared_ptr<Type> getType() const { return type; }

  std::vector<std::shared_ptr<AstNode>> getChildren() const override {
    std::vector<std::shared_ptr<AstNode>> children;
    children.push_back(std::dynamic_pointer_cast<AstNode>(type));
    return children;
  }

  bool isDeclResolved() const override { return true; }
};

class ThisNode : public ExprValue {
public:
  std::ostream &print(std::ostream &os) const override {
    os << "(This)";
    return os;
  }
};

class Separator : public ExprNode {
public:
  std::ostream &print(std::ostream &os) const { return os << " (|) "; }
};

// Operators /////////////////////////////////////////////////////////////

class ExprOp : public ExprNode {
public:
  int getNumArgs() const { return num_args; }

  std::shared_ptr<Type> getResultType() const { return resultType; }
  std::shared_ptr<Type> resolveResultType(std::shared_ptr<Type> type) {
    if (resultType)
      throw std::runtime_error("Tried to resolve op result type twice");
    if (!type || !type->isResolved())
      throw std::runtime_error("Tried to resolve op with unresolved type");
    resultType = type;
    return type;
  }

protected:
  ExprOp(int num_args) : num_args{num_args} {}

private:
  int num_args;
  std::shared_ptr<Type> resultType;
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

  OpType getOp() const { return op; }

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

  OpType getOp() const { return op; }

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
