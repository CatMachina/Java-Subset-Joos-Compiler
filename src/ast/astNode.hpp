#pragma once

#include "parsetree/ParseTree.h"
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <list>

namespace ast {

// Base class for all AST nodes //////////////////////////////////////////////

class QualifiedIdentifier;
class ExprOp;

class AstNode {
public:
  virtual ~AstNode() = default;
  virtual std::ostream &print(std::ostream &os) const = 0;
};

class Decl : public AstNode {
  std::string name;

public:
  explicit Decl(std::string_view name) : name{name} {}
  [[nodiscard]] std::string getName() const noexcept { return name; }
};

class CodeBody : public AstNode {};

class Type : public AstNode {
public:
  ~Type() override = default;
  [[nodiscard]] virtual std::string toString() const = 0;

  std::ostream &print(std::ostream &os) const override {
    return os << toString();
  }
};

class Stmt : public AstNode {};

class Expr : public AstNode {
  // Reverse Polish Notation
  std::list<ExprOp> rpn_ops;
};

std::ostream &operator<<(std::ostream &os, const AstNode &astNode);

// Decls /////////////////////////////////////////////////////////////

class PackageDecl : public Decl {
  std::shared_ptr<QualifiedIdentifier> qualifiedIdentifier;
};

struct ImportDecl {
  std::shared_ptr<QualifiedIdentifier> qualifiedIdentifier;
  bool hasStar;
};

class ProgramDecl : public CodeBody {
  std::shared_ptr<QualifiedIdentifier> package;
  std::vector<ImportDecl> imports;
  std::shared_ptr<CodeBody> body;

  public:
  ProgramDecl(std::shared_ptr<QualifiedIdentifier> package, std::vector<ImportDecl> imports, std::shared_ptr<CodeBody> body)
    : package{package}, imports{imports}, body{body} {}
};

class ClassDecl : public CodeBody, public Decl {
  std::shared_ptr<Modifiers> modifiers;
  std::shared_ptr<QualifiedIdentifier> superClass;
  std::vector<std::shared_ptr<QualifiedIdentifier>> interfaces;
  std::vector<std::shared_ptr<FieldDecl>> fields;
  std::vector<std::shared_ptr<MethodDecl>> constructors;
  std::vector<std::shared_ptr<MethodDecl>> methods;
  
public:
  ClassDecl(std::shared_ptr<Modifiers> modifiers,
      std::shared_ptr<QualifiedIdentifier> superClass,
      std::vector<std::shared_ptr<Decl>> classBodyDecls);
};

class InterfaceDecl : public Decl {
  std::shared_ptr<Modifiers> modifiers;
  std::vector<std::shared_ptr<QualifiedIdentifier>> extendsInterfaces;
  std::vector<std::shared_ptr<Decl>> interfaceBody;

public:
  InterfaceDecl(std::shared_ptr<Modifiers> modifiers,
      std::string_view name,
      std::vector<std::shared_ptr<QualifiedIdentifier>> extendsInterfaces,
      std::vector<std::shared_ptr<Decl>> interfaceBody);
};

class MethodDecl : public Decl {
  std::shared_ptr<Modifiers> methodModifiers;
  std::shared_ptr<Type> returnType;
  std::vector<std::shared_ptr<VarDecl>> params;
  std::shared_ptr<Stmt> methodBody;
  bool isConstructor_;

  public:
  MethodDecl(std::shared_ptr<Modifiers> methodModifiers,
      std::string_view name,
      std::shared_ptr<Type> returnType,
      std::vector<std::shared_ptr<VarDecl>> params,
      bool isConstructor,
      std::shared_ptr<Stmt> methodBody);
  bool isConstructor() const { return isConstructor_; }
};

class VarDecl : public Decl {
  std::shared_ptr<Type> type;
  std::shared_ptr<Expr> initializer;

