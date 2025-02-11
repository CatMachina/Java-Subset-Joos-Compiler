#pragma once

#include "parseTree/parseTree.hpp"
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace parsetree::ast {

// Base class for all AST nodes //////////////////////////////////////////////

class QualifiedIdentifier;
class ReferenceType;
class ExprOp;
class ExprNode;
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

std::ostream &operator<<(std::ostream &os, const AstNode &astNode);

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
  // right now we return a copy, inefficient i know...
  std::vector<std::shared_ptr<ExprNode>> getExprNodes() const {
    return exprNodes;
  }

  const std::shared_ptr<ExprNode> getLastExprNode() const {
    if (exprNodes.empty()) {
      throw std::runtime_error("Empty expression");
    }
    return exprNodes.back();
  }
};

// Decls /////////////////////////////////////////////////////////////

class PackageDecl : public Decl {
  std::shared_ptr<ReferenceType> qualifiedIdentifier;

public:
  PackageDecl(std::string_view name,
              std::shared_ptr<ReferenceType> qualifiedIdentifier)
      : Decl{name}, qualifiedIdentifier{qualifiedIdentifier} {}

  std::shared_ptr<ReferenceType> getQualifiedIdentifier() const {
    return qualifiedIdentifier;
  }
};

class ImportDecl {
  std::shared_ptr<ReferenceType> qualifiedIdentifier;
  bool hasStar_;

public:
  ImportDecl(std::shared_ptr<ReferenceType> qualifiedIdentifier, bool hasStar)
      : qualifiedIdentifier{qualifiedIdentifier}, hasStar_{hasStar} {}

  // Getters
  bool hasStar() const { return hasStar_; }
  std::shared_ptr<ReferenceType> getQualifiedIdentifier() const {
    return qualifiedIdentifier;
  }
};

class ProgramDecl : public CodeBody {
  std::shared_ptr<ReferenceType> package;
  std::vector<ImportDecl> imports;
  std::shared_ptr<CodeBody> body;

public:
  ProgramDecl(std::shared_ptr<ReferenceType> package,
              std::vector<ImportDecl> imports, std::shared_ptr<CodeBody> body);

  std::shared_ptr<CodeBody> getBody() const { return body; }

  std::ostream &print(std::ostream &os) const;
};

class ClassDecl : public CodeBody, public Decl {
  std::shared_ptr<Modifiers> modifiers;
  std::shared_ptr<ReferenceType> superClass;
  std::vector<std::shared_ptr<ReferenceType>> interfaces;
  std::vector<std::shared_ptr<Decl>> classBodyDecls;

public:
  ClassDecl(std::shared_ptr<Modifiers> modifiers, std::string_view name,
            std::shared_ptr<ReferenceType> superClass,
            std::vector<std::shared_ptr<ReferenceType>> interfaces,
            std::vector<std::shared_ptr<Decl>> classBodyDecls);

  std::ostream &print(std::ostream &os) const;
};

class InterfaceDecl : public CodeBody, public Decl {
  std::shared_ptr<Modifiers> modifiers;
  std::vector<std::shared_ptr<ReferenceType>> extendsInterfaces;
  std::vector<std::shared_ptr<Decl>> interfaceBody;

public:
  InterfaceDecl(std::shared_ptr<Modifiers> modifiers, std::string_view name,
                std::vector<std::shared_ptr<ReferenceType>> extendsInterfaces,
                std::vector<std::shared_ptr<Decl>> interfaceBody);

  std::ostream &print(std::ostream &os) const;
};

class MethodDecl : public Decl {
  std::shared_ptr<Modifiers> modifiers;
  std::shared_ptr<Type> returnType;
  std::vector<std::shared_ptr<VarDecl>> params;
  std::vector<std::shared_ptr<VarDecl>> localDecls;
  std::shared_ptr<Block> methodBody;
  bool isConstructor_;

