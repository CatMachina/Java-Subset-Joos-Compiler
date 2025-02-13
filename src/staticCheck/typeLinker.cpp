#include "typeLinker.hpp"

namespace static_check {

//////////////////// Declaration Visitors ////////////////////

void TypeLinker::visitProgramDecl(parsetree::ast::ProgramDecl node) {
  enterScope();
  visitPackageDecl();
  for (auto import : imports) {
    visitImportDecl(import);
  }
  visitCodeBody();
  leaveScope()
}

void TypeLinker::visitCodeBody(parsetree::ast::CodeBody node) {
  switch (node->type()) {
    case ProgramDecl:
      visitProgramDecl();
      break;
    case ClassDecl:
      visitPackageDecl();
      break;
    case InterfaceDecl:
      visitInterfaceDecl();
      break;
    default:
      // not a codebody
  }
}

void TypeLinker::visitPackageDecl(parsetree::ast::PackageDecl node) {
  enterScope();
  registerDecl();
  leaveScope();
}

void TypeLinker::visitImportDecl(parsetree::ast::ImportDecl node) {
  resolve(node);
}

void TypeLinker::visitClassDecl(parsetree::ast::ClassDecl node) {
  enterScope();
  registerDecl();
  for (auto decl : classBodyDecls) {
    visitDecl();
  }
  leaveScope();
}

void TypeLinker::visitInterfaceDecl(parsetree::ast::InterfaceDecl node) {
  enterScope();
  registerDecl();
  for (auto decl : interfaceBodyDecls) {
    visitDecl();
  }
  leaveScope();
}

void TypeLinker::visitMethodDecl(parsetree::ast::MethodDecl node) {
  enterScope();
  registerDecl();
  for (auto param : params) {
     visitVarDecl(param);
  }
  visitBlock(methodBody);
  leaveScope();
}

void TypeLinker::visitFieldDecl(parsetree::ast::FieldDecl node) {
  registerDecl();
}

void TypeLinker::visitVarDecl(parsetree::ast::VarDecl node) {
  registerDecl();
}

//////////////////// Statement Visitors ////////////////////

void TypeLinker::visitBlock(parsetree::ast::Block node) {
  enterScope();
  for (auto statement : node->getStatements()) {
    visitStatement(statement);
  }
  leaveScope();
}

void TypeLinker::visitStatement(parsetree::ast::Stmt node) {
  // TODO: Different kinds of statements
}

void TypeLinker::visitDeclStmt(parsetree::ast::DeclStmt node) {
  visitVarDecl(node->getDecl());
}

//////////////////// Expression Visitors ////////////////////

// TODO
void TypeLinker::visitExpression(parsetree::ast::Expr node) {
  auto exprNodes = node->getExprNodes();
  auto lastExprNode = node->getLastExprNode();
  for (auto exprNode : exprNodes) {
    if (auto typeNode = std::dynamic_pointer_cast<TypeNode>(exprNode)) {
      // resolve
    } else if (auto arrayType = std::dynamic_pointer_cast<ArrayType>(exprNode)) {
      // resolve
    } else if (auto qid = std::dynamic_pointer_cast<QualifiedIdentifier>(exprNode)) {
      // resolve
    } else if (auto memberName = std::dynamic_pointer_cast<MemberName>(exprNode)) {
      // resolve
    }
  }
}

/*
  If we find a declaration for a name, we:
    1. Search for name in current environment
    2. If name already exists, ERROR
    3. Else insert name into environment
  To resolve a usage:
    1. Search innermost environment
    2. If not found, search recursively in enclosing environments
    3. If not found in any enclosing environments, ERROR
*/
void TypeLinker::registerDecl(std::shared_ptr<parsetree::ast::AstNode> astNode) {
  switch ( /* astNode type */ ) {
    case PackageDecl:
      envs.top()->addDecl({packageName, make_shared<Package>});
      globalEnv->addDecl({fullyQualifiedName, make_shared<Package});
      break;
    case ClassDecl:
      // TODO
    case InterfaceDecl:
      // TODO
    case MethodDecl:
      // TODO
    case FieldDecl:
      // TODO
    case VarDecl:
      // TODO
    default:
      // not a decl
  }
}

// First pass?
void TypeLinker::buildSymbolTable() {
  for (auto programDecl : astManager->getASTs()) {
    /*
    // ... Entering a new scope, push new env to stack ...
    Environment env;
    stack.push(env);
    */
    visitProgramDecl(programDecl);

    /*
    See visitPackageDecl and other methods
    // it should be UnresolvedType
    auto packageNode =
        std::dynamic_pointer_cast<parsetree::ast::UnresolvedType>(
            programDecl->getPackage());
    if (!packageNode) {
      throw std::runtime_error("Package should be unresolved type!");
    }

    // for symbol table build
    for (auto const &id : packageNode->getIdentifiers()) {
      // find id in symbol table. if not in we add and continue
      // else traverse into the next subpackage
    }
    // after building symbol table we can also check if program decl is in
    // symbol table if it is then duplicate! add program decl into symbol table

    // How to insert into env:
    // ... Create package object ...
      Package package;
      env.registerDecl({package, object});
    
    */
  }
}

/*
Simple names have no . in their names.
We traverse the namespaces in the following priority order:
  1. Enclosing class/interface
  2. Single-type imports (e.g. import a.b.c)
  3. Type in same package
  4. Import-on-demand package (e.g. import a.b.*)
1 can probably be checked through the first pass, with the environment stack
2 3 4 need to wait until we build global env?
*/
std::shared_ptr<Decl> TypeLinker::resolveSimpleName(const std::string &simpleName) {
  // TODO: repetitively going up the scope, need to refactor
  // TODO: use a vector to represent the stack since we want to go down
  // 1. Enclosing class/interface
  for (auto it = envs.rbegin(); it != envs.rend(); ++it) {
    auto result = it->simpleNamesToDecls.find(simpleName);
    if (result != it->simpleNamesToDecls.end()) {
      return result->second;
    }
  }
  // 2. Single-type imports (e.g. import a.b.c)
  // TODO
  for (auto it = envs.rbegin(); it != envs.rend(); ++it) {
    if (auto programDecl = std::dynamic_pointer_cast<ast::ProgramDecl>(it->getScope())) {
      auto imports = programDecl->getImport();
      for (auto importDecl : imports) {
        if (simpleName == importDecl->getIdentifiers().back()) {
          return resolveQualifiedName(importDecl->toString());
        }
      }
    }
  }

  // 3. Type in same package
  // TODO
  for (auto it = envs.rbegin(); it != envs.rend(); ++it) {
    std::string curPackageName = "";
    if (auto packageDecl = std::dynamic_pointer_cast<ast::PackageDecl>(it->getScope())) {
      curPackageName = packageDecl->getName();
    }
  }
  curPackageName = (curPackagename == "" ? DEFAULT_PACKAGE_NAME : curPackageName);
  auto typesInSamePackage = globalEnv->getDeclaredTypes(curPackageName);
  auto result = typesInSamePackage.find(packageName);
  if (result != typesInSamePackage.end()) {
    return result->second;
  }

  // 4. Import-on-demand package (e.g. import a.b.*)
  // TODO
  
}

/*
Qualified names always have . in their names (e.g. a.b.c.d). 
We simply traverse the sequence of names listed starting from the top-level name. 
Remark 10.2. If there is a usage c.d and a single-type import a.b.c, 
then a.b.c.d will never be resolved to c.d.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     
*/
std::shared_ptr<Decl> TypeLinker::resolveQualifiedName(
  std::shared_ptr<ast::QualifiedIdentifier> qualifiedIddentifier) {
  // TODO
  for (const auto &id : qualifiedIdentifier->getIdentifiers()) {
    
  }
}

void TypeLinker::resolve() {
  // How to resolve a usage
  // We need functions to peek down the stack of envs, to access enclosing
  // environments
  name = astNode->getName()
  // ... Try find name down the stack ... Remember the checks!
  // ... If succeeded, figure out fully qualified name and update global env ...
  // ... Maybe augment AST node with a pointer to the decl object?

  // Now visit classes/interface declarations.
  // ... Create class/interface object and update env & global env if permitted
  // ... //
  // ... Entering a new cope again, push new env to stack ...
  // ... Visit method/field declarations ...
  // ... Popping env from stack when you leave scope ...

  // Then just keep going ...
}

} // namespace static_check
