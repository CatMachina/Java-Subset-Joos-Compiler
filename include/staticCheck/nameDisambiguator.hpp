#pragma once

#include "ast/ast.hpp"
#include "staticCheck/envManager.hpp"
#include <memory>

namespace static_check {

// Q: Do we need this? 
// I am thinking about decorating AST nodes (storing pointers to Decl objects)
// Like how we did type linking
// Or are we having a new implementation?
class ExprName {
public:
  enum class Type {
    PackageName,
    TypeName,
    ExpressionName,
    MethodName,
    SingleAmbiguousName
  }

  ExprName(Type type,
           std::shared_ptr<parsetree::ast::MemberName> node) : type(type),
      node(node) {}

  void reclassify(Type type, std::shared_ptr<parsetree::ast::MemberName> node) {
    this->type = type;
    this->node = node;
  }

  // Getters
  std::shared_ptr<parsetree::ast::MemberName> getNode() { return node; }
  Type getType() { return type; }

private:
  Type type;
  std::shared_ptr<parsetree::ast::MemberName> node;
};

class NameDisambiguator {
public:
  NameDisambiguator(std::unique_ptr<parsetree::ast::ASTManager> astManager)
      : astManager(std::move(astManager)) {}

  std::shared_ptr<ExprName>
  disambiguate(std::shared_ptr<parsetree::ast::MemberName> node);

  void resolve() {
    for (auto ast : astManager->getASTs()) {
      // Q: resolveAST?
      resolveRecursive(ast);
    }
  }

private:
  void resolveAST(std::shared_ptr<parsetree::ast::AstNode> node);

  void beginProgram(std::shared_ptr<parsetree::ast::ProgramDecl> programDecl) {
    currentProgramDecl = programDecl;
  }
  void beginContext((std::shared_ptr<parsetree::ast::CodeBody> ctx) {
    currentContext = ctx; }

  std::unique_ptr<parsetree::ast::ASTManager> astManager;
  // hard to reuse envManager? this is my bad of not implementing it nicely
  // std::unique_ptr<EnvManager> envManager;
  // Q: Can we reuse it for local scopes?
  
  std::shared_ptr<parsetree::ast::ProgramDecl> currentProgramDecl;
  std::shared_ptr<parsetree::ast::CodeBody> currentContext;
};

} // namespace static_check