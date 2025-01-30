#pragma once

#include "parsetree/ParseTree.h"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace ast {

// Base class for all AST nodes //////////////////////////////////////////////

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

class ProgramDecl : public AstNode {
  std::vector<std::shared_ptr<PackageDecl>> packageDecls;
  std::vector<std::shared_ptr<ImportDecl>> importDecls;
  std::shared_ptr<Decl> typeDecl;
};

class Type : public AstNode {
public:
  ~Type() override = default;
  [[nodiscard]] virtual std::string toString() const = 0;

  std::ostream &print(std::ostream &os) const override {
    return os << toString();
  }
};

class Stmt : public AstNode {};

class Expr : public AstNode {};

std::ostream &operator<<(std::ostream &os, const AstNode &astNode);

// Decls /////////////////////////////////////////////////////////////

class PackageDecl : public Decl {
  std::shared_ptr<QualifiedIdentifier> qualifiedIdentifier;
};

class ImportDecl : public Decl {
  std::shared_ptr<QualifiedIdentifier> qualifiedIdentifier;
};

class ClassDecl : public Decl {
  std::shared_ptr<Modifiers> modifiers;
  std::vector<std::shared_ptr<QualifiedIdentifier>> superClasses;
  std::vector<std::shared_ptr<QualifiedIdentifier>> interfaces;
  std::vector<std::shared_ptr<Decl>> classBody;
};

class InterfaceDecl : public Decl {
  std::shared_ptr<Modifiers> modifiers;
  std::vector<std::shared_ptr<QualifiedIdentifier>> extendsInterfaces;
  std::vector<std::shared_ptr<Decl>> interfaceBody;
};

class FieldDecl : public Decl {
  std::shared_ptr<Modifiers> modifiers;
  std::shared_ptr<Type> type;
};

class MethodDecl : public Decl {
  std::shared_ptr<Modifiers> methodModifiers;
  std::shared_ptr<Type> returnType;
  std::vector<std::shared_ptr<Param>> params;
  std::shared_ptr<Block> methodBody;
};

class ConstructorDecl : public Decl {
  std::shared_ptr<Modifiers> methodModifiers;
  std::vector<std::shared_ptr<Param>> params;
  std::shared_ptr<Block> constructorBody;
};

class Param : public AstNode {
  std::shared_ptr<Type> type;
  std::string name;
  std::shared_ptr<Modifiers> modifiers;
};

// Statements /////////////////////////////////////////////////////////////

class Block : public Stmt {
  std::vector<std::shared_ptr<Stmt>> statements;
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

class UnaryOp : public AstNode {
  OpType op;
  std::shared_ptr<Expr> operand;
public:
  enum class OpType { };
};

class BinaryOp : public Expr {
private:
  OpType op;
  std::shared_ptr<Expr> left;
  std::shared_ptr<Expr> right;
public:
  enum class OpType { };
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
    result += (isPublic_ ? "public" : "");
    result += (isProtected_ ? "protected" : "");
    result += (isStatic_ ? "static" : "");
    result += (isFinal_ ? "final" : "");
    result += (isAbstract_ ? "abstract" : "");
    result += (isNative_ ? "native" : "");
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
