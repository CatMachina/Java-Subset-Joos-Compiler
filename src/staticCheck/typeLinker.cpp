#include "staticCheck/typeLinker.hpp"

namespace static_check {

const std::string TypeLinker::DEFAULT_PACKAGE_NAME = "?";

// First pass
void TypeLinker::buildSymbolTable() {
  for (auto programDecl : astManager->getASTs()) {
    auto package = std::dynamic_pointer_cast<parsetree::ast::UnresolvedType>(
        programDecl->getPackage());
    if (!package) {
      throw std::runtime_error(
          "Package not Unresolved Type at build symbol table");
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
      if (std::holds_alternative<std::shared_ptr<Decl>>(child)) {
        throw std::runtime_error("Prefix include a decl for at " + id);
      }
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
      throw std::runtime_error("Duplicate declaration at " + body->getName());
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
        resolveType(type);
      }
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
    initContext(ast);
    resolveAST(ast->getBody());
  }
}

// //////////////////// Helpers ////////////////////

void TypeLinker::initContext(
    std::shared_ptr<parsetree::ast::ProgramDecl> node) {
  // clear the current context and single type imports
  // context.clear();
  auto &context = contextMap[node];
  currentProgram = node;
  // singleTypeImports.clear();
  auto packageAstNode =
      std::dynamic_pointer_cast<parsetree::ast::UnresolvedType>(
          node->getPackage());
  if (!packageAstNode) {
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

  // Step 1: Import-on-Demand Declarations (import pkg.*)
  for (auto impt : node->getImports()) {
    if (!impt->hasStar())
      continue; // not on demand
    auto imptPkg =
        resolveImport(std::dynamic_pointer_cast<parsetree::ast::UnresolvedType>(
            impt->getQualifiedIdentifier()));

    // resolved on demand package should be package
    if (!std::holds_alternative<std::shared_ptr<Package>>(imptPkg)) {
      throw std::runtime_error("Failed to resolve import-on-demand to package");
    }
    auto pkg = std::get<std::shared_ptr<Package>>(imptPkg);

    // add all decl of package to context
    for (auto &tuple : pkg->children) {
      auto key = tuple.first;
      auto value = tuple.second;
      // we only add decl
      if (!std::holds_alternative<std::shared_ptr<Decl>>(value))
        continue;
      auto decl = std::get<std::shared_ptr<Decl>>(value);
      if (context.find(key) != context.end()) {
        // already in context by another on demand import
        // I think this is an ambiguous case
        if (std::holds_alternative<std::shared_ptr<Decl>>(context[key]) &&
            std::get<std::shared_ptr<Decl>>(context[key]) == decl) {
          // same import, all good
          continue;
        }
        // different import from different package
        // ambiguous?
        context[key] = Package::packageChild{nullptr};
        continue;
      }
      context[key] = Package::packageChild{decl};
    }
  }

  // Step 2: Add Package Declarations
  for (auto &tuple : rootPackage->children) {
    // only import package
    if (!std::holds_alternative<std::shared_ptr<Package>>(tuple.second))
      continue;
    if (context.find(tuple.first) == context.end()) {
      // only add package not shadowed by on demand import
      context[tuple.first] = Package::packageChild{
          std::get<std::shared_ptr<Package>>(tuple.second)};
    }
  }

  // Step 3: Add Decl from the Same Package (Different ASTs)
  // info already in symbol table, no need another loop of ast
  auto currentPackage = resolveImport(packageAstNode);
  if (!std::holds_alternative<std::shared_ptr<Package>>(currentPackage)) {
    throw std::runtime_error("Failed to get current package");
  }
  for (auto &pair :
       std::get<std::shared_ptr<Package>>(currentPackage)->children) {
    auto key = pair.first;
    auto value = pair.second;
    // we only add decl
    if (!std::holds_alternative<std::shared_ptr<Decl>>(value))
      continue;
    auto decl = std::get<std::shared_ptr<Decl>>(value);
    context[key] = Package::packageChild{decl};
  }

  // Step 4: Single-Type Imports (import pkg.ClassName)
  for (auto impt : node->getImports()) {
    if (impt->hasStar()) {
      continue; // skip on-demand imports
    }
    auto imptType =
        resolveImport(std::dynamic_pointer_cast<parsetree::ast::UnresolvedType>(
            impt->getQualifiedIdentifier()));
    if (!std::holds_alternative<std::shared_ptr<Decl>>(imptType)) {
      throw std::runtime_error(
          "Failed to resolve single-type imports to type at " +
          impt->getQualifiedIdentifier()->toString());
    }
    auto decl = std::get<std::shared_ptr<Decl>>(imptType);
    auto typeName = decl->getName();
    // Check: decl name should not clash with class name
    auto classDecl =
        std::dynamic_pointer_cast<parsetree::ast::Decl>(node->getBody());
    if (!classDecl)
      throw std::runtime_error("Body not Decl");
    if (classDecl->getName() == typeName && decl->getAstNode() != classDecl) {
      throw std::runtime_error("Single-Type Import name clash with class name" +
                               decl->getName());
    }

    // singleTypeImports[typeName] = Package::packageChild{decl};
    context[typeName] = Package::packageChild{decl};
  }

  // Step 5: All Decl from the Current AST
  // this will shadow everything
  if (auto body = node->getBody()) {
    if (auto bodyDecl = std::dynamic_pointer_cast<parsetree::ast::Decl>(body)) {
      context[bodyDecl->getName()] =
          Package::packageChild{std::make_shared<Body>(bodyDecl)};
    } else {
      throw std::runtime_error(
          "Body is not Decl in initContext, this should not happen");
    }
  }
}

Package::packageChild TypeLinker::resolveImport(
    std::shared_ptr<parsetree::ast::UnresolvedType> node) {
  // Base cases
  if (!node)
    throw std::runtime_error("Node is null when resolving import");
  if (node->isResolved())
    throw std::runtime_error("unresolved type should not be resolved yet");
  if (node->getIdentifiers().size() == 0) {
    return rootPackage->children[DEFAULT_PACKAGE_NAME];
  }
  return resolveImport(node->getIdentifiers());
}

Package::packageChild
TypeLinker::resolveImport(const std::vector<std::string> &identifiers) {
  if (identifiers.size() == 0) {
    return rootPackage->children[DEFAULT_PACKAGE_NAME];
  }
  Package::packageChild currentPkg = rootPackage;
  for (auto &id : identifiers) {
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

void TypeLinker::resolveType(std::shared_ptr<parsetree::ast::Type> type) {
  // only resolve if not resolved
  auto unresolvedType =
      std::dynamic_pointer_cast<parsetree::ast::UnresolvedType>(type);
  if (!unresolvedType)
    return;
  if (unresolvedType->isResolved())
    return; // tbh should not happen

  if (unresolvedType->getIdentifiers().size() == 0)
    return; // ??

  Package::packageChild currentType;
  bool first = true;
  for (auto &id : unresolvedType->getIdentifiers()) {
    if (first) {
      currentType = resolveSimpleName(id);
      if (std::holds_alternative<std::nullptr_t>(currentType)) {
        throw std::runtime_error("Could not resolve type at " + id);
      }
      first = false;
    } else {
      // interior nodes in the tree should not be decl
      if (std::holds_alternative<std::shared_ptr<Decl>>(currentType)) {
        throw std::runtime_error(
            "resolving package should not be decl when resolving type");
      }
      auto pkg = std::get<std::shared_ptr<Package>>(currentType);
      if (pkg->children.find(id) == pkg->children.end()) {
        throw std::runtime_error("Could not resolve type " + id +
                                 " since this is not found");
      }
      currentType = pkg->children.at(id);
    }
  }
  // check the leaf node is decl
  if (!std::holds_alternative<std::shared_ptr<Decl>>(currentType)) {
    throw std::runtime_error("resolved type should be decl");
  }
  unresolvedType->setResolvedDecl(std::get<std::shared_ptr<Decl>>(currentType));
}

/*
Simple names have no . in their names.
We traverse the namespaces in the following priority order:
  1. Enclosing class/interface
  2. Single-type imports (e.g. import a.b.c)
  3. Type in same package
  4. Import-on-demand package (e.g. import a.b.*)
*/
Package::packageChild
TypeLinker::resolveSimpleName(const std::string &simpleName) {
  auto &context = contextMap[currentProgram];
  return context.find(simpleName) != context.end() ? context[simpleName]
                                                   : nullptr;
}

} // namespace static_check
