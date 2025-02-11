#include "staticCheck/envManager.hpp"
#include <string_view>

namespace parsetree::ast {

std::shared_ptr<ProgramDecl>
EnvManager::BuildProgramDecl(const std::shared_ptr<ast::ReferenceType> &package,
                             const std::vector<ast::ImportDecl> imports,
                             const std::shared_ptr<ast::CodeBody> &body) {
  return std::make_shared<ast::ProgramDecl>(package, imports, body);
}

std::shared_ptr<ast::ClassDecl> EnvManager::BuildClassDecl(
    const std::shared_ptr<ast::Modifiers> &modifiers, std::string_view name,
    const std::shared_ptr<ast::ReferenceType> &super,
    const std::vector<std::shared_ptr<ast::ReferenceType>> &interfaces,
    const std::vector<std::shared_ptr<ast::Decl>> &classBodyDecls) {
  return std::make_shared<ast::ClassDecl>(modifiers, name, super, interfaces,
                                          classBodyDecls);
}

std::shared_ptr<ast::FieldDecl>
EnvManager::BuildFieldDecl(const std::shared_ptr<ast::Modifiers> &modifiers,
                           const std::shared_ptr<ast::Type> &type,
                           std::string_view name,
                           const std::shared_ptr<ast::Expr> &init) {
  return std::make_shared<ast::FieldDecl>(modifiers, type, name, init);
}

std::shared_ptr<MethodDecl> EnvManager::BuildMethodDecl(
    const std::shared_ptr<ast::Modifiers> &modifiers, std::string_view name,
    const std::shared_ptr<ast::Type> &returnType,
    const std::vector<std::shared_ptr<ast::VarDecl>> &params,
    bool isConstructor, const std::shared_ptr<ast::Block> &methodBody) {
  std::shared_ptr<MethodDecl> methodDecl = std::make_shared<ast::MethodDecl>(
      modifiers, name, returnType, params, isConstructor, methodBody);
  methodDecl->addDecls(getAllDecls());
  return methodDecl;
}

std::shared_ptr<ast::VarDecl>
EnvManager::BuildVarDecl(const std::shared_ptr<ast::Type> &type,
                         std::string_view name,
                         const std::shared_ptr<ast::Expr> &initializer) {
  std::shared_ptr<ast::VarDecl> varDecl =
      std::make_shared<ast::VarDecl>(type, name, initializer);
  if (!AddToLocalScope(varDecl)) {
    throw std::runtime_error("Variable " + std::string(name) +
                             " already declared in this scope.");
  }
  return varDecl;
}

std::shared_ptr<ast::UnresolvedType> EnvManager::BuildUnresolvedType() {
  return std::make_shared<ast::UnresolvedType>();
}

std::shared_ptr<ast::DeclStmt>
EnvManager::BuildDeclStmt(const std::shared_ptr<ast::VarDecl> decl) {
  return std::make_shared<ast::DeclStmt>(decl);
}

std::shared_ptr<ast::InterfaceDecl> EnvManager::BuildInterfaceDecl(
    const std::shared_ptr<ast::Modifiers> &modifiers, std::string_view name,
    const std::vector<std::shared_ptr<ast::ReferenceType>> &extends,
    const std::vector<std::shared_ptr<ast::Decl>> &interfaceBodyDecls) {
  return std::make_shared<ast::InterfaceDecl>(modifiers, name, extends,
                                              interfaceBodyDecls);
}

std::shared_ptr<ast::BasicType>
EnvManager::BuildBasicType(ast::BasicType::Type basicType) {
  return std::make_shared<ast::BasicType>(basicType);
}

std::shared_ptr<ast::ArrayType>
EnvManager::BuildArrayType(const std::shared_ptr<ast::Type> elemType) {
  return std::make_shared<ast::ArrayType>(elemType);
}

} // namespace parsetree::ast
