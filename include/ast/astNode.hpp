#pragma once

#include "parseTree/parseTree.hpp"
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <vector>

namespace parsetree::ast {

// Base class for all AST nodes //////////////////////////////////////////////

class QualifiedIdentifier;
class ExprOp;
class Modifiers;
class FieldDecl;
class MethodDecl;
class VarDecl;
class StatementExpr;
class Block;

class AstNode {
public:
  virtual ~AstNode() = default;
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

  std::ostream &print(std::ostream &os) const { return os << toString(); }
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

public:
  PackageDecl(std::string_view name,
              std::shared_ptr<QualifiedIdentifier> qualifiedIdentifier)
      : Decl{name}, qualifiedIdentifier{qualifiedIdentifier} {}

  std::shared_ptr<QualifiedIdentifier> getQualifiedIdentifier() const {
    return qualifiedIdentifier;
  }
};

class ImportDecl {
  std::shared_ptr<QualifiedIdentifier> qualifiedIdentifier;
  bool hasStar;

public:
  ImportDecl(std::shared_ptr<QualifiedIdentifier> qualifiedIdentifier,
             bool hasStar)
      : qualifiedIdentifier{qualifiedIdentifier}, hasStar{hasStar} {}
};

class ProgramDecl : public CodeBody {
  std::shared_ptr<QualifiedIdentifier> package;
  std::vector<ImportDecl> imports;
  std::shared_ptr<CodeBody> body;

public:
  ProgramDecl(std::shared_ptr<QualifiedIdentifier> package,
              std::vector<ImportDecl> imports, std::shared_ptr<CodeBody> body)
      : package{package}, imports{imports}, body{body} {
    std::unordered_set<std::string_view> simpleImportNames;
    std::unordered_set<std::string_view> fullQualifiedImportNames;

    for (const auto &importDecl : imports) {
      if (importDecl.hasStar) {
        continue;
      }

      std::string_view simpleName{importDecl.simpleName()};
      std::string_view qualifiedName{importDecl.type->toString()};

      // Ensure no conflicting single-type-import declarations
      if (simpleImportNames.contains(simpleName) &&
          !fullQualifiedImportNames.contains(qualifiedName)) {
        throw std::runtime_error(
            "No two single-type-import declarations clash with each other.");
      }

      simpleImportNames.insert(simpleName);
      fullQualifiedImportNames.insert(qualifiedName);
    }
  }

  std::shared_ptr<CodeBody> getBody() const { return body; }

  std::ostream &print(std::ostream &os) const;
};

class ClassDecl : public CodeBody, public Decl {
  std::shared_ptr<Modifiers> modifiers;
  std::shared_ptr<QualifiedIdentifier> superClass;
  std::vector<std::shared_ptr<QualifiedIdentifier>> interfaces;
  std::vector<std::shared_ptr<Decl>> classBodyDecls;

  std::vector<std::shared_ptr<FieldDecl>> fields;

public:
  ClassDecl(std::shared_ptr<Modifiers> modifiers, std::string_view name,
            std::shared_ptr<QualifiedIdentifier> superClass,
            std::vector<std::shared_ptr<QualifiedIdentifier>> interfaces,
            std::vector<std::shared_ptr<Decl>> classBodyDecls);

  std::ostream &print(std::ostream &os) const;

  auto fields() const { return std::views::all(fields); }
  auto interfaces() const { return std::views::all(interfaces); }
};

class InterfaceDecl : public CodeBody, public Decl {
  std::shared_ptr<Modifiers> modifiers;
  std::vector<std::shared_ptr<QualifiedIdentifier>> extendsInterfaces;
  std::vector<std::shared_ptr<Decl>> interfaceBody;

public:
  InterfaceDecl(
      std::shared_ptr<Modifiers> modifiers, std::string_view name,
      std::vector<std::shared_ptr<QualifiedIdentifier>> extendsInterfaces,
      std::vector<std::shared_ptr<Decl>> interfaceBody);

  std::ostream &print(std::ostream &os) const;
};

class MethodDecl : public Decl {
  std::shared_ptr<Modifiers> modifiers;
  std::shared_ptr<Type> returnType;
  std::vector<std::shared_ptr<VarDecl>> params;
  std::shared_ptr<Block> methodBody;
  bool isConstructor_;

