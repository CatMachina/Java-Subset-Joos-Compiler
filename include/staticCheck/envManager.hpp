#pragma once

#include "ast/ast.hpp"
#include "parseTree/parseTree.hpp"
#include <ranges>

namespace static_check {

class EnvManager {
public:
  EnvManager() {
    // everything inherits java.lang.Object
    objectType = BuildUnresolvedType();
    objectType->addIdentifier("java");
    objectType->addIdentifier("lang");
    objectType->addIdentifier("Object");
  }

  [[nodiscard]] std::shared_ptr<parsetree::ast::ProgramDecl> BuildProgramDecl(
      const std::shared_ptr<parsetree::ast::ReferenceType> &package,
      std::vector<std::shared_ptr<parsetree::ast::ImportDecl>> imports,
      const std::shared_ptr<parsetree::ast::CodeBody> &body);

  [[nodiscard]] std::shared_ptr<parsetree::ast::ClassDecl> BuildClassDecl(
      const std::shared_ptr<parsetree::ast::Modifiers> &modifiers,
      std::string name,
      const std::shared_ptr<parsetree::ast::ReferenceType> &super,
      const std::vector<std::shared_ptr<parsetree::ast::ReferenceType>>
          &interfaces,
      const std::vector<std::shared_ptr<parsetree::ast::Decl>> &classBodyDecls);

  [[nodiscard]] std::shared_ptr<parsetree::ast::FieldDecl>
  BuildFieldDecl(const std::shared_ptr<parsetree::ast::Modifiers> &modifiers,
                 const std::shared_ptr<parsetree::ast::Type> &type,
                 std::string name,
                 const std::shared_ptr<parsetree::ast::Expr> &init);

  [[nodiscard]] std::shared_ptr<parsetree::ast::MethodDecl> BuildMethodDecl(
      const std::shared_ptr<parsetree::ast::Modifiers> &modifiers,
      std::string name, const std::shared_ptr<parsetree::ast::Type> &returnType,
      const std::vector<std::shared_ptr<parsetree::ast::VarDecl>> &params,
      bool isConstructor,
      const std::shared_ptr<parsetree::ast::Block> &methodBody);

  [[nodiscard]] std::shared_ptr<parsetree::ast::VarDecl> BuildVarDecl(
      const std::shared_ptr<parsetree::ast::Type> &type, std::string name,
      const std::shared_ptr<parsetree::ast::ScopeID> &scopeID,
      const std::shared_ptr<parsetree::ast::Expr> &initializer = nullptr);

  [[nodiscard]] std::shared_ptr<parsetree::ast::InterfaceDecl>
  BuildInterfaceDecl(
      const std::shared_ptr<parsetree::ast::Modifiers> &modifiers,
      std::string name,
      const std::vector<std::shared_ptr<parsetree::ast::ReferenceType>>
          &extends,
      const std::vector<std::shared_ptr<parsetree::ast::Decl>>
          &interfaceBodyDecls);

  [[nodiscard]] std::shared_ptr<parsetree::ast::ReferenceType>
  BuildQualifiedIdentifier(const std::vector<std::string> &identifiers);

  [[nodiscard]] std::shared_ptr<parsetree::ast::BasicType>
  BuildBasicType(parsetree::ast::BasicType::Type basicType);

  [[nodiscard]] std::shared_ptr<parsetree::ast::BasicType>
  BuildBasicType(parsetree::Literal::Type basicType);

  [[nodiscard]] std::shared_ptr<parsetree::ast::ArrayType>
  BuildArrayType(const std::shared_ptr<parsetree::ast::Type> elemType);

  [[nodiscard]] std::shared_ptr<parsetree::ast::DeclStmt>
  BuildDeclStmt(const std::shared_ptr<parsetree::ast::VarDecl> decl);

  [[nodiscard]] std::shared_ptr<parsetree::ast::UnresolvedType>
  BuildUnresolvedType();

  void ClearLocalScope() noexcept {
    std::cout << "Clearing local scope" << std::endl;
    localDecls_.clear();
    localDeclStack_.clear();
    localScope_.clear();
    currentScope_ = parsetree::ast::ScopeID::New();
  }

  auto getAllDecls() const noexcept { return std::views::all(localDecls_); }

  bool AddToLocalScope(std::shared_ptr<parsetree::ast::VarDecl> decl) {
    const std::string name = decl->getName();
    if (localScope_.contains(name)) {
      return false;
    }
    localScope_.emplace(name);
    localDecls_.emplace_back(decl);
    localDeclStack_.emplace_back(decl);
    return true;
  }

  [[nodiscard]] std::size_t EnterNewScope() {
    currentScope_ = currentScope_->next(currentScope_);
    return localDeclStack_.size();
  }

  void ExitScope(std::size_t size) {
    for (auto i = localDeclStack_.size(); i > size; --i) {
      localScope_.erase(localDeclStack_[i - 1]->getName());
    }
    localDeclStack_.resize(size);
    if (currentScope_->parent() == nullptr) {
      throw std::runtime_error("Tried to exit root scope");
    }
    currentScope_->next(currentScope_->parent()->parent());
  }

  std::shared_ptr<parsetree::ast::ScopeID> NextScopeID() {
    std::cout << "NextScopeID of scope " << currentScope_->toString()
              << std::endl;
    currentScope_ = currentScope_->next(currentScope_->parent());
    return currentScope_;
  }

  std::shared_ptr<parsetree::ast::ScopeID> CurrentScopeID() const {
    return currentScope_;
  }

  std::shared_ptr<parsetree::ast::ScopeID> NextFieldScopeID() {
    currentFieldScope_ = currentFieldScope_->next(currentFieldScope_);
    return currentFieldScope_;
  }

  std::shared_ptr<parsetree::ast::ScopeID> CurrentFieldScopeID() const {
    return currentFieldScope_;
  }

  void ResetFieldScope() {
    currentFieldScope_ = parsetree::ast::ScopeID::New();
  }

private:
  std::vector<std::shared_ptr<parsetree::ast::VarDecl>> localDecls_;
  std::vector<std::shared_ptr<parsetree::ast::VarDecl>> localDeclStack_;
  std::unordered_set<std::string> localScope_;
  std::shared_ptr<parsetree::ast::UnresolvedType> objectType;
  std::shared_ptr<parsetree::ast::ScopeID> currentScope_;
  std::shared_ptr<parsetree::ast::ScopeID> currentFieldScope_;
};

} // namespace static_check