  public:
  VarDecl(std::shared_ptr<Type> type, std::shared_ptr<Expr> initializer) : type{std::move(type)}, initializer{std::move(initializer)} {}
  bool hasInitializer() const { return initializer != nullptr; }
  std::shared_ptr<Expr> getInitializer() const { return initializer; }
  std::shared_ptr<Type> getType() const { return type; }
};

class FieldDecl : public VarDecl {
  std::shared_ptr<Modifiers> modifiers;

  public:
  FieldDecl(std::shared_ptr<Modifiers> modifiers, std::shared_ptr<Type> type, std::shared_ptr<Stmt> initializer)
      : modifiers{std::move(modifiers)}, VarDecl{std::move(type), std::move(initializer)} {
        if(modifiers.isFinal()) {
          throw std::runtime_error("FieldDecl cannot be final");
        }
        if(modifiers.isAbstract()) {
          throw std::runtime_error("FieldDecl cannot be abstract");
        }
        if(modifiers.isNative()) {
          throw std::runtime_error("FieldDecl cannot be native");
        }
        if(modifiers.isPublic() && modifiers.isProtected()) {
          throw std::runtime_error(
                "A method cannot be both public and protected. " + name);
        }
        if(!modifiers.isPublic() && !modifiers.isProtected()) {
          throw std::runtime_error("Field must have a visibility modifier");
        }
      }
};

class Param : public AstNode {
  std::shared_ptr<Type> type;
  std::string name;
  std::shared_ptr<Modifiers> modifiers;
};

// Statements /////////////////////////////////////////////////////////////

class Block : public Stmt {
  std::vector<std::shared_ptr<Stmt>> statements;

public:
  Block(std::vector<std::shared_ptr<Stmt>> statements) : statements{std::move(statements)} {}
};

class IfStmt : public Stmt {
  std::shared_ptr<Expr> condition;
  std::shared_ptr<Stmt> ifBody;
  std::shared_ptr<Stmt> elseBody;
};

class WhileStmt : public Stmt {
  std::shared_ptr<Expr> condition;
  std::shared_ptr<Stmt> whileBody;
};

class ForStmt : public Stmt {
  std::shared_ptr<Stmt> forInit;
  std::shared_ptr<Expr> condition;
  std::shared_ptr<Stmt> forUpdate;
};

class ReturnStmt : public Stmt {
  std::shared_ptr<Expr> returnExpr;
};

class ExpressionStmt : public Stmt {
  std::shared_ptr<StatementExpr> expr;
}

// Expressions /////////////////////////////////////////////////////////////

class Literal : public Expr {};

class LValue : public Expr {};

class StatementExpr : public Expr {};

class Assignment : public StatementExpr {
  std::shared_ptr<LValue> lvalue;
  std::shared_ptr<Expr> expr;
}

class MethodInvocation : public StatementExpr {
  std::shared_ptr<Expr> expr;
  std::vector<std::shared_ptr<Expr>> args;
};

class ClassCreation : public StatementExpr {
  std::shared_ptr<Type> type;
  std::vector<std::shared_ptr<Expr>> args;
};

class FieldAccess : public Expr {
  std::shared_ptr<Expr> expr;
  std::string fieldName;
};

class ArrayCreation : public Expr {
  std::shared_ptr<QualifiedIdentifier> name;
  std::shared_ptr<Expr> size;
  std::shared_ptr<Type> type;
};

class ArrayAccess : public Expr {
  std::shared_ptr<Expr> primaryNoArray;
  std::shared_ptr<QualifiedIdentifier> qid;
  std::shared_ptr<Expr> index;
};

class ArrayCast : public Expr {
  std::shared_ptr<QualifiedIdentifier> qid;
  std::shared_ptr<Type> type;
};

// Operators /////////////////////////////////////////////////////////////

class ExprOp {
protected:
    ExprOp(int num_args) : num_args{num_args} {}
private:
    int num_args;
};

class UnaryOp : ExprOp {
  OpType op;
public:
  enum class OpType {
    Not,
    Plus,
    Minus,
    BitWiseNot
  };
  UnaryOp(OpType op) : op{op}, ExprOp{1} {}
};

class BinaryOp : ExprOp {
private:
  OpType op;
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
  BinaryOp(OpType op) : op{op}, ExprOp{2} {}
};

// Types /////////////////////////////////////////////////////////////

class BuiltInType : public Type {
  TypeType type;
public:
  enum class TypeType {
    Int,
    Boolean,
    Short,
    Char,
    Void,
    Byte
  }