  // Check for explicit this() or super() calls
  void checkSuperThisCalls(std::shared_ptr<Block> block) const;

public:
  MethodDecl(std::shared_ptr<Modifiers> modifiers, std::string_view name,
             std::shared_ptr<Type> returnType,
             std::vector<std::shared_ptr<VarDecl>> params, bool isConstructor,
             std::shared_ptr<Block> methodBody);

  template <std::ranges::range T>
  requires std::same_as<std::ranges::range_value_t<T>, std::shared_ptr<VarDecl>>
  void addDecls(T decls) {
    localDecls.reserve(decls.size());
    localDecls.insert(localDecls.end(), decls.begin(), decls.end());
  }

  std::shared_ptr<Modifiers> getModifiers() const { return modifiers; };
  bool isConstructor() const { return isConstructor_; }
  bool hasBody() const { return methodBody != nullptr; };
};

class VarDecl : public Decl {
  std::shared_ptr<Type> type;
  std::shared_ptr<Expr> initializer;

public:
  VarDecl(std::shared_ptr<Type> type, std::string_view name,
          std::shared_ptr<Expr> initializer)
      : Decl{name}, type{type}, initializer{initializer} {}

  bool hasInit() const { return initializer != nullptr; }

  // Getters
  std::shared_ptr<Type> getType() const { return type; }
  std::shared_ptr<Expr> getInitializer() const { return initializer; }
};

class FieldDecl : public VarDecl {
  std::shared_ptr<Modifiers> modifiers;

public:
  FieldDecl(std::shared_ptr<Modifiers> modifiers, std::shared_ptr<Type> type,
            std::string_view name, std::shared_ptr<Expr> initializer);

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
  std::shared_ptr<Expr> statementExpr;

public:
  explicit ExpressionStmt(std::shared_ptr<Expr> statementExpr)
      : statementExpr{statementExpr} {};

  // Getters
  std::shared_ptr<Expr> getStatementExpr() const { return statementExpr; };
};

class DeclStmt : public Stmt {
  std::shared_ptr<VarDecl> decl;

public:
  explicit DeclStmt(std::shared_ptr<VarDecl> decl) : decl{decl} {};

  // Getters
  std::shared_ptr<VarDecl> getDecl() const { return decl; };
};

class NullStmt : public Stmt {};

// Types /////////////////////////////////////////////////////////////

class QualifiedIdentifier : public ExprNode {
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

class BasicType : public Type, public ExprNode {
public:
  enum class Type { Int, Boolean, Short, Char, Void, Byte };

  BasicType(Type type) : type_{type} {}
  BasicType(parsetree::BasicType::Type type) {
    switch (type) {
    case parsetree::BasicType::Type::Byte:
      type_ = Type::Byte;
      break;
    case parsetree::BasicType::Type::Short:
      type_ = Type::Short;
      break;
    case parsetree::BasicType::Type::Int:
      type_ = Type::Int;
      break;
    case parsetree::BasicType::Type::Char:
      type_ = Type::Char;
      break;
    case parsetree::BasicType::Type::Boolean:
      type_ = Type::Boolean;
      break;
    default:
      break;
    }
  }

  Type getType() const { return type_; }
  std::string toString() const override {
    return std::string(magic_enum::enum_name(type_));
  }

private:
  Type type_;
};

class ArrayType : public Type, public ExprNode {
  std::shared_ptr<Type> elementType;

public:
  ArrayType(std::shared_ptr<Type> elementType) : elementType{elementType} {}
  std::string toString() const override {
    return elementType->toString() + "[]";
  }
};

class ReferenceType : public Type {
  std::shared_ptr<Decl> decl;

protected:
  // Only used by unresolved types.
  ReferenceType() : Type(), decl{nullptr} {}

public:
  ReferenceType(std::shared_ptr<Decl> decl) : decl{decl} {}
  std::string toString() const override { return "ReferenceType"; }
};

class UnresolvedType : public ReferenceType {
  std::vector<std::string> identifiers;

public:
  const std::vector<std::string> &getIdentifiers() const {
    return identifiers;
  };

  void addIdentifier(std::string_view identifier) {
    identifiers.emplace_back(identifier);
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
