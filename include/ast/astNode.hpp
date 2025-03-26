#pragma once

#include "parseTree/parseTree.hpp"
#include "parseTree/sourceNode.hpp"
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace static_check {
class Decl;
}

namespace parsetree::ast {

// Base class for all AST nodes //////////////////////////////////////////////

class ReferenceType;
class Expr;
class ExprOp;
class ExprNode;
class Modifiers;
class FieldDecl;
class MethodDecl;
class VarDecl;
class StatementExpr;
class Block;
class UnresolvedType;
class ScopeID;
class CodeBody;

class AstNode {
protected:
  std::ostream &printIndent(std::ostream &os, int indent = 0) const;

public:
  virtual ~AstNode() = default;
  virtual std::vector<std::shared_ptr<AstNode>> getChildren() const {
    return std::vector<std::shared_ptr<AstNode>>();
  }

  virtual std::ostream &print(std::ostream &os, int indent = 0) const = 0;
};

class Decl : virtual public AstNode {
protected:
  std::string name;
  std::weak_ptr<CodeBody> parent;
  source::SourceRange loc;
  std::string fullName = "";

public:
  explicit Decl(std::string name,
                const source::SourceRange loc = source::SourceRange())
      : name{name}, loc{loc} {}
  [[nodiscard]] std::string getName() const noexcept { return name; }
  [[nodiscard]] std::shared_ptr<CodeBody> getParent() const noexcept {
    return parent.lock();
  }

  [[nodiscard]] const source::SourceRange getLoc() const { return loc; }

  virtual bool isStatic() const { return false; }

  virtual void setParent(std::shared_ptr<CodeBody> rawParent);
  virtual std::shared_ptr<CodeBody> asCodeBody() const { return nullptr; }

  std::string getFullName() const {
    if (fullName.empty()) {
      return name;
    }
    return fullName;
  }
};

class CodeBody : virtual public AstNode {
public:
  // temp fix of unknown wrong parent issue
  virtual void reApplySetParent() { ; };

  std::vector<std::shared_ptr<Decl>> getDecls() {
    std::vector<std::shared_ptr<Decl>> declVector;
    reApplySetParent();
    for (auto child : getChildren()) {
      if (auto decl = std::dynamic_pointer_cast<Decl>(child)) {
        if (!(decl->getParent().get()))
          throw std::runtime_error("parent null in CodeBody::getDecls");
        if (!(decl->getParent().get() == this)) {

          std::cout << "wrong parent decl: ";
          decl->print(std::cout);
          std::cout << "\nwith parent: ";
          if (decl->getParent()) {
            decl->getParent()->print(std::cout);
          } else {
            std::cout << "null\n";
          };

          throw std::runtime_error(
              "child declaration of this context has the wrong parent");
        }

        declVector.push_back(decl);
      }
    }
    return declVector;
  }

  virtual std::shared_ptr<Decl> asDecl() const { return nullptr; }
};

class Type : public AstNode {
public:
  ~Type() override = default;
  [[nodiscard]] virtual std::string toString() const = 0;
  [[nodiscard]] virtual bool isResolved() const = 0;
  [[nodiscard]] virtual bool isString() const { return false; };
  [[nodiscard]] virtual bool isPrimitive() const { return false; };
  [[nodiscard]] virtual bool isNull() const { return false; };
  [[nodiscard]] virtual bool isNumeric() const { return false; }
  [[nodiscard]] virtual bool isBoolean() const { return false; }
  [[nodiscard]] virtual bool isArray() const { return false; }

  [[nodiscard]] virtual std::shared_ptr<Decl> getAsDecl() const {
    return nullptr;
  }

  virtual bool operator==(const Type &other) const = 0;

  // std::ostream &print(std::ostream &os, int indent = 0) const override {
  //   if (!toString().empty())
  //     printIndent(os, indent);
  //   return os << toString();
  // }
};

class Stmt : public AstNode {
public:
  std::ostream &print(std::ostream &os, int indent = 0) const override {
    return os << "(Stmt)";
  }

  virtual std::vector<std::shared_ptr<Expr>> getExprs() const = 0;
};

std::ostream &operator<<(std::ostream &os, const AstNode &astNode);

class ExprNode : public AstNode {
public:
  virtual ~ExprNode() = default;
  // virtual std::ostream &print(std::ostream &os, int indent = 0) const = 0;
  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    return os << "(ExprNode)";
  }
};

