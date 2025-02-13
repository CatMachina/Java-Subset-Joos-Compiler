#include "typeLinker.hpp"

namespace static_check {

void TypeLinker::buildSymbolTable() {
  for (auto programDecl : astManager->getASTs()) {
    /*
    // ... Entering a new scope, push new env to stack ...
    Environment env;
    stack.push(env);
    */

    // it should be UnresolvedType
    auto packageNode =
        std::dynamic_pointer_cast<parsetree::ast::UnresolvedType>(
            programDecl->getPackage());
    if (!packageNode) {
      throw std::runtime_error("Package should be unresolved type!");
    }

    // ... Visit package declaration ...
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
    // for symbol table build
    for (auto const &id : packageNode->getIdentifiers()) {
      // find id in symbol table. if not in we add and continue
      // else traverse into the next subpackage
    }
    // after building symbol table we can also check if program decl is in
    // symbol table if it is then duplicate! add program decl into symbol table

    /*
    // How to insert into env:
    // ... Create package object ...
    Package package;
    env.registerDecl({package, object});
    */
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
