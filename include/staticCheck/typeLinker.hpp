#pragma once

#include "ast/ast.hpp"
#include "globalEnvironment.hpp"

namespace static_check {

class TypeLinker {
public:
  ////////////////////// Resolvers ////////////////////

  // Resolver method to call for each AST
  // Essentially, AST nodes that represent a "use" will be decorated with a
  // Decl object (that is in global env)?

  // Resolver for single-type import

  // Resolver for type (class/interface)

  ////////////////////// Checkers ////////////////////

private:
  // Env manager (for local envs)

  // Global env? (package - classes/interfaces - fields/methods - variables)
};

} // namespace static_check