  // Check for explicit this() or super() calls
  void checkSuperThisCalls(std::shared_ptr<Block> block) const;

public:
  MethodDecl(std::shared_ptr<Modifiers> modifiers, std::string_view name,
             std::shared_ptr<Type> returnType,
             std::vector<std::shared_ptr<VarDecl>> params, bool isConstructor,
             std::shared_ptr<Block> methodBody);
  std::shared_ptr<Modifiers> getModifiers() const { return modifiers; };
  bool isConstructor() const { return isConstructor_; }
  bool hasBody() const { return methodBody != nullptr; };
};

class VarDecl : public Decl {
  std::shared_ptr<Type> type;

public:
  VarDecl(std::shared_ptr<Type> type, std::string_view name)
      : Decl{name}, type{type} {}

  // Getters
  std::shared_ptr<Type> getType() const { return type; }
};

class FieldDecl : public VarDecl {
  std::shared_ptr<Modifiers> modifiers;

public:
  FieldDecl(std::shared_ptr<Modifiers> modifiers, std::shared_ptr<Type> type,
            std::string_view name);

  // Getters
  std::shared_ptr<Modifiers> getModifiers() const { return modifiers; }
};

class Param : public AstNode {
  std::shared_ptr<Type> type;
  std::string name;

public:
  Param(std::shared_ptr<Type> type, std::string_view name)
      : type{type}, name{name} {}

  // Getters
  std::shared_ptr<Type> getType() const { return type; }
  const std::string &getName() const { return name; }
};

// Statements /////////////////////////////////////////////////////////////

class Block : public Stmt {
  std::vector<std::shared_ptr<Stmt>> statements;

public:
  Block() : statements{std::vector<std::shared_ptr<Stmt>>{}} {}
  Block(std::vector<std::shared_ptr<Stmt>> statements)
      : statements{statements} {}

  // Getters
  const std::vector<std::shared_ptr<Stmt>> &getStatements() const {
    return statements;
  };
};

class IfStmt : public Stmt {
  std::shared_ptr<Expr> condition;
  std::shared_ptr<Stmt> ifBody;
  std::shared_ptr<Stmt> elseBody;

public:
  IfStmt(std::shared_ptr<Expr> condition, std::shared_ptr<Stmt> ifBody,
         std::shared_ptr<Stmt> elseBody = nullptr)
      : condition{condition}, ifBody{ifBody}, elseBody{elseBody} {};

  // Getters
  std::shared_ptr<Expr> getCondition() const { return condition; };
  std::shared_ptr<Stmt> getIfBody() const { return ifBody; };
  std::shared_ptr<Stmt> getElseBody() const { return elseBody; };
};

class WhileStmt : public Stmt {
  std::shared_ptr<Expr> condition;
  std::shared_ptr<Stmt> whileBody;

public:
  WhileStmt(std::shared_ptr<Expr> condition, std::shared_ptr<Stmt> whileBody)
      : condition{condition}, whileBody{whileBody} {};

  // Getters
  std::shared_ptr<Expr> getCondition() const { return condition; };
  std::shared_ptr<Stmt> getWhileBody() const { return whileBody; };
};

class ForStmt : public Stmt {
  std::shared_ptr<Stmt> forInit;
  std::shared_ptr<Expr> condition;
  std::shared_ptr<Stmt> forUpdate;
  std::shared_ptr<Stmt> forBody;

public:
  ForStmt(std::shared_ptr<Stmt> forInit, std::shared_ptr<Expr> condition,
          std::shared_ptr<Stmt> forUpdate, std::shared_ptr<Stmt> forBody)
      : forInit{forInit}, condition{condition}, forUpdate{forUpdate},
        forBody{forBody} {};

  // Getters
  std::shared_ptr<Stmt> getForInit() const { return forInit; };
  std::shared_ptr<Expr> getCondition() const { return condition; };
  std::shared_ptr<Stmt> getForUpdate() const { return forUpdate; };
  std::shared_ptr<Stmt> getForBody() const { return forBody; };
};

class ReturnStmt : public Stmt {
  std::shared_ptr<Expr> returnExpr;

public:
  explicit ReturnStmt(std::shared_ptr<Expr> returnExpr = nullptr)
      : returnExpr{returnExpr} {};

  // Getters
  std::shared_ptr<Expr> getReturnExpr() const { return returnExpr; };
};

class ExpressionStmt : public Stmt {
  std::shared_ptr<StatementExpr> statementExpr;

public:
  explicit ExpressionStmt(std::shared_ptr<StatementExpr> statementExpr)
      : statementExpr{statementExpr} {};

  // Getters
  std::shared_ptr<StatementExpr> getStatementExpr() const {
    return statementExpr;
  };
};

// Expressions /////////////////////////////////////////////////////////////

class Literal : public Expr {};

class LValue : public Expr {};

class StatementExpr : public Expr {};

class Assignment : public StatementExpr {
  std::shared_ptr<LValue> lvalue;
  std::shared_ptr<Expr> expr;

public:
  Assignment(std::shared_ptr<LValue> lvalue, std::shared_ptr<Expr> expr)
      : lvalue{lvalue}, expr{expr} {}

