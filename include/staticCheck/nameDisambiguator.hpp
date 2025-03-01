#pragma once

#include "ast/ast.hpp"
#include "staticCheck/hierarchyCheck.hpp"
#include "staticCheck/typeLinker.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace static_check {

class NameDisambiguator {
public:
  NameDisambiguator(std::shared_ptr<parsetree::ast::ASTManager> astManager,
                    std::shared_ptr<TypeLinker> typeLinker,
                    std::shared_ptr<HierarchyCheck> hierarchyChecker)
      : astManager(astManager), typeLinker(typeLinker),
        hierarchyChecker(hierarchyChecker) {}

  void resolve();

  void disambiguate(
      std::vector<std::shared_ptr<parsetree::ast::MemberName>> &memberNames,
      bool isFieldInitializer = false, bool isAssignment = false);

private:
  // For scoping
  std::shared_ptr<parsetree::ast::ProgramDecl> currentProgram;
  std::shared_ptr<parsetree::ast::CodeBody> currentContext;
  std::shared_ptr<parsetree::ast::ClassDecl> currentClass;
  std::shared_ptr<parsetree::ast::FieldDecl> currentField;
  std::vector<
      std::unordered_map<std::string, std::shared_ptr<parsetree::ast::Decl>>>
      scopes;

  void enterProgram(std::shared_ptr<parsetree::ast::ProgramDecl> programDecl);
  void enterContext(std::shared_ptr<parsetree::ast::CodeBody> context);
  void enterScope();
  void leaveScope();
  void addToScope(std::string name, std::shared_ptr<parsetree::ast::Decl> decl);
  std::shared_ptr<parsetree::ast::Decl> findInScopes(const std::string &name);
  std::shared_ptr<parsetree::ast::Decl>
  findInCurrentClass(const std::string &name);
  std::shared_ptr<parsetree::ast::Decl>
  findInSuperClasses(const std::string &name);

  std::shared_ptr<parsetree::ast::ASTManager> astManager;
  std::shared_ptr<TypeLinker> typeLinker;
  std::shared_ptr<HierarchyCheck> hierarchyChecker;

  // AST traversal helpers
  void resolveAST(std::shared_ptr<parsetree::ast::AstNode> node);
  void resolveExpr(std::shared_ptr<parsetree::ast::Expr> expr);
  void resolveVarDecl(std::shared_ptr<parsetree::ast::VarDecl> decl);

  bool isLegalReference(bool isFieldInitialization, bool isAssignment);
};

} // namespace static_check