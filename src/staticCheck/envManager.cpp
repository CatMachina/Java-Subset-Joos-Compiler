#include "staticCheck/envManager.hpp"
#include <string_view>

namespace static_check {

std::shared_ptr<parsetree::ast::ProgramDecl> EnvManager::BuildProgramDecl(
    const std::shared_ptr<parsetree::ast::ReferenceType> &package,
    const std::vector<std::shared_ptr<parsetree::ast::ImportDecl>> imports,
    const std::shared_ptr<parsetree::ast::CodeBody> &body) {
  return std::make_shared<parsetree::ast::ProgramDecl>(package, imports, body);
}

std::shared_ptr<parsetree::ast::ClassDecl> EnvManager::BuildClassDecl(
    const std::shared_ptr<parsetree::ast::Modifiers> &modifiers,
    std::string_view name,
    const std::shared_ptr<parsetree::ast::ReferenceType> &super,
    const std::vector<std::shared_ptr<parsetree::ast::ReferenceType>>
        &interfaces,
    const std::vector<std::shared_ptr<parsetree::ast::Decl>> &classBodyDecls) {
  return std::make_shared<parsetree::ast::ClassDecl>(
      modifiers, name, super, interfaces, classBodyDecls);
}

std::shared_ptr<parsetree::ast::FieldDecl> EnvManager::BuildFieldDecl(
    const std::shared_ptr<parsetree::ast::Modifiers> &modifiers,
    const std::shared_ptr<parsetree::ast::Type> &type, std::string_view name,
    const std::shared_ptr<parsetree::ast::Expr> &init) {
  return std::make_shared<parsetree::ast::FieldDecl>(modifiers, type, name,
                                                     init);
}

std::shared_ptr<parsetree::ast::MethodDecl> EnvManager::BuildMethodDecl(
    const std::shared_ptr<parsetree::ast::Modifiers> &modifiers,
    std::string_view name,
    const std::shared_ptr<parsetree::ast::Type> &returnType,
    const std::vector<std::shared_ptr<parsetree::ast::VarDecl>> &params,
    bool isConstructor,
    const std::shared_ptr<parsetree::ast::Block> &methodBody) {
  std::shared_ptr<parsetree::ast::MethodDecl> methodDecl =
      std::make_shared<parsetree::ast::MethodDecl>(
          modifiers, name, returnType, params, isConstructor, methodBody);
  methodDecl->addDecls(getAllDecls());
  return methodDecl;
}

std::shared_ptr<parsetree::ast::VarDecl> EnvManager::BuildVarDecl(
    const std::shared_ptr<parsetree::ast::Type> &type, std::string_view name,
    const std::shared_ptr<parsetree::ast::Expr> &initializer) {
  std::shared_ptr<parsetree::ast::VarDecl> varDecl =
      std::make_shared<parsetree::ast::VarDecl>(type, name, initializer);
  if (!AddToLocalScope(varDecl)) {
    throw std::runtime_error("Variable " + std::string(name) +
                             " already declared in this scope.");
  }
  return varDecl;
}

std::shared_ptr<parsetree::ast::UnresolvedType>
EnvManager::BuildUnresolvedType() {
  return std::make_shared<parsetree::ast::UnresolvedType>();
}

std::shared_ptr<parsetree::ast::DeclStmt>
EnvManager::BuildDeclStmt(const std::shared_ptr<parsetree::ast::VarDecl> decl) {
  return std::make_shared<parsetree::ast::DeclStmt>(decl);
}

std::shared_ptr<parsetree::ast::InterfaceDecl> EnvManager::BuildInterfaceDecl(
    const std::shared_ptr<parsetree::ast::Modifiers> &modifiers,
    std::string_view name,
    const std::vector<std::shared_ptr<parsetree::ast::ReferenceType>> &extends,
    const std::vector<std::shared_ptr<parsetree::ast::Decl>>
        &interfaceBodyDecls) {
  return std::make_shared<parsetree::ast::InterfaceDecl>(
      modifiers, name, extends, interfaceBodyDecls);
}

std::shared_ptr<parsetree::ast::BasicType>
EnvManager::BuildBasicType(parsetree::ast::BasicType::Type basicType) {
  return std::make_shared<parsetree::ast::BasicType>(basicType);
}

std::shared_ptr<parsetree::ast::ArrayType> EnvManager::BuildArrayType(
    const std::shared_ptr<parsetree::ast::Type> elemType) {
  return std::make_shared<parsetree::ast::ArrayType>(elemType);
}

} // namespace static_check
