#pragma once

#include "ast/ast.hpp"
#include "parseTree/parseTree.hpp"

namespace parsetree::ast {

class EnvManager {
public:
  [[nodiscard]] std::shared_ptr<ast::ProgramDecl>
  BuildProgramDecl(const std::shared_ptr<ast::QualifiedIdentifier> &package,
                   std::vector<ast::ImportDecl> imports,
                   const std::shared_ptr<ast::CodeBody> &body);

  [[nodiscard]] std::shared_ptr<ast::ClassDecl> BuildClassDecl(
      const std::shared_ptr<ast::Modifiers> &modifiers, std::string_view name,
      const std::shared_ptr<ast::QualifiedIdentifier> &super,
      const std::vector<std::shared_ptr<ast::QualifiedIdentifier>> &interfaces,
      const std::vector<std::shared_ptr<ast::Decl>> &classBodyDecls);

  [[nodiscard]] std::shared_ptr<ast::FieldDecl>
  BuildFieldDecl(const std::shared_ptr<ast::Modifiers> &modifiers,
                 const std::shared_ptr<ast::Type> &type, std::string_view name,
                 const std::shared_ptr<ast::Expr> &init);

  [[nodiscard]] std::shared_ptr<ast::MethodDecl> BuildMethodDecl(
      const std::shared_ptr<ast::Modifiers> &modifiers, std::string_view name,
      const std::shared_ptr<ast::Type> &returnType,
      const std::vector<std::shared_ptr<ast::VarDecl>> &params,
      bool isConstructor, const std::shared_ptr<ast::Block> &methodBody);

  [[nodiscard]] std::shared_ptr<ast::VarDecl>
  BuildVarDecl(const std::shared_ptr<ast::Type> &type, std::string_view name);

  [[nodiscard]] std::shared_ptr<ast::InterfaceDecl> BuildInterfaceDecl(
      const std::shared_ptr<ast::Modifiers> &modifiers, std::string_view name,
      const std::vector<std::shared_ptr<ast::QualifiedIdentifier>> &extends,
      const std::vector<std::shared_ptr<ast::Decl>> &interfaceBodyDecls);

  [[nodiscard]] std::shared_ptr<ast::QualifiedIdentifier>
  BuildQualifiedIdentifier(const std::vector<std::string> &identifiers);

  [[nodiscard]] std::shared_ptr<ast::BasicType>
  BuildBasicType(ast::BasicType::Type basicType);

  [[nodiscard]] std::shared_ptr<ast::ArrayType>
  BuildArrayType(const std::shared_ptr<ast::Type> &elemType);

  void ClearLocalScope() noexcept {
    localDecls_.clear();
    localDeclStack_.clear();
    localScope_.clear();
  }

  const std::vector<std::shared_ptr<VarDecl>> &getAllDecls() const noexcept {
    return localDecls_;
  }

  bool AddToLocalScope(std::shared_ptr<VarDecl> decl) {
    const std::string name = decl->getName();
    if (localScope_.contains(name)) {
      return false;
    }
    localScope_.emplace(name);
    localDecls_.emplace_back(decl);
    localDeclStack_.emplace_back(decl);
    return true;
  }

  [[nodiscard]] std::size_t EnterNewScope() const noexcept {
    return localDeclStack_.size();
  }

  void ExitScope(std::size_t size) {
    for (auto i = localDeclStack_.size(); i > size; --i) {
      localScope_.erase(localDeclStack_[i - 1]->getName());
    }
    localDeclStack_.resize(size);
  }

private:
  std::vector<std::shared_ptr<VarDecl>> localDecls_;
  std::vector<std::shared_ptr<VarDecl>> localDeclStack_;
  std::unordered_set<std::string> localScope_;
};

} // namespace parsetree::ast