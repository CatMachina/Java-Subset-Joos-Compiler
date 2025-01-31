#include "ast/astNode.hpp"

namespace ast {

ClassDecl::ClassDecl(std::shared_ptr<Modifiers> modifiers,
                     std::shared_ptr<QualifiedIdentifier> superClass,
                     std::vector<std::shared_ptr<Decl>> classBodyDecls) {
  // check class modifiers
  // seperate fields methods and constructors
  // no constructor is invalide
  // ...
}

InterfaceDecl::InterfaceDecl(
    std::shared_ptr<Modifiers> modifiers, std::string_view name,
    std::vector<std::shared_ptr<QualifiedIdentifier>> extendsInterfaces,
    std::vector<std::shared_ptr<Decl>> interfaceBody) {

  // check modifiers
  // interface can only have abstract methods
  // ...
}

MethodDecl::MethodDecl(std::shared_ptr<Modifiers> methodModifiers,
                       std::string_view name, std::shared_ptr<Type> returnType,
                       std::vector<std::shared_ptr<VarDecl>> params,
                       bool isConstructor, std::shared_ptr<Stmt> methodBody) {
  // check method modifiers
  // ...
}

} // namespace ast