  // Getters
  std::shared_ptr<LValue> getLValue() const { return lvalue; }
  std::shared_ptr<Expr> getExpr() const { return expr; }
};

class MethodInvocation : public StatementExpr {
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

class ClassCreation : public StatementExpr {
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

class FieldAccess : public LValue {
  std::shared_ptr<Expr> expr;
  std::string fieldName;

public:
  FieldAccess(std::shared_ptr<Expr> expr, std::string fieldName)
      : expr{expr}, fieldName{fieldName} {}

  // Getters
  std::shared_ptr<Expr> getExpr() const { return expr; }
  const std::string &getFieldName() const { return fieldName; }
};

class ArrayCreation : public Expr {
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

class ArrayAccess : public LValue {
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

class ArrayCast : public Expr {
  std::shared_ptr<QualifiedIdentifier> qualifiedIdentifier;
  std::shared_ptr<Type> type;

public:
  ArrayCast(std::shared_ptr<QualifiedIdentifier> qualifiedIdentifier,
            std::shared_ptr<Type> type)
      : qualifiedIdentifier{qualifiedIdentifier}, type{type} {}

  // Getters
  std::shared_ptr<QualifiedIdentifier> getqualifiedIdentifier() const {
    return qualifiedIdentifier;
  }
  std::shared_ptr<Type> getType() const { return type; }
};

// Operators /////////////////////////////////////////////////////////////

class ExprOp {
protected:
  ExprOp(int num_args) : num_args{num_args} {}

private:
  int num_args;
};

class UnaryOp : ExprOp {
public:
  enum class OpType { Not, Plus, Minus, BitWiseNot };
  UnaryOp(OpType op) : op{op}, ExprOp{1} {}

private:
  OpType op;
};

class BinaryOp : ExprOp {
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

private:
  OpType op;
};

// Types /////////////////////////////////////////////////////////////

/*
TODO: The commented out BuiltinType didn't look right to me so I rewrote it (see
below) Needs to check if mine is correct

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
    switch (type) {
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
*/

class QualifiedIdentifier : public LValue {
  std::vector<std::string> identifiers;

public:
  const std::vector<std::string> &getIdentifiers() const {
    return identifiers;
  };

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

  friend std::ostream &
  operator<<(std::ostream &os, const QualifiedIdentifier &qualifiedIdentifier) {
    return os << qualifiedIdentifier.toString();
  }
};

class BuiltInType : public Type {
public:
  enum class Kind { Int, Boolean, Short, Char, Void, Byte };

  BuiltInType(Kind kind) : kind{kind} {}
  BuiltInType(parsetree::BasicType::Type type) {
    switch (type) {
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

  Kind getKind() const { return kind; }
  std::string toString() const override {
    return std::string(magic_enum::enum_name(kind));
  }

private:
  Kind kind;
};

class ArrayType : public Type {
  std::shared_ptr<Type> elementType;

public:
  ArrayType(std::shared_ptr<Type> elementType) : elementType{elementType} {}
  std::string toString() const override {
    return elementType->toString() + "[]";
  }
};

class ReferenceType : public Type {
  std::shared_ptr<QualifiedIdentifier> name;

public:
  ReferenceType(std::shared_ptr<QualifiedIdentifier> name) : name{name} {}
  std::string toString() const override { return name->toString(); }
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
    switch (modifier.get_type()) {
    case parsetree::Modifier::Type::Public:
      setPublic();
      break;
    case parsetree::Modifier::Type::Protected:
      setProtected();
      break;
    case parsetree::Modifier::Type::Static:
      setStatic();
      break;
    case parsetree::Modifier::Type::Abstract:
      setAbstract();
      break;
    case parsetree::Modifier::Type::Final:
      setFinal();
      break;
    case parsetree::Modifier::Type::Native:
      setNative();
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
  [[nodiscard]] bool isInvalid() const noexcept {
    return !isPublic_ && !isProtected_ && !isStatic_ && !isFinal_ &&
           !isAbstract_ && !isNative_;
  }

  void setPublic() { isPublic_ = true; };
  void setProtected() { isProtected_ = true; };
  void setStatic() { isStatic_ = true; };
  void setFinal() { isFinal_ = true; };
  void setAbstract() { isAbstract_ = true; };
  void setNative() { isNative_ = true; };

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

} // namespace parsetree::ast