class Expr : public AstNode {
  // Reverse Polish Notation
  // TODO: We use vector for now
  std::vector<std::shared_ptr<ExprNode>> exprNodes;
  std::shared_ptr<ScopeID> scope;

public:
  Expr(std::vector<std::shared_ptr<ExprNode>> exprNodes,
       std::shared_ptr<ScopeID> scope)
      : exprNodes{exprNodes}, scope{scope} {}

  // Getter
  // right now we return a copy, inefficient i know...
  std::vector<std::shared_ptr<ExprNode>> &getExprNodes() { return exprNodes; }

  std::shared_ptr<ScopeID> getScope() { return scope; }

  void setExprNodes(std::vector<std::shared_ptr<ExprNode>> exprNodes) {
    this->exprNodes = exprNodes;
  }

  void setScope(std::shared_ptr<ScopeID> scope) { this->scope = scope; }

  const std::shared_ptr<ExprNode> getLastExprNode() const {
    if (exprNodes.empty()) {
      throw std::runtime_error("Empty expression");
    }
    return exprNodes.back();
  }

  std::ostream &print(std::ostream &os, int indent = 0) const {
    printIndent(os, indent);
    os << "(Expr \n";
    for (const auto &exprNode : exprNodes) {
      exprNode->print(os, indent + 1);
    }
    printIndent(os, indent);
    return os << ")\n";
  }

  std::vector<std::shared_ptr<AstNode>> getChildren() const override {
    std::vector<std::shared_ptr<AstNode>> children;
    for (const auto &node : exprNodes) {
      children.push_back(std::dynamic_pointer_cast<AstNode>(node));
    }
    return children;
  }
};

class ReferenceType : public Type {
  std::shared_ptr<Decl> decl;
  std::shared_ptr<static_check::Decl> resolvedDecl;

protected:
  // Only used by unresolved types.
  ReferenceType() : Type(), decl{nullptr} {}

public:
  ReferenceType(std::shared_ptr<Decl> decl) : decl{decl} {}
  virtual std::string toString() const override { return "ReferenceType"; }

  bool isResolved() const override { return resolvedDecl != nullptr; }
  std::shared_ptr<Decl> getAsDecl() const override {
    if (!decl && !isResolved())
      throw std::runtime_error("Decl not resolved");
    return decl;
  }

  void setResolvedDecl(const std::shared_ptr<static_check::Decl> resolvedDecl) {
    if (isResolved() && resolvedDecl != this->resolvedDecl) {
      throw std::runtime_error("Decl already resolved");
    }
    this->resolvedDecl = resolvedDecl;
  }

  std::shared_ptr<static_check::Decl> getResolvedDecl() { return resolvedDecl; }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(ReferenceType " << toString() << ")\n";
    return os;
  }

  bool operator==(const Type &other) const override;
};

class UnresolvedType : public ReferenceType {
  std::vector<std::string> identifiers;
  mutable std::string originalName = "";

public:
  const std::vector<std::string> &getIdentifiers() const {
    return identifiers;
  };

  std::string toString() const override {
    if (identifiers.empty())
      return "";
    if (!originalName.empty()) {
      return originalName;
    }
    for (auto &id : identifiers) {
      originalName += id;
      originalName += ".";
    }
    originalName.pop_back();
    return originalName;
  }

  void addIdentifier(std::string identifier) {
    identifiers.emplace_back(identifier);
  }
};

// Decls /////////////////////////////////////////////////////////////

class PackageDecl : public Decl {
  std::shared_ptr<ReferenceType> qualifiedIdentifier;

public:
  PackageDecl(std::string name,
              std::shared_ptr<ReferenceType> qualifiedIdentifier,
              const source::SourceRange loc)
      : Decl{name, loc}, qualifiedIdentifier{qualifiedIdentifier} {}

  std::shared_ptr<ReferenceType> getQualifiedIdentifier() const {
    return qualifiedIdentifier;
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override;
};

class ImportDecl : public AstNode {
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

