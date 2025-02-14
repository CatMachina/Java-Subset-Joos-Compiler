#include "staticCheck/typeLinker.hpp"

namespace static_check {

const std::string TypeLinker::DEFAULT_PACKAGE_NAME = "?";

// First pass
void TypeLinker::buildSymbolTable() {
  for (auto programDecl : astManager->getASTs()) {
    auto package = std::dynamic_pointer_cast<parsetree::ast::UnresolvedType>(
        programDecl->getPackage());
    if (!package) {
      throw std::runtime_error("Package not Unresolved Type");
    }

    // Traverse the package name to find the leaf package.
    std::shared_ptr<Package> currentPackage = rootPackage;
    for (const auto &id : package->getIdentifiers()) {
      // If the subpackage name is not in the symbol table, add it
      // and continue to the next one.
      if (currentPackage->children.find(id) == currentPackage->children.end()) {
        auto newPackage = std::make_shared<Package>(id);
        currentPackage->children[id] = newPackage;
        currentPackage = newPackage;
        continue;
      }
      // The subpackage name already exists.
      auto const &child = currentPackage->children[id];
      // Get the next subpackage.
      currentPackage = std::get<std::shared_ptr<Package>>(child);
    }
    if (programDecl->isDefaultPackage()) {
      currentPackage = std::get<std::shared_ptr<Package>>(
          currentPackage->children[DEFAULT_PACKAGE_NAME]);
    }

    // now we are at leaf package
    if (!programDecl->getBody())
      continue;
    // check duplicate
    auto body =
        std::dynamic_pointer_cast<parsetree::ast::Decl>(programDecl->getBody());
    if (!body) {
      throw std::runtime_error("Body not Decl");
    }
    if (currentPackage->children.find(body->getName()) !=
        currentPackage->children.end()) {
      throw std::runtime_error("Duplicate declaration");
    }
    // add to symbol table
    currentPackage->children[body->getName()] = std::make_shared<Body>(body);
  }
}

void TypeLinker::resolveAST(std::shared_ptr<parsetree::ast::AstNode> node) {
  if (!node)
    throw std::runtime_error("Node is null when resolving AST");
  for (auto child : node->getChildren()) {
    if (!child)
      continue;

    // Case: Type
    if (auto type = std::dynamic_pointer_cast<parsetree::ast::Type>(child)) {
      // if not resolved, resolve.
      if (!type->isResolved()) {
        type->resolve(*this);
      }
    }
    // Case: New AST root
    else if (auto ast = std::dynamic_pointer_cast<parsetree::ast::ProgramDecl>(
                 child)) {
      // resolve body
      initContext(ast);
      resolveAST(ast->getBody());
    }
    // Case: regular code
    else {
      resolveAST(child);
    }
  }
}

// Second pass
void TypeLinker::resolve() {
  for (auto ast : astManager->getASTs()) {
    resolveAST(ast);
  }
}

// //////////////////// Helpers ////////////////////

void TypeLinker::initContext(
    std::shared_ptr<parsetree::ast::ProgramDecl> node) {
  // clear the current context
  context.clear();
  auto package = std::dynamic_pointer_cast<parsetree::ast::UnresolvedType>(
      node->getPackage());
  if (!package) {
    throw std::runtime_error("Package not Unresolved Type in initContext");
  }

  /*
  Shadowing Declarations
  https://web.archive.org/web/20120105104400/http://java.sun.com/docs/books/jls/second_edition/html/names.doc.html#34133
  Hence currently I am doing in this sequence:
  1. Import-on-Demand Declarations (import pkg.*)
  2. Add Package Declarations
  3. Add Decl from the Same Package (Different ASTs)
  4. Single-Type Imports (import pkg.ClassName)
  5. All Decl from the Current AST
  */

  // Step 1:
  for (auto impt : node->getImports()) {
    if (!impt.hasStar())
      continue;                            // not on demand
    auto imptPkg = resolveImport(package); // garuanteed to be not null

    // resolved on demand package should be package
    if (!std::holds_alternative<std::shared_ptr<Package>>(imptPkg.value())) {
      throw std::runtime_error("Failed to resolve import-on-demand to package");
    }
    auto pkg = std::get<std::shared_ptr<Package>>(imptPkg.value());

    // add all decl of package to context
    for (auto &tuple : pkg->children) {
      auto key = tuple.first;
      auto value = tuple.second;
      // we only add decl
      if (!std::holds_alternative<std::shared_ptr<Decl>>(value))
        continue;
      auto decl = std::get<std::shared_ptr<Decl>>(value);
      if (context.find(key) != context.end()) {
        // already in context, shadowing?
        // need more thinking
        continue;
      }
      context[key] = Package::packageChild{decl};
    }
  }

  // Step 2:
}

Package::packageChild TypeLinker::resolveImport(
    std::shared_ptr<parsetree::ast::UnresolvedType> node) {
  // Base cases
  if (!node)
    throw std::runtime_error("Node is null when resolving import");
  if (node->isResolved())
    throw std::runtime_error("unresolved type should not be resolved yet");
  if (node->getIdentifiers().size() == 0) {
    return rootPackage->[DEFAULT_PACKAGE_NAME];
  }

  auto currentPkg = rootPackage;
  for (auto &id : node->getIdentifiers()) {
    if (std::holds_alternative<std::shared_ptr<Decl>>(currentPkg)) {
      throw std::runtime_error("internal node should not be decl");
    }
    auto pkg = std::get<std::shared_ptr<Package>>(currentPkg);
    if (pkg->children.find(id) == pkg->children.end()) {
      throw std::runtime_error("Could not resolve " + id +
                               " since this is not found");
    }
    currentPkg = pkg->children.at(id);
  }
  // return the leaf nodes
  return currentPkg;
}

// // Find the fully qualified name given the current scope.
// std::string TypeLinker::getQualifiedName(const std::string &name) {
//   if (envs.empty()) {
//     return name;
//   }

//   std::string qualifiedName = name;
//   bool foundPackage = false;
//   for (auto it = envs.rbegin(); it != envs.rend(); ++it) {
//     auto node = it->getScope();
//     std::string newPart;
//     if (auto block = std::dynamic_pointer_cast<parsetree::ast::Block>(node))
//     {
//       continue;
//     } else if (auto methodDecl =
//     std::dynamic_pointer_cast<parsetree::ast::MethodDecl>(node)) {
//       newPart = methodDecl->getName();
//     } else if (auto classDecl =
//     std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(node)) {
//       newPart = classDecl->getName();
//     } else if (auto interfaceDecl =
//     std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(node)) {
//       newPart = interfaceDecl->getName();
//     } else if (auto packageDecl =
//     std::dynamic_pointer_cast<parsetree::ast::PackageDecl>) {
//       foundPackage = true;
//       newPart = packageDecl->getName();
//     }
//     qualifiedName = newPart + "." + qualifiedName;
//   }
//   if (!foundPackage) {
//     qualifiedName = DEFAULT_PACKAGE_NAME + "." + qualifiedName;
//   }
//   return qualifiedName;
// }

// /*
//   If we find a declaration for a name, we:
//     1. Search for name in current environment
//     2. If name already exists, ERROR
//     3. Else insert name into environment
//   To resolve a usage:
//     1. Search innermost environment
//     2. If not found, search recursively in enclosing environments
//     3. If not found in any enclosing environments, ERROR
// */
// void TypeLinker::registerDecl(const std::shared_ptr<parsetree::ast::AstNode>
// astNode) {
//   if (auto packageDecl =
//   std::dynamic_pointer_cast<parsetree::ast::PackageDecl>(astNode)) {
//     auto package = std::make_shared<Package>();
//     std::string qualifiedName = getQualifiedName(packageName);
//     envs.top()->addDecl({packageName, package});
//     globalEnv->addDecl({qualifiedName, package});
//   } else if (auto classDecl =
//   std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(astNode)) {
//     auto cls = std::make_shared<Class>();
//     envs.top()->addDecl({className, cls});
//     std::string qualifiedName = getQualifiedName(className);
//     globalEnv->addDecl({qualifiedName, cls});
//   } else if (auto interfaceDecl =
//   std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(astNode)) {
//     auto interface = std::make_shared<Interface>();
//     envs.top()->addDecl({interfaceName, interface});
//     std::string qualifiedName = getQualifiedName(interfaceName);
//     globalEnv->addDecl({qualifiedName, interface});
//   } else if (auto methodDecl =
//   std::dynamic_pointer_cast<parsetree::ast::MethodDecl>(astNode)) {
//     auto method = std::make_shared<Method>();
//     envs.top()->addDecl({methodName, method});
//     std::string qualifiedName = getQualifiedName(methodName);
//     globalEnv->addDecl({qualifiedName, method});
//   } else if (auto fieldDecl =
//   std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(astNode)) {
//     auto field = std::make_shared<Field>();
//     envs.top()->addDecl({fieldName, field});
//     std::string qualifiedName = getQualifiedName(fieldName);
//     globalEnv->addDecl({qualifiedName, field});
//   } else if (auto varDecl =
//   std::dynamic_pointer_cast<parsetree::ast::VarDecl>(astNode)) {
//     auto variable = std::make_shared<Variable>();
//     envs.top()->addDecl({varName, variable});
//   } else {
//       // Not a declaration node, ignore
//   }
// }

// /*
// Simple names have no . in their names.
// We traverse the namespaces in the following priority order:
//   1. Enclosing class/interface
//   2. Single-type imports (e.g. import a.b.c)
//   3. Type in same package
//   4. Import-on-demand package (e.g. import a.b.*)
// 1 can probably be checked through the first pass, with the environment stack
// 2 3 4 need to wait until we build global env?
// */
// std::shared_ptr<Decl> TypeLinker::resolveSimpleName(const std::string
// &simpleName) {
//   // TODO: repetitively going up the scope, need to refactor
//   // TODO: use a vector to represent the stack since we want to go down
//   // 1. Enclosing class/interface
//   for (auto it = envs.rbegin(); it != envs.rend(); ++it) {
//     auto result = it->simpleNamesToDecls.find(simpleName);
//     if (result != it->simpleNamesToDecls.end()) {
//       return result->second;
//     }
//   }
//   // 2. Single-type imports (e.g. import a.b.c)
//   // TODO
//   for (auto it = envs.rbegin(); it != envs.rend(); ++it) {
//     if (auto programDecl =
//     std::dynamic_pointer_cast<ast::ProgramDecl>(it->getScope())) {
//       auto imports = programDecl->getImport();
//       for (auto importDecl : imports) {
//         if (simpleName == importDecl->getIdentifiers().back()) {
//           return resolveQualifiedName(importDecl->toString());
//         }
//       }
//     }
//   }

//   // 3. Type in same package
//   // TODO
//   for (auto it = envs.rbegin(); it != envs.rend(); ++it) {
//     std::string curPackageName;
//     if (auto packageDecl =
//     std::dynamic_pointer_cast<ast::PackageDecl>(it->getScope())) {
//       curPackageName = packageDecl->getName();
//     }
//   }
//   curPackageName = (curPackagename == "" ? DEFAULT_PACKAGE_NAME :
//   curPackageName); auto typesInSamePackage =
//   globalEnv->getDeclaredTypes(curPackageName); auto result =
//   typesInSamePackage.find(packageName); if (result !=
//   typesInSamePackage.end()) {
//     return result->second;
//   }

//   // 4. Import-on-demand package (e.g. import a.b.*)
//   // TODO

// }

// /*
// Qualified names always have . in their names (e.g. a.b.c.d).
// We simply traverse the sequence of names listed starting from the top-level
// name. Remark 10.2. If there is a usage c.d and a single-type import a.b.c,
// then a.b.c.d will never be resolved to c.d.
// */
// std::shared_ptr<Decl> TypeLinker::resolveQualifiedName(
//   std::shared_ptr<ast::QualifiedIdentifier> qualifiedIdentifier) {
//   // TODO
//   for (const auto &id : qualifiedIdentifier->getIdentifiers()) {

//   }
// }

// //////////////////// Declaration Visitors ////////////////////

// void TypeLinker::visitProgramDecl(const
// std::shared_ptr<parsetree::ast::ProgramDecl> node) {
//   enterScope();
//   visitPackageDecl();
//   for (auto import : imports) {
//     visitImportDecl(import);
//   }
//   visitCodeBody();
//   leaveScope()
// }

// void TypeLinker::visitCodeBody(const
// std::shared_ptr<parsetree::ast::CodeBody> node) {
//   switch (node->type()) {
//     case ProgramDecl:
//       visitProgramDecl();
//       break;
//     case ClassDecl:
//       visitPackageDecl();
//       break;
//     case InterfaceDecl:
//       visitInterfaceDecl();
//       break;
//     default:
//       // not a codebody
//   }
// }

// void TypeLinker::visitPackageDecl(const
// std::shared_ptr<parsetree::ast::PackageDecl> node) {
//   enterScope();
//   registerDecl(node);
//   leaveScope();
// }

// void TypeLinker::visitImportDecl(const
// std::shared_ptr<parsetree::ast::ImportDecl> node) {
//   resolve(node);
// }

// void TypeLinker::visitClassDecl(const
// std::shared_ptr<parsetree::ast::ClassDecl> node) {
//   enterScope();
//   registerDecl(node);
//   for (auto decl : classBodyDecls) {
//     visitDecl();
//   }
//   leaveScope();
// }

// void TypeLinker::visitInterfaceDecl(const
// std::shared_ptr<parsetree::ast::InterfaceDecl> node) {
//   enterScope();
//   registerDecl(node);
//   for (auto decl : interfaceBodyDecls) {
//     visitDecl();
//   }
//   leaveScope();
// }

// void TypeLinker::visitMethodDecl(const
// std::shared_ptr<parsetree::ast::MethodDecl> node) {
//   enterScope();
//   registerDecl(node);
//   for (auto param : params) {
//      visitVarDecl(param);
//   }
//   visitBlock(methodBody);
//   leaveScope();
// }

// void TypeLinker::visitFieldDecl(const
// std::shared_ptr<parsetree::ast::FieldDecl> node) {
//   registerDecl(node);
// }

// void TypeLinker::visitVarDecl(const std::shared_ptr<parsetree::ast::VarDecl>
// node) {
//   registerDecl(node);
// }

// //////////////////// Statement Visitors ////////////////////

// void TypeLinker::visitBlock(const std::shared_ptr<parsetree::ast::Block>
// node) {
//   enterScope();
//   for (auto statement : node->getStatements()) {
//     visitStatement(statement);
//   }
//   leaveScope();
// }

// void TypeLinker::visitStatement(const std::shared_ptr<parsetree::ast::Stmt>
// node) {
//   // TODO: Different kinds of statements
// }

// void TypeLinker::visitDeclStmt(const
// std::shared_ptr<parsetree::ast::DeclStmt> node) {
//   visitVarDecl(node->getDecl());
// }

// //////////////////// Expression Visitors ////////////////////

// // TODO
// void TypeLinker::visitExpression(const std::shared_ptr<parsetree::ast::Expr>
// node) {
//   auto exprNodes = node->getExprNodes();
//   auto lastExprNode = node->getLastExprNode();
//   for (auto exprNode : exprNodes) {
//     if (auto typeNode =
//     std::dynamic_pointer_cast<parsetree::ast::TypeNode>(exprNode)) {
//       // resolve
//     } else if (auto arrayType =
//     std::dynamic_pointer_cast<ast::ArrayType>(exprNode)) {
//       // resolve
//     } else if (auto qid =
//     std::dynamic_pointer_cast<QualifiedIdentifier>(exprNode)) {
//       // resolve
//     } else if (auto memberName =
//     std::dynamic_pointer_cast<MemberName>(exprNode)) {
//       // resolve
//     }
//   }
// }

} // namespace static_check
