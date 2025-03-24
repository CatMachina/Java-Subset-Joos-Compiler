#pragma once

#include "ast/astNode.hpp"
#include <variant>

namespace parsetree::ast {

// Expressions /////////////////////////////////////////////////////////////

class ExprValue : public ExprNode {
public:
  explicit ExprValue(std::shared_ptr<Type> type = nullptr)
      : ExprNode{}, decl_{nullptr}, type_{type} {}

  std::shared_ptr<Decl> getResolvedDecl() const { return decl_; }
  std::shared_ptr<Type> getType() const { return type_; }

  void setResolvedDecl(const std::shared_ptr<Decl> resolvedDecl) {
    if (!resolvedDecl)
      throw std::runtime_error("setResolvedDecl Decl cannot be null");
    this->decl_ = resolvedDecl;
    if (type_) {
      auto refType = std::dynamic_pointer_cast<ReferenceType>(type_);
      if (!refType)
        return;
      if (!(refType->isResolved()))
        refType->setResolvedDecl(
            std::make_shared<static_check::Decl>(resolvedDecl));
    }
  }

  virtual bool isDeclResolved() const { return decl_ != nullptr; }
  bool isTypeResolved() const {
    if (!type_)
      return false;
    if (auto refType = std::dynamic_pointer_cast<ReferenceType>(type_)) {
      return refType->isResolved();
    }
    return true;
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(ExprValue ";
    os << type_->toString() << ", " << decl_->getName();
    os << ")\n";
    return os;
  }

  void resolveDeclAndType(std::shared_ptr<Decl> decl,
                          std::shared_ptr<Type> type) {
    if (!decl)
      throw std::runtime_error("resolveDeclAndType Decl cannot be null");
    if (!type)
      throw std::runtime_error("resolveDeclAndType Type cannot be null");
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

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Simple Name " << name << ", shouldBeStatic: " << shouldBeStatic
       << ")\n";
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

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(QualifiedName ";
    bool first = true;
    for (const auto &simpleName : simpleNames) {
      if (first) {
        os << simpleName->getName();
        first = false;
      } else {
        os << "." << simpleName->getName();
      }
    }
    os << ")\n";
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
  MemberName(std::string name, const source::SourceRange loc)
      : ExprValue{}, name{name}, loc{loc} {}

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Member name " << name << ", loc: " << loc << ")\n";
    return os;
  }

  void setAccessedByThis() { accessedByThis = true; }
  void setNotAsBase() { notAsBase = true; }
  void setinitializedInExpr() { initializedInExpr = true; }

  // Getters
  std::string getName() const { return name; }
  const source::SourceRange getLoc() const { return loc; }
  bool isAccessedByThis() const { return accessedByThis; }
  bool isNotAsBase() const { return notAsBase; }
  bool isinitializedInExpr() const { return initializedInExpr; }

private:
  std::string name;
  const source::SourceRange loc;
  bool accessedByThis = false;
  bool notAsBase = false;
  bool initializedInExpr = false;
};

class MethodName : public MemberName {
public:
  MethodName(std::string name, const source::SourceRange loc)
      : MemberName{name, loc} {}

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Method name " << getName() << ", loc: " << getLoc() << ")\n";
    return os;
  }
};

class TypeExprBase : public ExprValue {
public:
  TypeExprBase(std::shared_ptr<Type> type = nullptr) : ExprValue{type} {}
};

// class UnresolvedTypeExpr : public TypeExprBase, public UnresolvedType {
// public:
//   UnresolvedTypeExpr() : TypeExprBase{}, UnresolvedType() {}
//   std::ostream &print(std::ostream &os, int indent = 0) const override {
//     ExprNode::printIndent(os, indent);
//     os << "(UnresolvedTypeExpr ";
//     bool first = true;
//     for (const auto &id : getIdentifiers()) {
//       {
//         if (first) {
//           os << id;
//           first = false;
//         } else {
//           os << "." << id;
//         }
//       }
//       os << ")\n";
//     }
//     return os;
//   }
// };

