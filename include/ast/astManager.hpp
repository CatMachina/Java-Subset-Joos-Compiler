#pragma once

#include "ast/astNode.hpp"

namespace parsetree::ast {

class ASTManager {
private:
  std::vector<std::shared_ptr<parsetree::ast::ProgramDecl>> asts;

public:
  void addAST(std::shared_ptr<ProgramDecl> ast) { asts.push_back(ast); }

  const std::vector<std::shared_ptr<parsetree::ast::ProgramDecl>> &
  getASTs() const {
    return asts;
  }

  struct {
    std::shared_ptr<ClassDecl> Array;
    std::shared_ptr<ClassDecl> Arrays;
    std::shared_ptr<ClassDecl> Boolean;
    std::shared_ptr<ClassDecl> Byte;
    std::shared_ptr<ClassDecl> Character;
    std::shared_ptr<ClassDecl> Class;
    std::shared_ptr<InterfaceDecl> Cloneable;
    std::shared_ptr<ClassDecl> Integer;
    std::shared_ptr<ClassDecl> Number;
    std::shared_ptr<ClassDecl> Object;
    std::shared_ptr<ClassDecl> Short;
    std::shared_ptr<ClassDecl> String;
    std::shared_ptr<ClassDecl> System;
    std::shared_ptr<InterfaceDecl> Serializable;
  } java_lang;

  std::vector<std::shared_ptr<parsetree::ast::Decl>> allDecls;
};

} // namespace parsetree::ast
