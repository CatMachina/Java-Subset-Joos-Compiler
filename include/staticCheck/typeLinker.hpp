#pragma once

#include "ast/ast.hpp"
#include "environment.hpp"
#include <memory>

namespace static_check {

class TypeLinker {
public:
  TypeLinker(std::unique_ptr<parsetree::ast::ASTManager> astManager)
      : astManager(std::move(astManager)) {
    rootPackage = std::make_unique<Package>();
    // add the default package
    rootPackage->addPackage(DEFAULT_PACKAGE_NAME);
    buildSymbolTable();
  }

  std::shared_ptr<Package> getRootPackage() { return rootPackage; }

  void initContext(std::shared_ptr<parsetree::ast::ProgramDecl> node);

  ////////////////////// Resolvers ////////////////////
  // Second pass
  void resolve();

  Package::packageChild
  resolveImport(std::shared_ptr<parsetree::ast::UnresolvedType> node);

  Package::packageChild
  resolveImport(const std::vector<std::string> &identifiers);

  void resolveType(std::shared_ptr<parsetree::ast::Type> type);

  Package::packageChild resolveSimpleName(const std::string &simpleName);

  std::unordered_map<std::string, Package::packageChild> &
  getContext(std::shared_ptr<parsetree::ast::ProgramDecl> node) {
    auto it = contextMap.find(node);
    if (it == contextMap.end()) {
      throw std::runtime_error("Could not find context for node");
    }
    return it->second;
  }

private:
  // First pass?
  void buildSymbolTable();
  // Second pass recursive helper
  void resolveAST(std::shared_ptr<parsetree::ast::AstNode> ast);

  std::unique_ptr<parsetree::ast::ASTManager> astManager;
  std::shared_ptr<Package> rootPackage; // no decl
  // std::unordered_map<std::string, Package::packageChild>
  //     context; // for each AST
  std::unordered_map<std::shared_ptr<parsetree::ast::ProgramDecl>,
                     std::unordered_map<std::string, Package::packageChild>>
      contextMap;
  std::shared_ptr<parsetree::ast::ProgramDecl> currentProgram; // for each AST

  static const std::string DEFAULT_PACKAGE_NAME;
};

} // namespace static_check