class TypeNode : public TypeExprBase {
  std::shared_ptr<Type> unresolvedType;

public:
  TypeNode(std::shared_ptr<Type> type) : TypeExprBase{type} {};

  std::ostream &print(std::ostream &os, int indent = 0) const {
    printIndent(os, indent);
    return os << "(TypeNode " << getType()->toString() << ")\n";
  }
  // std::shared_ptr<Type> getUnresolvedType() const { return unresolvedType;
  // }

  // std::vector<std::shared_ptr<AstNode>> getChildren() const override {
  //   std::vector<std::shared_ptr<AstNode>> children;
  //   children.push_back(std::dynamic_pointer_cast<AstNode>(type));
  //   return children;
  // }

  bool isDeclResolved() const override { return true; }
};

class ThisNode : public ExprValue {
public:
  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(This)\n";
    return os;
  }
};

class Separator : public ExprNode {
public:
  std::ostream &print(std::ostream &os, int indent = 0) const {
    // Don't know what this is for, not putting an indent
    return os << " (|) ";
  }
};

// Operators /////////////////////////////////////////////////////////////

class ExprOp : public ExprNode {
public:
  int getNumArgs() const { return num_args; }

  std::shared_ptr<Type> getResultType() const { return resultType; }
  std::shared_ptr<Type> resolveResultType(std::shared_ptr<Type> type) {
    if (!type) {
      return nullptr;
    }
    if (!type->isResolved()) {
      type->print(std::cout);
      throw std::runtime_error("Tried to resolve op with unresolved type");
    }

    if (resultType && (resultType != type)) {
      std::cout << "current type decl: ";
      resultType->print(std::cout);
      std::cout << " new type decl: ";
      type->print(std::cout);
      throw std::runtime_error(
          "Tried to resolve op result type twice with different decl");
    }

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

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(UnOp ";
    os << magic_enum::enum_name(op);
    os << ")\n";
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

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(BinOp ";
    os << magic_enum::enum_name(op) << ")\n";
    return os;
  }

  OpType getOp() const { return op; }

private:
  OpType op;
};

class Assignment : public ExprOp {
  std::shared_ptr<VarDecl> assignedVariable; // only for assignment
public:
  Assignment() : ExprOp(2){};

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Assignment)\n";
    return os;
  }

  std::shared_ptr<VarDecl> getAssignedVariable() const {
    return assignedVariable;
  }
  void setAssignedVariable(std::shared_ptr<VarDecl> var) {
    if (assignedVariable && assignedVariable != var)
      throw std::runtime_error("Assignment already set");
    assignedVariable = var;
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

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(MethodInvocation: " << getNumArgs();
    bool first = true;
    for (const auto &qid : qualifiedIdentifier) {
      if (first)
        os << "\n";
      if (!qid) {
        os << "null";
      } else {
        printIndent(os, indent + 1);
        qid->print(os);
      }
      first = false;
    }
    printIndent(os, indent);
    os << ")\n";
    return os;
  }
};

class ClassCreation : public ExprOp {
public:
  ClassCreation(int num_args) : ExprOp(num_args) {}

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(ClassCreation: " << getNumArgs() << " args)\n";
    return os;
  }
};

class FieldAccess : public ExprOp {
public:
  // Question: Why 1?
  FieldAccess() : ExprOp(1) {}

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(FieldAccess)\n";
    return os;
  }
};

class ArrayCreation : public ExprOp {
public:
  ArrayCreation() : ExprOp(2) {}

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(ArrayCreation)\n";
    return os;
  }
};

class ArrayAccess : public ExprOp {
public:
  ArrayAccess() : ExprOp(2) {}

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(ArrayAccess)\n";
    return os;
  }
};

class Cast : public ExprOp {
public:
  Cast() : ExprOp(2) {}

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Cast)\n";
    return os;
  }
};