  std::ostream &print(std::ostream &os, int indent = 0) const override;
};

class ProgramDecl : public CodeBody,
                    public std::enable_shared_from_this<ProgramDecl> {
  std::shared_ptr<ReferenceType> package;
  std::vector<std::shared_ptr<ImportDecl>> imports;
  std::shared_ptr<CodeBody> body;

public:
  ProgramDecl(std::shared_ptr<ReferenceType> package,
              std::vector<std::shared_ptr<ImportDecl>> imports,
              std::shared_ptr<CodeBody> body);

  std::shared_ptr<CodeBody> getBody() const { return body; }
  std::shared_ptr<ReferenceType> getPackage() const { return package; }
  std::string getPackageName() const { return package->toString(); }
  std::vector<std::shared_ptr<ImportDecl>> &getImports() { return imports; }

  // should only be called once
  void setAllParent() {
    auto decl = std::dynamic_pointer_cast<Decl>(body);
    if (decl) {
      auto ptr = std::const_pointer_cast<ProgramDecl>(shared_from_this());
      if (!ptr)
        throw std::runtime_error("Failed to cast to ProgramDecl");
      decl->setParent(std::dynamic_pointer_cast<CodeBody>(ptr));
    } else {
      throw std::runtime_error("Body wrong type in program decl!");
    }
  }

  bool isDefaultPackage() const {
    if (!package)
      return true;
    auto pkg = std::dynamic_pointer_cast<UnresolvedType>(package);
    if (!pkg)
      throw std::runtime_error("Package wrong type in program decl!");
    return pkg->getIdentifiers().size() == 0;
  }

  std::ostream &print(std::ostream &os, int indent = 0) const;

  std::vector<std::shared_ptr<AstNode>> getChildren() const override {
    std::vector<std::shared_ptr<AstNode>> children;
    children.push_back(std::dynamic_pointer_cast<AstNode>(package));
    for (const auto &node : imports) {
      children.push_back(std::dynamic_pointer_cast<AstNode>(node));
    }
    children.push_back(std::dynamic_pointer_cast<AstNode>(body));
    return children;
  }
};

class ClassDecl : virtual public CodeBody,
                  virtual public Decl,
                  public std::enable_shared_from_this<ClassDecl> {
  std::shared_ptr<Modifiers> modifiers;
  std::vector<std::shared_ptr<ReferenceType>> superClasses;
  std::vector<std::shared_ptr<ReferenceType>> interfaces;
  std::vector<std::shared_ptr<Decl>> classBodyDecls;
  std::shared_ptr<ReferenceType> objectType;

public:
  ClassDecl(std::shared_ptr<Modifiers> modifiers, std::string name,
            std::shared_ptr<ReferenceType> superClass,
            std::shared_ptr<ReferenceType> objectType,
            std::vector<std::shared_ptr<ReferenceType>> interfaces,
            std::vector<std::shared_ptr<Decl>> classBodyDecls);

  ClassDecl(std::string name) : Decl{name} {}

  std::ostream &print(std::ostream &os, int indent = 0) const override;

  std::vector<std::shared_ptr<ReferenceType>> getSuperClasses() {
    return superClasses;
  }

  std::vector<std::shared_ptr<ReferenceType>> getInterfaces() {
    return interfaces;
  }

  std::vector<std::shared_ptr<Decl>> getClassMembers() {
    return classBodyDecls;
  }

  std::shared_ptr<Modifiers> getModifiers() { return modifiers; }

  std::vector<std::shared_ptr<FieldDecl>> getFields() const {
    std::vector<std::shared_ptr<FieldDecl>> fields;

    for (const auto &decl : classBodyDecls) {
      if (auto fieldDecl = std::dynamic_pointer_cast<FieldDecl>(decl)) {
        fields.push_back(fieldDecl);
      }
    }

    return fields;
  }

  int getFieldOffset(std::shared_ptr<FieldDecl> field) const {
    int offset = 0;
    for (const auto &decl : classBodyDecls) {
      if (auto fieldDecl = std::dynamic_pointer_cast<FieldDecl>(decl)) {
        if (fieldDecl == field) {
          return offset;
        }
        offset += 1;
      }
    }

    return -1;
  }

  std::vector<std::shared_ptr<MethodDecl>> getMethods() const {
    std::vector<std::shared_ptr<MethodDecl>> methods;

    for (const auto &decl : classBodyDecls) {
      if (auto methodDecl = std::dynamic_pointer_cast<MethodDecl>(decl)) {
        methods.push_back(methodDecl);
      }
    }

    return methods;
  }

  std::vector<std::shared_ptr<MethodDecl>> getConstructors() const;

  std::vector<std::shared_ptr<AstNode>> getChildren() const override {
    std::vector<std::shared_ptr<AstNode>> children;
    for (const auto &node : classBodyDecls) {
      children.push_back(std::dynamic_pointer_cast<AstNode>(node));
    }
    for (const auto &node : interfaces) {
      children.push_back(std::dynamic_pointer_cast<AstNode>(node));
    }
    for (const auto &node : superClasses) {
      children.push_back(std::dynamic_pointer_cast<AstNode>(node));
    }
    // // do we need this
    // for (const auto &node : getFields()) {
    //   children.push_back(std::dynamic_pointer_cast<AstNode>(node));
    // }
    // for (const auto &node : getMethods()) {
    //   children.push_back(std::dynamic_pointer_cast<AstNode>(node));
    // }
    return children;
  }

  void setParent(std::shared_ptr<CodeBody> parent) override;

  std::shared_ptr<Modifiers> getModifiers() const { return modifiers; }

  // WARNING - ONLY USE FOR java.lang.Object
  void clearSuperClasses() { superClasses.clear(); }

  std::shared_ptr<CodeBody> asCodeBody() const override {
    auto ptr1 = std::const_pointer_cast<ClassDecl>(shared_from_this());
    auto ptr = std::dynamic_pointer_cast<CodeBody>(ptr1);
    if (!ptr)
      throw std::runtime_error("Failed to cast to CodeBody");
    return ptr;
    // return std::static_pointer_cast<CodeBody>(shared_from_this());
  }
  std::shared_ptr<Decl> asDecl() const override {
    auto ptr1 = std::const_pointer_cast<ClassDecl>(shared_from_this());
    auto ptr = std::dynamic_pointer_cast<Decl>(ptr1);
    if (!ptr)
      throw std::runtime_error("Failed to cast to CodeBody");
    return ptr;
    // return std::static_pointer_cast<Decl>(shared_from_this());
  }
};

class InterfaceDecl : virtual public CodeBody,
                      virtual public Decl,
                      public std::enable_shared_from_this<InterfaceDecl> {
  std::shared_ptr<Modifiers> modifiers;
  std::vector<std::shared_ptr<ReferenceType>> interfaces;
  std::vector<std::shared_ptr<Decl>> interfaceBodyDecls;
  std::shared_ptr<ReferenceType> objectType;

public:
  InterfaceDecl(std::shared_ptr<Modifiers> modifiers, std::string name,
                std::vector<std::shared_ptr<ReferenceType>> interfaces,
                std::shared_ptr<ReferenceType> objectType,
                std::vector<std::shared_ptr<Decl>> interfaceBody);

  std::ostream &print(std::ostream &os, int indent = 0) const;

  std::vector<std::shared_ptr<AstNode>> getChildren() const override {
    std::vector<std::shared_ptr<AstNode>> children;
    for (const auto &node : interfaces) {
      children.push_back(std::dynamic_pointer_cast<AstNode>(node));
    }
    for (const auto &node : interfaceBodyDecls) {
      children.push_back(std::dynamic_pointer_cast<AstNode>(node));
    }
    return children;
  }

  std::vector<std::shared_ptr<MethodDecl>> getMethods() {
    std::vector<std::shared_ptr<MethodDecl>> methods;

    for (const auto &decl : interfaceBodyDecls) {
      if (auto methodDecl = std::dynamic_pointer_cast<MethodDecl>(decl)) {
        methods.push_back(methodDecl);
      }
    }

    return methods;
  }

  std::vector<std::shared_ptr<ReferenceType>> getInterfaces() {
    return interfaces;
  }

  void setParent(std::shared_ptr<CodeBody> parent) override;

  std::shared_ptr<CodeBody> asCodeBody() const override {
    auto ptr1 = std::const_pointer_cast<InterfaceDecl>(shared_from_this());
    auto ptr = std::dynamic_pointer_cast<CodeBody>(ptr1);
    if (!ptr)
      throw std::runtime_error("Failed to cast to CodeBody");
    return ptr;
    // return std::static_pointer_cast<CodeBody>(shared_from_this());
  }
  std::shared_ptr<Decl> asDecl() const override {
    // return std::static_pointer_cast<Decl>(shared_from_this());
    auto ptr1 = std::const_pointer_cast<InterfaceDecl>(shared_from_this());
    auto ptr = std::dynamic_pointer_cast<Decl>(ptr1);
    if (!ptr)
      throw std::runtime_error("Failed to cast to CodeBody");
    return ptr;
  }
};

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

class VarDecl : public Decl {
  std::shared_ptr<Type> type;
  std::shared_ptr<Expr> initializer;
  std::shared_ptr<ScopeID> scope;
  bool inParam = false;

public:
  VarDecl(std::shared_ptr<Type> type, std::string name,
          std::shared_ptr<Expr> initializer, std::shared_ptr<ScopeID> scope,
          const source::SourceRange &loc)
      : Decl{name, loc}, type{type}, initializer{initializer}, scope{scope} {}