  BuiltInType(TypeType type) : type{type} {}
  BuiltInType(parsetree::BasicType::Type type) {
    switch(type) {
         case parsetree::BasicType::Type::Byte:
            kind = Kind::Byte;
            break;
         case parsetree::BasicType::Type::Short:
            kind = Kind::Short;
            break;
         case parsetree::BasicType::Type::Int:
            kind = Kind::Int;
            break;
         case parsetree::BasicType::Type::Char:
            kind = Kind::Char;
            break;
         case parsetree::BasicType::Type::Boolean:
            kind = Kind::Boolean;
            break;
         default:
            break;
      }
  }
};

class ArrayType : public Type {
   std::shared_ptr<Type> elementType;

public:
   ArrayType(std::shared_ptr<Type> elementType) : elementType{elementType} {}
   std::string toString() const override {
      return magic_enum::enum_name(*elementType) + "[]";
   }
};


// Other classes /////////////////////////////////////////////////////////////

class Modifiers {
  bool isPublic_ = false;
  bool isProtected_ = false;
  bool isStatic_ = false;
  bool isFinal_ = false;
  bool isAbstract_ = false;
  bool isNative_ = false;

public:
  void set(parsetree::Modifier modifier) {
    switch modifier.getType() {
      case Public:
        setPublic();
        break;
      case Protected:
        setProtected();
        break;
      case Static:
        setStatic();
        break;
      case Abstract:
        setAbstract();
        break;
      case Final:
        setFinal();
        break;
    }
  };

  void set(ast::Modifiers modifier);

  [[nodiscard]] bool isPublic() const noexcept { return isPublic_; }
  [[nodiscard]] bool isProtected() const noexcept { return isProtected_; }
  [[nodiscard]] bool isStatic() const noexcept { return isStatic_; }
  [[nodiscard]] bool isFinal() const noexcept { return isFinal_; }
  [[nodiscard]] bool isAbstract() const noexcept { return isAbstract_; }
  [[nodiscard]] bool isNative() const noexcept { return isNative_; }

  void setPublic() { isPublic_ = true };
  void setProtected() { isProtected_ = true };
  void setStatic() { isStatic_ = true };
  void setFinal() { isFinal_ = true };
  void setAbstract() { isAbstract_ = true };
  void setNative() { isNative_ = true };

  [[nodiscard]] std::string toString() const {
    std::string result;
    result += (isPublic_ ? "public " : "");
    result += (isProtected_ ? "protected " : "");
    result += (isStatic_ ? "static " : "");
    result += (isFinal_ ? "final " : "");
    result += (isAbstract_ ? "abstract " : "");
    result += (isNative_ ? "native " : "");
    return result;
  };

  friend std::ostream &operator<<(std::ostream &os, const Modifiers &mod) {
    return os << mod.toString();
  }
};

// Other classes /////////////////////////////////////////////////////////////

class QualifiedIdentifier {
  std::vector<std::string> identifiers;

public:
  void addIdentifier(std::string_view identifier) {
    identifiers.emplace_back(identifier);
  }

  [[nodiscard]] std::string toString() const {
    if (identifiers.empty())
      return "";

    std::string result;
    for (const auto &identifier : identifiers) {
      result += identifier + '.';
    }
    result.pop_back();
    return result;
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const QualifiedIdentifier &qid) {
    return os << qid.toString();
  }
};

} // namespace ast