static uint8_t parseChar(std::string value) {
  // Consume ' first, next value is either \ or a character
  // If it is a character, just return that character
  if (value.at(1) != '\\')
    return (uint8_t)value.at(1);
  // Here, we have an escape sequence, let's first handle the octal case
  if (isdigit(value.at(2))) {
    // We have an octal escape sequence of 1 to 3 digits
    uint8_t octal[3] = {0, 0, 0};
    // 1 digit '\0
    octal[0] = value.at(2) - '0';
    // 2 digits '\00
    if (value.length() >= 4)
      octal[1] = value.at(3) - '0';
    // 3 digits '\000
    if (value.length() >= 5)
      octal[2] = value.at(4) - '0';
    // Must consume ' then return the character
    return (uint8_t)((octal[0] << 6) | (octal[1] << 3) | octal[2]);
  }
  // Here, we have a non-octal escape sequence
  switch (value.at(2)) {
  case 'n':
    return '\n';
  case 't':
    return '\t';
  case 'r':
    return '\r';
  case 'b':
    return '\b';
  case 'f':
    return '\f';
  case '\\':
    return '\\';
  case '\'':
    return '\'';
  case '\"':
    return '\"';
  default:
    assert(false && "Invalid escape sequence");
  }
}

static void unescapeString(std::string in, std::string &out) {
  // minor: String literals are broken for now
  for (size_t i = 1; i < in.length(); i++) {
    char c = in.at(i);
    if (c == '\"') {
      break;
    } else {
      out.push_back(c);
    }
  }
}

class Literal : public ExprValue {
public:
  enum class Type { Integer, Character, String, Boolean, Null };

  Literal::Type literalType;

  // Literal(Type type, std::string value) : type{type}, value{value} {}
  Literal(std::shared_ptr<parsetree::Literal> node,
          std::shared_ptr<parsetree::ast::BasicType> type)
      : ExprValue{std::dynamic_pointer_cast<parsetree::ast::Type>(type)} {
    auto str = node->getValue();

    // 1. Check if the type is numeric
    if (type->isNumeric()) {
      int64_t val = 0;
      if (type->getType() == BasicType::Type::Char) {
        literalType = Literal::Type::Character;
        val = parseChar(str);
      } else {
        // Convert the string to an integer
        try {
          literalType = Literal::Type::Integer;
          if (node->isNegativeVal())
            val = std::stoi("-" + std::string(str));
          else
            val = std::stoi(std::string(str));
        } catch (std::invalid_argument &e) {
          throw std::runtime_error("Invalid integer literal");
        }
      }
      value = val;
    }
    // 2. Otherwise, check if the type is boolean
    else if (type->isBoolean()) {
      literalType = Literal::Type::Boolean;
      if (str == "true") {
        value = 1U;
      } else if (str == "false") {
        value = 0U;
      } else {
        throw std::runtime_error("Invalid boolean literal");
      }
    }
    // 3. Otherwise, its a string
    else if (type->isString()) {
      literalType = Literal::Type::String;
      // Unescape the string
      value = std::string{};
      unescapeString(str, std::get<std::string>(value));
    }
    // 4. Maybe it's a NoneType (i.e., NULL)
    else if (type->getType() == BasicType::Type::Void) {
      literalType = Literal::Type::Null;
      value = 0U;
    }
    // 5. Otherwise, it's an invalid type
    else {
      throw std::runtime_error("Invalid type for literal node");
    }
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    // os << "(Literal " << magic_enum::enum_name(type) << ", " << value <<
    // ")"; return os;
    printIndent(os, indent);
    os << "(Literal \n";
    getBasicType()->print(os, indent + 1);
    printIndent(os, indent);
    os << ")\n";
    return os;
  }

  bool isDeclResolved() const override { return true; }

  bool isString() const { return std::holds_alternative<std::string>(value); }

  // Getters
  Type getLiteralType() const { return literalType; }
  // std::string getValue() const { return value; }
  int64_t getAsInt() const { return std::get<int64_t>(value); }
  auto const &getAsString() const { return std::get<std::string>(value); }
  std::shared_ptr<BasicType> getBasicType() const {
    return std::dynamic_pointer_cast<BasicType>(getType());
  }

private:
  // Type type;
  // std::string value;
  std::variant<int64_t, std::string> value;
};

} // namespace parsetree::ast