  bool hasInit() const { return initializer != nullptr; }
  bool isInParam() const { return inParam; }

  void setInParam() { inParam = true; }

  // Getters
  std::shared_ptr<Type> getType() const { return type; }
  std::shared_ptr<Expr> getInitializer() const { return initializer; }
  std::shared_ptr<ScopeID> getScope() const { return scope; }

  std::vector<std::shared_ptr<AstNode>> getChildren() const override {
    std::vector<std::shared_ptr<AstNode>> children;
    children.push_back(std::dynamic_pointer_cast<AstNode>(type));
    children.push_back(std::dynamic_pointer_cast<AstNode>(initializer));
    return children;
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override;
};

class FieldDecl final : public VarDecl {
  std::shared_ptr<Modifiers> modifiers;

public:
  FieldDecl(std::shared_ptr<Modifiers> modifiers, std::shared_ptr<Type> type,
            std::string name, std::shared_ptr<Expr> initializer,
            std::shared_ptr<ScopeID> scope, const source::SourceRange &loc,
            bool allowFinal = false);

  // Getters
  std::shared_ptr<Modifiers> getModifiers() const { return modifiers; }

  void setParent(std::shared_ptr<CodeBody> parent) override;

  bool isStatic() const override { return modifiers->isStatic(); }
};

class MethodDecl : public Decl,
                   public CodeBody,
                   public std::enable_shared_from_this<MethodDecl> {
  std::shared_ptr<Modifiers> modifiers;
  std::shared_ptr<Type> returnType;
  std::vector<std::shared_ptr<VarDecl>> params;
  std::vector<std::shared_ptr<VarDecl>> localDecls;
  std::shared_ptr<Block> methodBody;
  bool isConstructor_;

  // Check for explicit this() or super() calls
  void checkSuperThisCalls(std::shared_ptr<Block> block) const;

public:
  MethodDecl(std::shared_ptr<Modifiers> modifiers, std::string name,
             std::shared_ptr<Type> returnType,
             std::vector<std::shared_ptr<VarDecl>> params, bool isConstructor,
             std::shared_ptr<Block> methodBody, const source::SourceRange loc);

  template <std::ranges::range T>
  requires std::same_as<std::ranges::range_value_t<T>, std::shared_ptr<VarDecl>>
  void addDecls(T decls) {
    localDecls.reserve(decls.size());
    localDecls.insert(localDecls.end(), decls.begin(), decls.end());
  }

  std::shared_ptr<Modifiers> getModifiers() const { return modifiers; };
  bool isConstructor() const { return isConstructor_; }
  bool hasBody() const { return methodBody != nullptr; };
  bool isStatic() const override { return modifiers->isStatic(); }

  void reApplySetParent() override {
    for (const auto &decl : localDecls) {
      decl->setParent(std::static_pointer_cast<CodeBody>(shared_from_this()));
    }
  }

  std::vector<std::shared_ptr<AstNode>> getChildren() const override {
    std::vector<std::shared_ptr<AstNode>> children;
    children.push_back(std::dynamic_pointer_cast<AstNode>(returnType));
    for (const auto &node : params) {
      children.push_back(std::dynamic_pointer_cast<AstNode>(node));
    }
    for (const auto &node : localDecls) {
      children.push_back(std::dynamic_pointer_cast<AstNode>(node));
    }
    children.push_back(std::dynamic_pointer_cast<AstNode>(methodBody));
    return children;
  }

  std::string getSignature() const {
    std::string signature = getName() + "(";
    bool first = true;
    for (const auto &param : params) {
      if (!first)
        signature += ", ";

      std::string param_type = param->getType()->toString();
      size_t pos = param_type.find_last_of('.');
      if (pos != std::string::npos) {
        param_type = param_type.substr(pos + 1);
      }

      signature += param_type;
      first = false;
    }
    signature += ")";
    return signature;
  }

  void setParent(std::shared_ptr<CodeBody> parent) override;

  std::shared_ptr<Type> getReturnType() { return returnType; }
  std::shared_ptr<Block> getMethodBody() { return methodBody; }

  std::vector<std::shared_ptr<VarDecl>> &getParams() { return params; }
  std::vector<std::shared_ptr<VarDecl>> &getLocalDecls() { return localDecls; }
  std::shared_ptr<CodeBody> asCodeBody() const override {
    auto ptr1 = std::const_pointer_cast<MethodDecl>(shared_from_this());
    auto ptr = std::dynamic_pointer_cast<CodeBody>(ptr1);
    if (!ptr)
      throw std::runtime_error("Failed to cast to CodeBody");
    return ptr;
    // return std::static_pointer_cast<CodeBody>(shared_from_this());
  }
  std::shared_ptr<Decl> asDecl() const override {
    auto ptr1 = std::const_pointer_cast<MethodDecl>(shared_from_this());
    auto ptr = std::dynamic_pointer_cast<Decl>(ptr1);
    if (!ptr)
      throw std::runtime_error("Failed to cast to CodeBody");
    return ptr;
    // return std::static_pointer_cast<Decl>(shared_from_this());
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override;
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

  bool isEmpty() const { return statements.empty(); }

  std::vector<std::shared_ptr<AstNode>> getChildren() const override {
    std::vector<std::shared_ptr<AstNode>> children;
    for (const auto &node : statements) {
      children.push_back(std::dynamic_pointer_cast<AstNode>(node));
    }
    return children;
  }

  std::vector<std::shared_ptr<Expr>> getExprs() const { return {}; }

  std::ostream &print(std::ostream &os, int indent = 0) const override;
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

  std::vector<std::shared_ptr<AstNode>> getChildren() const override {
    std::vector<std::shared_ptr<AstNode>> children;
    children.push_back(std::dynamic_pointer_cast<AstNode>(condition));
    children.push_back(std::dynamic_pointer_cast<AstNode>(ifBody));
    if (elseBody != nullptr)
      children.push_back(std::dynamic_pointer_cast<AstNode>(elseBody));
    return children;
  }

  std::vector<std::shared_ptr<Expr>> getExprs() const override {
    std::vector<std::shared_ptr<Expr>> exprs;
    exprs.push_back(condition);
    return exprs;
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override;
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

  std::vector<std::shared_ptr<AstNode>> getChildren() const override {
    std::vector<std::shared_ptr<AstNode>> children;
    children.push_back(std::dynamic_pointer_cast<AstNode>(condition));
    children.push_back(std::dynamic_pointer_cast<AstNode>(whileBody));
    return children;
  }

  std::vector<std::shared_ptr<Expr>> getExprs() const override {
    std::vector<std::shared_ptr<Expr>> exprs;
    exprs.push_back(condition);
    return exprs;
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override;
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

  std::vector<std::shared_ptr<AstNode>> getChildren() const override {
    std::vector<std::shared_ptr<AstNode>> children;
    children.push_back(std::dynamic_pointer_cast<AstNode>(forInit));
    children.push_back(std::dynamic_pointer_cast<AstNode>(condition));
    children.push_back(std::dynamic_pointer_cast<AstNode>(forUpdate));
    children.push_back(std::dynamic_pointer_cast<AstNode>(forBody));
    return children;
  }

  std::vector<std::shared_ptr<Expr>> getExprs() const override {
    std::vector<std::shared_ptr<Expr>> exprs;
    exprs.push_back(condition);
    return exprs;
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override;
};

class ReturnStmt : public Stmt {
  std::shared_ptr<Expr> returnExpr;

public:
  explicit ReturnStmt(std::shared_ptr<Expr> returnExpr = nullptr)
      : returnExpr{returnExpr} {};

  // Getters
  std::shared_ptr<Expr> getReturnExpr() const { return returnExpr; };

  std::vector<std::shared_ptr<AstNode>> getChildren() const override {
    std::vector<std::shared_ptr<AstNode>> children;
    children.push_back(std::dynamic_pointer_cast<AstNode>(returnExpr));
    return children;
  }

  std::vector<std::shared_ptr<Expr>> getExprs() const override {
    std::vector<std::shared_ptr<Expr>> exprs;
    exprs.push_back(returnExpr);
    return exprs;
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(ReturnStmt \n";
    if (returnExpr)
      returnExpr->print(os, indent + 1);
    printIndent(os, indent);
    os << ")\n";
    return os;
  }
};

class ExpressionStmt : public Stmt {
  std::shared_ptr<Expr> statementExpr;

public:
  explicit ExpressionStmt(std::shared_ptr<Expr> statementExpr)
      : statementExpr{statementExpr} {};

  // Getters
  std::shared_ptr<Expr> getStatementExpr() const { return statementExpr; }

  std::vector<std::shared_ptr<AstNode>> getChildren() const override {
    std::vector<std::shared_ptr<AstNode>> children;
    children.push_back(std::dynamic_pointer_cast<AstNode>(statementExpr));
    return children;
  }

  std::vector<std::shared_ptr<Expr>> getExprs() const override {
    std::vector<std::shared_ptr<Expr>> exprs;
    exprs.push_back(statementExpr);
    return exprs;
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(ExpressionStmt \n";
    statementExpr->print(os, indent + 1);
    printIndent(os, indent);
    os << ")\n";
    return os;
  }
};

class DeclStmt : virtual public Stmt {
  std::shared_ptr<VarDecl> decl;

public:
  explicit DeclStmt(std::shared_ptr<VarDecl> decl) : decl{decl} {};

  // Getters
  std::shared_ptr<VarDecl> getDecl() const { return decl; };

  std::vector<std::shared_ptr<AstNode>> getChildren() const override {
    std::vector<std::shared_ptr<AstNode>> children;
    children.push_back(std::dynamic_pointer_cast<AstNode>(decl));
    return children;
  }

  std::vector<std::shared_ptr<Expr>> getExprs() const override { return {}; }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(DeclStmt \n";
    decl->print(os, indent + 1);
    printIndent(os, indent);
    os << ")\n";
    return os;
  }
};

class NullStmt : public Stmt {
public:
  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    return os << "(NullStmt)\n";
  };

  std::vector<std::shared_ptr<Expr>> getExprs() const override { return {}; }
};

// Types /////////////////////////////////////////////////////////////

class BasicType : public Type, public ExprNode {
public:
  enum class Type { Int, Boolean, Short, Char, Void, Byte, String };

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

  BasicType(parsetree::Literal::Type type) {
    switch (type) {
    case parsetree::Literal::Type::String:
      type_ = Type::String;
      break;
    case parsetree::Literal::Type::Null:
      type_ = Type::Void;
      break;
    case parsetree::Literal::Type::Integer:
      type_ = Type::Int;
      break;
    case parsetree::Literal::Type::Character:
      type_ = Type::Char;
      break;
    case parsetree::Literal::Type::Boolean:
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

  bool isResolved() const override { return true; }
  bool isString() const override { return type_ == Type::String; }
  bool isPrimitive() const override { return type_ != Type::String; }
  bool isNull() const override { return type_ == Type::Void; }
  bool isNumeric() const override {
    return type_ == Type::Int || type_ == Type::Char || type_ == Type::Short ||
           type_ == Type::Byte;
  }
  bool isBoolean() const override { return type_ == Type::Boolean; }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    parsetree::ast::Type::printIndent(os, indent);
    os << "(BasicType " << magic_enum::enum_name(type_) << ")\n";
    return os;
  }

  bool operator==(const parsetree::ast::Type &other) const override {
    const BasicType *otherBasic = dynamic_cast<const BasicType *>(&other);
    if (!otherBasic) {
      return false;
    }
    if (type_ != otherBasic->type_) {
      return false;
    }
    return true;
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

  bool isResolved() const override { return elementType->isResolved(); }
  bool isArray() const override { return true; }

  std::shared_ptr<Type> getElementType() const { return elementType; }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    Type::printIndent(os, indent);
    os << "(ArrayType \n";
    elementType->print(os, indent + 1);
    Type::printIndent(os, indent);
    os << ")\n";
    return os;
  }

  std::vector<std::shared_ptr<AstNode>> getChildren() const override {
    std::vector<std::shared_ptr<AstNode>> children;
    children.push_back(std::dynamic_pointer_cast<AstNode>(elementType));
    return children;
  }

  bool operator==(const Type &other) const override {
    const ArrayType *otherArrayType = dynamic_cast<const ArrayType *>(&other);
    if (!otherArrayType) {
      return false;
    }
    if (*elementType != *(otherArrayType->elementType)) {
      return false;
    }
    return true;
  }
};

// Other classes /////////////////////////////////////////////////////////////

class MethodType : public Type {
  std::shared_ptr<Type> returnType;
  std::vector<std::shared_ptr<Type>> paramTypes;

public:
  MethodType(std::shared_ptr<MethodDecl> method)
      : Type{}, returnType{method->getReturnType()}, paramTypes{} {
    for (auto param : method->getParams()) {
      paramTypes.push_back(param->getType());
    }
  }

  bool isResolved() const override { return true; }
  std::string toString() const override { return "MethodType"; }

  void setReturnType(std::shared_ptr<Type> returnType) {
    this->returnType = returnType;
  }
  std::shared_ptr<Type> getReturnType() const { return returnType; }
  const std::vector<std::shared_ptr<Type>> &getParamTypes() const {
    return paramTypes;
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(MethodType ";
    printIndent(os, indent + 1);
    os << "Return Type: ";
    if (returnType)
      returnType->print(os, indent + 2);
    os << "Param Types: [";
    bool paramTypesIndent = false;
    for (auto &paramType : paramTypes) {
      if (paramType) {
        os << "\n";
        paramType->print(os, indent + 2);
        paramTypesIndent = true;
      }
    }
    if (paramTypesIndent)
      printIndent(os, indent + 1);
    os << "]\n";
    printIndent(os, indent);
    os << ")\n";
    return os;
  }

  bool operator==(const Type &other) const override {
    const MethodType *otherMethod = dynamic_cast<const MethodType *>(&other);
    if (!otherMethod) {
      return false;
    }

    if (!(*returnType == *(otherMethod->returnType))) {
      return false;
    }

    if (paramTypes.size() != otherMethod->paramTypes.size()) {
      return false;
    }

    for (size_t i = 0; i < paramTypes.size(); ++i) {
      if (!(*(paramTypes[i]) == *(otherMethod->paramTypes[i]))) {
        return false;
      }
    }

    return true;
  }
};

class ScopeID final {
public:
  ScopeID(const std::shared_ptr<ScopeID> &parent, int pos)
      : parent_{parent}, pos_{pos} {}

public:
  std::shared_ptr<ScopeID> next(std::shared_ptr<ScopeID> parent) const {
    return std::make_shared<ScopeID>(parent, pos_ + 1);
  }

  bool canView(std::shared_ptr<ScopeID> other) const {
    assert(other != nullptr && "Can't view the null scope");
    // std::cout << "canView: this=" << toString() << " other=" <<
    // other->toString() << std::endl;
    if (this->parent_ == other->parent_) {
      return this->pos_ >= other->pos_;
    }
    if (this->parent_) {
      // std::cout << "canView: this_parent=" << this->parent_->toString() <<
      // std::endl;
      return this->parent_->canView(other);
    }
    return false;
  }

  std::shared_ptr<ScopeID> parent() const { return parent_; }

  static std::shared_ptr<ScopeID> New() {
    return std::make_shared<ScopeID>(nullptr, 0);
  }

  std::string toString() const {
    return (parent_ ? parent_->toString() + "." : "") + std::to_string(pos_);
  }

  friend std::ostream &operator<<(std::ostream &os, const ScopeID &id) {
    return os << id.toString();
  }

private:
  std::shared_ptr<ScopeID> parent_;
  const int pos_;
};

} // namespace parsetree::ast

namespace static_check {
class Decl {
  std::shared_ptr<parsetree::ast::Decl> astNode;

public:
  explicit Decl(std::shared_ptr<parsetree::ast::Decl> node) : astNode(node) {}
  void printDecl(int depth = 0) const {
    for (int i = 0; i < depth; ++i)
      std::cout << "  ";
    std::cout << "(Decl: " << astNode->getName() << ")"
              << "\n";
  }
  std::string getName() const { return astNode->getName(); }
  std::shared_ptr<parsetree::ast::Decl> getAstNode() const { return astNode; }
};
} // namespace static_check
